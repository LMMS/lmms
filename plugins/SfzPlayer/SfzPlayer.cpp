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
	, m_parentTrack(instrumentTrack)
{
	auto iph = new InstrumentPlayHandle(this, instrumentTrack);
	Engine::audioEngine()->addPlayHandle(iph);

	// When a user loads a new SFZ file, the main thread prepares the new data to be swapped out by the audio thread, but the main thread
	// needs to delete the old data afterwards, so here we connect some kind of run-loop so that the main thread gets a chance to check after the audio thread acts
	connect(lmms::gui::getGUI()->mainWindow(), SIGNAL(periodicUpdate()), this, SLOT(mainThreadUpdateAfterDataSwap())); // I really don't like this. Would it be better to use our own QTimer, not LMMS's gui's?

	emit dataChanged();
}

SfzPlayer::~SfzPlayer()
{
	// Make sure to end the sample loading thread before the plugin exits, or else the program might forcefully terminate
	if (m_sampleLoadingThread.joinable()) { m_sampleLoadingThread.join(); }

	if (m_regionManager != nullptr) { delete m_regionManager; } // these may be nullptr at first when no SFZ file has been loaded previously
	if (m_samplePool != nullptr) { delete m_samplePool; }
	if (m_tempRegionManager != nullptr) { delete m_tempRegionManager; }
	if (m_tempSamplePool != nullptr) { delete m_tempSamplePool; }
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
}

void SfzPlayer::deleteNotePluginData(NotePlayHandle* handle)
{
}


void SfzPlayer::processTrigger(const SfzTrigger& trigger)
{
	if (m_regionManager == nullptr) { return; } // Before any SFZ file is loaded, m_regionManager is nullptr, so there's no regions to trigger.

	// Notify the global state to update which switch keys are active, update midi CC values, etc
	m_sfzGlobalState.processTrigger(trigger);
	
	for (auto* region : m_regionManager->findPotentialMatchingRegions(trigger))
	{
		// If the trigger conditions are met, spawn a new sound
		if (region->triggerConditionsMet(m_sfzGlobalState, trigger))
		{
			// Loop through array to find open position
			bool foundOpenPosition = false;
			for (size_t i = 0; i <= m_voices.size(); ++i)
			{
				auto& regionPlayState = m_voices[i];
				if (!regionPlayState.active())
				{
					regionPlayState = SfzRegionPlayState(region, trigger, m_sfzGlobalState);
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
			regionPlayState.processTrigger(trigger, m_sfzGlobalState);
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

	// The SfzParser generates all the SfzRegion objects, but it doesn't load any of the samples
	// The sample filenames are stored in the regions as from the `sample` opcode, so we just need to load the files into memory to use them
	// The samples are stored with relative paths with respect to the sfz file, so first find the parent directory:
	QDir parentDirectory = QFileInfo(filePath).absoluteDir();

	// Initialize a new sample pool for the new samples to be loaded in
	m_tempSamplePool = new SfzSamplePool();

	// To prevent the main gui thread from freezing while all the samples are loaded, start up a separate thread to handle loading everything
	m_currentlyLoadingSamples = true;
	if (m_sampleLoadingThread.joinable()) { m_sampleLoadingThread.join(); } // Reset the previous thread if it is active
	m_sampleLoadingThread = std::thread(&SfzPlayer::sampleLoadingThreadFunction, this, parentDirectory);
}

void SfzPlayer::sampleLoadingThreadFunction(const QDir& parentDirectory)
{
	int i = 0;
	for (auto* region : m_tempRegionManager->allRegions())
	{
		// Update the GUI info text to notify the user as samples are loaded
		setStatusInfo(QString("Loading sample %1/%2 %3").arg(i+1).arg(m_tempRegionManager->allRegions().size()).arg(QFileInfo(region->m_sampleFile.value_or("N/A")).fileName()));
		// Initialize the sample into the temporary pool, so that it doesn't disturb the audio thread which may still be using the previous samples.
		bool successfulLoadSample = region->initializeSample(parentDirectory, *m_tempSamplePool);
		if (!successfulLoadSample) { qDebug() << "[SFZ Player] An error occured when loading a sample."; } // TODO organize this debug info
		i++;
	}
	setStatusInfo(QString("Loaded %1 regions and %2 samples.").arg(m_tempRegionManager->allRegions().size()).arg(m_tempSamplePool->sampleCount()));
	// When the thread is done loading all the samples, set the flag to let the audio thread know it can swap the data
	m_bufferCounterWhenDataReady = m_bufferCounter; // Save the current frame counter so the main thread knows when enough buffers have passed that it can delete the old data
	m_newSfzDataReady = true;
	m_currentlyLoadingSamples = false; // TODO this doesn't seem thread safe, since there would be a brief moment in time where this is false but the thread is still active?
}

void SfzPlayer::audioThreadHandleNewSfzData()
{
	if (m_newSfzDataReady)
	{
		// Swap the temporary object pointers with the real ones
		std::swap(m_regionManager, m_tempRegionManager);
		std::swap(m_samplePool, m_tempSamplePool);
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
		if (m_tempRegionManager != nullptr) { delete m_tempRegionManager; m_tempRegionManager = nullptr; } // these may be nullptr at first when no SFZ file has been loaded previously
		if (m_tempSamplePool != nullptr) { delete m_tempSamplePool; m_tempSamplePool = nullptr; }
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
		// Update the GUI
		emit fileLoaded();
	}
}


void SfzPlayer::saveSettings(QDomDocument& document, QDomElement& element)
{
	element.setAttribute("sfzfile", m_sfzFilePath);
}

void SfzPlayer::loadSettings(const QDomElement& element)
{
	m_sfzFilePath = element.attribute("sfzfile");
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
