/*
 * SfzPlayer.cpp - Simple SFZ instrument player
 *
 * Copyright (c) 2026 Keratin
 *
 * This file is part of LMMS - https://lmms.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 *
 */

#include "SfzPlayer.h"
#include "SfzParser.h"

#include <QDomElement>
#include <QDebug>

#include "Engine.h"
#include "InstrumentPlayHandle.h"
#include "InstrumentTrack.h"
#include "PathUtil.h"
#include "ConfigManager.h"
#include "SfzPlayerView.h"
#include "Song.h"
#include "ThreadPool.h"
#include "embed.h"
#include "interpolation.h"
#include "plugin_export.h"

#include "GuiApplication.h"
#include "MainWindow.h"

namespace lmms
{

extern "C" {
Plugin::Descriptor PLUGIN_EXPORT sfzplayer_plugin_descriptor = {
	LMMS_STRINGIFY(PLUGIN_NAME),
	"SFZ Player",
	QT_TRANSLATE_NOOP("PluginBrowser", "Load .sfz Instrument Files"),
	"Keratin <3",
	0x0100,
	Plugin::Type::Instrument,
	new PluginPixmapLoader("logo"),
	"sfz",
	nullptr,
};
PLUGIN_EXPORT Plugin* lmms_plugin_main(Model* m, void*)
{
	return new SfzPlayer(static_cast<InstrumentTrack*>(m));
}
} // end extern


SfzPlayer::SfzPlayer(InstrumentTrack* instrumentTrack)
	: Instrument(instrumentTrack, &sfzplayer_plugin_descriptor, nullptr, Flag::IsSingleStreamed)
	, m_samplePool(SfzSamplePool::instance())
	, m_parentTrack(instrumentTrack)
	, m_preloadAllSamplesModel(false, this, tr("Preload All Samples"))
{
	auto iph = new InstrumentPlayHandle(this, instrumentTrack);
	Engine::audioEngine()->addPlayHandle(iph);

	// When a user loads a new SFZ file, the main thread prepares the new data to be swapped out by the audio thread, but the main thread
	// needs to delete the old data afterwards, so here we connect some kind of run-loop so that the main thread gets a chance to check after the audio thread acts
	connect(lmms::gui::getGUI()->mainWindow(), SIGNAL(periodicUpdate()), this, SLOT(mainThreadUpdateAfterDataSwap())); // I really don't like this. Would it be better to use our own QTimer, not LMMS's gui's?

	// If the user enables preloading all samples, load all the samples immediately
	connect(&m_preloadAllSamplesModel, &BoolModel::dataChanged, [&](){ if (m_preloadAllSamplesModel.value()) { preloadAllSamples(); }});

	emit dataChanged();
}

SfzPlayer::~SfzPlayer()
{
	// Make sure to end the sample loading thread before the plugin exits, or else the program might forcefully terminate
	if (m_sampleLoadingTask.valid()) { m_sampleLoadingTask.get(); }

	// Deleting the region objects will also destroy their shared_ptrs to the sample objects, which will delete them if no other SfzPlayers are also using them.
	if (m_regionManager != nullptr) { delete m_regionManager; } // these may be nullptr at first when no SFZ file has been loaded previously
	if (m_tempRegionManager != nullptr) { delete m_tempRegionManager; }
}



bool SfzPlayer::handleMidiEvent(const MidiEvent& event, const TimePos& time, f_cnt_t offset)
{
	if (event.type() == MidiNoteOn)
	{
		processTrigger(SfzTrigger::noteOnEvent(offset, event.key(), event.velocity()));
		// Store the velocity of this event so that we can apply it to the corresponding NoteOff event (see comment below)
		m_previousNoteOnVelocity.at(event.key()) = event.velocity();
		return true;
	}
	else if (event.type() == MidiNoteOff)
	{
		// TODO: Currently, LMMS by default has NoteOff events have 0 velocity. SFZ requires that the NoteOff velocity match its corresponding NoteOn velocity
		// To account for this, we track the last pressed velocity of each note in an array and use that
		const int previousVelocity = m_previousNoteOnVelocity.at(event.key());
		processTrigger(SfzTrigger::noteOffEvent(offset, event.key(), previousVelocity));
		return true;
	}
	else if (event.type() == MidiControlChange)
	{
		processTrigger(SfzTrigger::controlChangeEvent(offset, event.controllerNumber(), event.controllerValue()));
		return true;
	}
	return false;
}

void SfzPlayer::playNote(NotePlayHandle* handle, SampleFrame* workingBuffer)
{
	// TODO: Currently this instrument only uses midi events as input, and a single audio buffer as output.
	// It might be possible to split things up to use individual NotePlayHandles, but that is for another time.

	// In order for pitch bending and microtuning to work, the exact freq of the NotePlayHandle needs to be conveyed to the voices
	m_sfzGlobalState.updateNphFreq(handle->midiKey(), handle->frequency());
	// Same thing for note panning in the piano roll
	m_sfzGlobalState.updateNphPanning(handle->midiKey(), handle->getPanning());
}

void SfzPlayer::deleteNotePluginData(NotePlayHandle* handle)
{
}


void SfzPlayer::processTrigger(const SfzTrigger& trigger)
{
	if (m_regionManager == nullptr) { return; } // Before any SFZ file is loaded, m_regionManager is nullptr, so there's no regions to trigger.

	if (m_currentlyLoadingSamples) { return; } // There are some issues with setStatusInfo being called from multiple threads at once, so just to be safe, don't handle any note events while the samples thread is running.

	// Notify the global state to update which switch keys are active, update midi CC values, etc
	m_sfzGlobalState.processTrigger(trigger);

	bool preloadSamples = m_preloadAllSamplesModel.value(); // Cache this variable so that it isn't acessed every iteration
	
	for (auto* region : m_regionManager->findPotentialMatchingRegions(trigger))
	{
		// If the trigger conditions are met, spawn a new sound
		if (region->triggerConditionsMet(m_sfzGlobalState, trigger))
		{
			// If samples are loaded when notes are pressed, check if this region has its sample loaded yet. If not, quickly load it before spawning the voice.
			if (!preloadSamples && !region->sample())
			{
				setStatusInfo(QString("Loading sample %1").arg(QFileInfo(region->m_sampleFile.value_or("N/A")).fileName()));
				bool sampleInPool = false;
				bool successfulLoadSample = region->initializeSample(QFileInfo(m_sfzFilePath).absoluteDir(), *m_samplePool, &sampleInPool);
				if (successfulLoadSample)
				{
					setStatusInfo((sampleInPool ? QString("Loaded sample %1 (already cached)") : QString("Loaded sample %1")).arg(QFileInfo(region->m_sampleFile.value_or("N/A")).fileName()));
				}
				else
				{
					setStatusInfo(QString("An error occured when loading sample %1").arg(QFileInfo(region->m_sampleFile.value_or("N/A")).fileName()));
					continue;
				}
			}
			// Loop through array to find open position
			bool foundOpenPosition = false;
			for (size_t i = 0; i <= m_voices.size(); ++i)
			{
				auto& regionPlayState = m_voices[i];
				if (!regionPlayState.active())
				{
					regionPlayState = SfzRegionPlayState(region, trigger, &m_sfzGlobalState);
					// If this new index is above the current max active index, update it
					m_maxActiveIndex = std::max(m_maxActiveIndex, i);
					foundOpenPosition = true;
					break;
				}
			}
			// For fun, display the last played sample on the GUI
			if (foundOpenPosition)
			{
				setStatusInfo("Last Played Sample: " + QFileInfo(region->m_sampleFile.value_or("N/A")).fileName());
			}
			else
			{
				setStatusInfo("Warning: Too many active voices. Could not find vacant position buffer. ");
			}
		}
	}

	// Loop through all the active sounds to check if any need to be deactivated/released by the trigger
	for (size_t i = 0; i <= m_maxActiveIndex; ++i)
	{
		auto& regionPlayState = m_voices[i];

		if (regionPlayState.active())
		{
			regionPlayState.processTrigger(trigger);
			// If this was the max active index and the trigger caused it to deactivate, figure out what the next active index is
			if (!regionPlayState.active() && i == m_maxActiveIndex) { recalculateMaxActiveIndex(); }
		}
	}
}





void SfzPlayer::play(SampleFrame* workingBuffer)
{
	const f_cnt_t frames = Engine::audioEngine()->framesPerPeriod();

	// Render audio from each of the active voices
	for (size_t i = 0; i <= m_maxActiveIndex; ++i)
	{
		auto& regionPlayState = m_voices[i];
		if (!regionPlayState.active()) { continue; }

		regionPlayState.play(workingBuffer, frames);
		// If the play state deactivated during playback, and this was the max active index, figure out what the new max active index is
		if (!regionPlayState.active() && i == m_maxActiveIndex) { recalculateMaxActiveIndex(); }
	}


	// In the event that the user tries to load a new sfz file while the current one is playing, the region and sample data will be
	// loaded into temporary member variables, and it's the job of the audio thread to swap them into place when the data is ready
	audioThreadHandleNewSfzData();
	// Increment the buffer counter so that the main thread knows when it's safe to delete the old objects
	m_bufferCounter++;
}



void SfzPlayer::recalculateMaxActiveIndex()
{
	// Loop backward from the old max active index to find the next play state which is active
	while (m_maxActiveIndex > 0)
	{
		if (m_voices[m_maxActiveIndex].active()) { return; }
		else { m_maxActiveIndex--; }
	}
}



void SfzPlayer::loadFile(const QString& filePath)
{
	loadSfzFile(filePath, true); // Passing true to reset the InstrumentTrack's midi CC knobs to the SFZ file's defaults
	// Set the instrument name to the filename
	m_parentTrack->setName(PathUtil::cleanName(filePath));
}


void SfzPlayer::loadSfzFile(const QString& filePath, const bool resetCCKnobs)
{
	// If the sample loading thread is already running, don't let the user load another file.
	if (m_currentlyLoadingSamples)
	{
		qDebug() << "[SFZ Player] Requested to load new SFZ file while still loading samples from previous SFZ file. Ignoring.";
		return;
	}
	// Or, if the new sample/region data was just swapped into place a couple buffers ago, return just to be safe
	if (m_justSwappedData && m_bufferCounter <= m_bufferCounterWhenDataReady + 2)
	{
		qDebug() << "[SFZ Player] Requested to load new SFZ file while new region/sample data from previous SFZ file may not yet have been swapped into place. Ignoring.";
		return;
	}
	// Set this variable so that after the audio thread swaps the data, the main thread remembers whether to reset the CC knobs to the SFZ file defaults or just to the current InstrumentTrack CC knob values
	m_resetCCKnobs = resetCCKnobs;


	// Reset the note counts, midi cc values, etc
	m_sfzGlobalState = SfzGlobalState();
	// And any info about control labels, default values, etc
	m_controlsConfig = SfzControlsConfig();

	// Temporary vector to store SfzRegion objects as they are parsed
	std::vector<SfzRegion> regions;
	// Parse all the <region> headers of the .sfz (accounting for <global> and <group> defaults) and populate the regions vector with the new SfzRegions
	// The <control> header is also parsed into a separate object for easy access by the gui
	bool successfulParseFile = SfzParser::parseSfzFile(filePath, regions, m_controlsConfig);

	if (!successfulParseFile) { qDebug() << "[SFZ Player] An error occurred when parsing the SFZ file."; return; }

	// Set the initial cc values based on any `set_ccN` opcodes in the <control> header
	m_sfzGlobalState.initializeMidiCCValues(m_controlsConfig);
	// Setup a map for keeping track of which switch keys may be pressed. This is more efficient than having array with 128 element, since that takes much longer to loop over, and most keys are not switch keys
	m_sfzGlobalState.initializeSwitchKeysMap(m_controlsConfig);

	m_sfzFilePath = filePath;

	// Hand off the vector of regions to SfzRegionManager, which will sort them out to optimize trigger selection
	// Don't immediately set m_regionManager, since the audio thread may still be accessing it; instead set the temporary object
	m_tempRegionManager = new SfzRegionManager(regions);

	// Set the flag to let the audio thread know it can swap the data
	m_bufferCounterWhenDataReady = m_bufferCounter; // Save the current frame counter so the main thread knows when enough buffers have passed that it can delete the old data
	m_newSfzDataReady = true;
}


void SfzPlayer::preloadAllSamples()
{
	// (For some reason, when loading the settings of the "Preload All Samples" toggle, it sets the value, which triggers this function, so we need to check to make sure the regions have actually been initialized before continuing)
	if (m_regionManager == nullptr) { return; }
	// To prevent the main gui thread from freezing while all the samples are loaded, start up a separate thread to handle loading everything
	m_currentlyLoadingSamples = true;
	if (m_sampleLoadingTask.valid()) { m_sampleLoadingTask.get(); } // Reset the previous thread if it is active
	m_sampleLoadingTask = ThreadPool::instance().enqueue(&SfzPlayer::sampleLoadingThreadFunction, this);
}

void SfzPlayer::sampleLoadingThreadFunction()
{
	// The samples are stored with relative paths with respect to the sfz file, so first find the parent directory:
	QDir parentDirectory = QFileInfo(m_sfzFilePath).absoluteDir();

	int i = 0; // Count the number of regions
	int samplesLoadedFromDisk = 0; // Count the number of samples which had to be loaded from disk
	int samplesAlreadyInPool = 0; // Count the number of samples which had previously been loaded into the pool and didn't need to be loaded from disk
	int samplesFailedToLoad = 0; // Count the number of samples which couldn't be loaded (invalid path, etc)
	for (auto* region : m_regionManager->allRegions())
	{
		// Update the GUI info text to notify the user as samples are loaded
		setStatusInfo(QString("Loading sample %1/%2 %3").arg(i+1).arg(m_regionManager->allRegions().size()).arg(QFileInfo(region->m_sampleFile.value_or("N/A")).fileName()));
		// Initialize the sample into the temporary pool, so that it doesn't disturb the audio thread which may still be using the previous samples.
		bool sampleInPool = false;
		bool successfulLoadSample = region->initializeSample(parentDirectory, *m_samplePool, &sampleInPool);
		if (successfulLoadSample)
		{
			if (sampleInPool) { samplesAlreadyInPool++; }
			else { samplesLoadedFromDisk++; }
		}
		else
		{
			samplesFailedToLoad++;
			setStatusInfo(QString("An error occured when loading sample %1").arg(QFileInfo(region->m_sampleFile.value_or("N/A")).fileName()));
		}
		i++;
	}
	if (samplesFailedToLoad == 0)
	{
		setStatusInfo(QString("Initialized %1 regions.\nLoaded %2 samples from disk.\nRetrieved %3 from sample pool.").arg(m_regionManager->allRegions().size()).arg(samplesLoadedFromDisk).arg(samplesAlreadyInPool));
	}
	else
	{
		setStatusInfo(QString("Initialized %1 regions.\nLoaded %2 samples from disk.\nRetrieved %3 from sample pool.\nWARNING: Failed to load %4 samples, see logs for details.").arg(m_regionManager->allRegions().size()).arg(samplesLoadedFromDisk).arg(samplesAlreadyInPool).arg(samplesFailedToLoad));
	}
	m_currentlyLoadingSamples = false; // TODO this doesn't seem thread safe, since there would be a brief moment in time where this is false but the thread is still active?
}

void SfzPlayer::audioThreadHandleNewSfzData()
{
	if (m_newSfzDataReady)
	{
		// Swap the temporary regions with the real ones
		std::swap(m_regionManager, m_tempRegionManager);
		// And reset the active voices, since they have pointers to old region objects
		std::fill_n(m_voices.begin(), m_maxActiveIndex + 1, SfzRegionPlayState());
		m_newSfzDataReady = false;
		m_justSwappedData = true;
	}
}

void SfzPlayer::mainThreadUpdateAfterDataSwap()
{
	if (m_justSwappedData && m_bufferCounter > m_bufferCounterWhenDataReady + 2)
	{
		m_justSwappedData = false;
		// Now that the audio thread has swapped the data, delete the old sample pool and region manager pointers
		// Deleting the region objects will also delete the sample object shared_ptrs, so if no other SfzPlayers are using them, the samples willl be cleaned up.
		if (m_tempRegionManager != nullptr) { delete m_tempRegionManager; m_tempRegionManager = nullptr; } // these may be nullptr at first when no SFZ file has been loaded previously
		// Also set the midi CC knobs to match the defaults, or whatever the current InstrumentTrack midi CC knobs are
		for (int i = 0; i < NumMidiCCs; ++i)
		{
			if (m_resetCCKnobs)
			{
				// Reset the instrument track's midi CC knobs to the defaults of the SFZ
				m_parentTrack->midiCCModel(i)->setInitValue(m_sfzGlobalState.midiCCValue(i));
			}
			// Sync the internal knobs to the LMMS knobs. TODO why does `setInitValue` not trigger this?
			processTrigger(SfzTrigger::controlChangeEvent(0, i, m_parentTrack->midiCCModel(i)->value())); // TODO there may be a cleaner way to do this
		}
		// So that the GUI shows an accurate sample count, refresh the sample pool so that old pointers to samples are removed
		m_samplePool->clearExpiredWeakPtrs();
		// Update the GUI
		emit fileLoaded();

		// Now load the samples
		// The SfzParser generates all the SfzRegion objects, but it doesn't load any of the samples
		// The sample filenames are stored in the regions as from the `sample` opcode, so we just need to load the files into memory to use them
		// If the user had enabled loading all the samples at once up front, do that. Otherwise, they will be loaded whenever they are needed, in SfzPlayer::processTrigger()
		if (m_preloadAllSamplesModel.value())
		{
			// This is fine to do while the audio thread is running, since the sample pointers in the region objects are atomic
			preloadAllSamples();
		}
	}
}


void SfzPlayer::saveSettings(QDomDocument& document, QDomElement& element)
{
	element.setAttribute("sfzfile", m_sfzFilePath);
	m_preloadAllSamplesModel.saveSettings(document, element, "preload_all_samples");
}

void SfzPlayer::loadSettings(const QDomElement& element)
{
	m_sfzFilePath = element.attribute("sfzfile");
	m_preloadAllSamplesModel.loadSettings(element, "preload_all_samples");
	if (!m_sfzFilePath.isEmpty())
	{
		loadSfzFile(m_sfzFilePath, false); // Passing false to leave the user-set midi CC knobs alone, since they were loaded by the InstrumentTrack and shouldn't be reset to the SFZ's defaults.
	} // TODO add error handling, if path doesn't exist
}

QString SfzPlayer::nodeName() const
{
	return sfzplayer_plugin_descriptor.name;
}

gui::PluginView* SfzPlayer::instantiateView(QWidget* parent)
{
	return new gui::SfzPlayerView(this, parent);
}



void SfzPlayer::setStatusInfo(const QString& text)
{
	// Print to console
	qDebug().noquote() << "[SFZ Player]" << text;
	// And send to GUI
	// (Instead of actually sending a Qt signal, which seemed to cause off crashes, simply set a member variable and let the GUI read it when it wants)
	m_statusText = text;
}


} // namespace lmms
