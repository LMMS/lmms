/*
 * SfzSampler.cpp - Simple SFZ instrument player
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

#include "SfzSampler.h"
#include "SfzParser.h"

#include <QDomElement>
#include <QDebug>

#include "Engine.h"
#include "InstrumentPlayHandle.h"
#include "InstrumentTrack.h"
#include "PathUtil.h"
#include "ConfigManager.h"
#include "SfzSamplerView.h"
#include "Song.h"
#include "embed.h"
#include "interpolation.h"
#include "plugin_export.h"

namespace lmms
{

extern "C" {
Plugin::Descriptor PLUGIN_EXPORT sfzsampler_plugin_descriptor = {
	LMMS_STRINGIFY(PLUGIN_NAME),
	"SfzSampler",
	QT_TRANSLATE_NOOP("PluginBrowser", "Basic Slicer"),
	"Daniel Kauss Serna <daniel.kauss.serna@gmail.com>",
	0x0100,
	Plugin::Type::Instrument,
	new PluginPixmapLoader("logo"),
	"sfz",
	nullptr,
};
PLUGIN_EXPORT Plugin* lmms_plugin_main(Model* m, void*)
{
	return new SfzSampler(static_cast<InstrumentTrack*>(m));
}
} // end extern


SfzSampler::SfzSampler(InstrumentTrack* instrumentTrack)
	: Instrument(instrumentTrack, &sfzsampler_plugin_descriptor, nullptr, Flag::IsSingleStreamed)
	, m_parentTrack(instrumentTrack)
{
	auto iph = new InstrumentPlayHandle(this, instrumentTrack);
	Engine::audioEngine()->addPlayHandle(iph);

	emit dataChanged();
}



bool SfzSampler::handleMidiEvent(const MidiEvent& event, const TimePos& time, f_cnt_t offset)
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

void SfzSampler::playNote(NotePlayHandle* handle, SampleFrame* workingBuffer)
{
	//int noteIndex = handle->key();
	//const fpp_t frames = handle->framesLeftForCurrentPeriod();
	//const f_cnt_t offset = handle->noteOffset();
}

void SfzSampler::deleteNotePluginData(NotePlayHandle* handle)
{
	//delete static_cast<Sample::PlaybackState*>(handle->m_pluginData);
}


void SfzSampler::processTrigger(const SfzTrigger& trigger)
{
	// Notify the global state to update which keys are active, update midi CC values, etc
	m_sfzGlobalState.processTrigger(trigger);

	// Loop through all the regions to check if a new note should be played
	// TODO can we get rid of this loop
	for (auto* region : m_regionManager.allRegions())
	{
		// Notify the region of the event so that it can update cached CC modulations, keyswitch states, etc
		region->processTrigger(m_sfzGlobalState, trigger);
	}
	
	for (auto* region : m_regionManager.findPotentialMatchingRegions(trigger))
	{
		// If the trigger conditions are met, spawn a new sound
		if (region->triggerConditionsMet(m_sfzGlobalState, trigger))
		{
			qDebug() << "Spawning sound!" << region->m_sampleFile.value().value_or("N/A");
			// Loop through array to find open position
			bool foundOpenPosition = false;
			for (size_t i = 0; i <= m_voices.size(); ++i)
			{
				auto& regionPlayState = m_voices[i];
				if (!regionPlayState.active())
				{
					regionPlayState = SfzRegionPlayState(region, trigger);
					// If this new index is above the current max active index, update it
					m_maxActiveIndex = std::max(m_maxActiveIndex, i);
					foundOpenPosition = true;
					break;
				}
			}
			if (!foundOpenPosition) { qDebug() << "[SFZ Player] Could not find vacant position in m_voices buffer!"; }
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





void SfzSampler::play(SampleFrame* workingBuffer)
{
	const fpp_t frames = Engine::audioEngine()->framesPerPeriod();

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
	// loaded into temporary member variables, and it's the job of the audio thread to swap them into place when the data is ready and it gets a chance
	if (m_newSfzDataReady)
	{
		m_regionManager = std::move(m_tempRegionManager);
		m_samplePool = std::move(m_tempSamplePool);
		// And reset the voices, since they may have invalid pointers to old region objects
		std::fill(m_voices.begin(), m_voices.end(), SfzRegionPlayState());
		m_newSfzDataReady = false;
		m_justSwappedData = true;
		// Also set the midi CC knobs to match the defaults, or whatever the current InstrumentTrack midi CC knobs are
		// TODO can this be moved to the main thread?
		for (int i = 0; i < NumMidiCCs; ++i)
		{
			if (m_resetCCKnobs)
			{
				// Reset the instrument track's midi CC knobs to the defaults of the SFZ
				m_parentTrack->midiCCModel(i)->setInitValue(m_sfzGlobalState.midiCCValue(i));
				qDebug() << "Resetting lmms CC knob" << i << m_sfzGlobalState.midiCCValue(i);
			}
			qDebug() << "Resetting internal CC" << i << m_parentTrack->midiCCModel(i)->value();
			// Sync the internal knobs to the LMMS knobs. TODO why does `setInitValue` not trigger this?
			processTrigger(SfzTrigger::controlChangeEvent(0, i, m_parentTrack->midiCCModel(i)->value())); // TODO there may be a cleaner way to do this
		}
		qDebug() << "Audio thread: new data ready, swapped.";
	}
	// Increment the buffer counter so that the main thread knows when it's safe to touch the temp objects again
	m_bufferCounter++;
}



void SfzSampler::recalculateMaxActiveIndex()
{
	// Loop backward from the old max active index to find the next play state which is active
	while (m_maxActiveIndex > 0)
	{
		if (m_voices[m_maxActiveIndex].active()) { return; }
		else { m_maxActiveIndex--; }
	}
}



void SfzSampler::loadFile(const QString& filePath)
{
	loadSfzFile(filePath, true); // Passing true to reset the InstrumentTrack's midi CC knobs to the SFZ file's defaults
}


void SfzSampler::loadSfzFile(const QString& filePath, const bool resetCCKnobs)
{
	// If the sample loading thread is already running, don't let the user load another file.
	if (m_currentlyLoadingSamples) { qDebug() << "main thread: Currently loading samples, can't right now."; return; }
	// Or, if the new sample/region data was just swapped into place a couple buffers ago, return just to be safe
	if (m_justSwappedData && m_bufferCounter <= m_bufferCounterWhenDataReady + 2) { qDebug() << "main thread: not enough buffers passed, not safe"; return; }
	else if (m_justSwappedData)
	{
		// If enough buffers have passed we assume the swap was successful, so reset the flag, and prep the internal CC values to match the knobs
		m_justSwappedData = false;
	}
	// Set this variable so that when the audio thread is swapping data and reset the CC knobs, it knows whether to reset them to the SFZ file defaults or just to the current InstrumentTrack CC knob values
	m_resetCCKnobs = resetCCKnobs;


	// Reset the note counts, midi cc values, etc
	m_sfzGlobalState = SfzGlobalState();
	// And any info about control labels, default values, etc
	m_controlsConfig = SfzControlsConfig();

	// Temporary vector to store SfzRegion objects
	std::vector<SfzRegion> regions;
	// Parse all the <region> headers of the .sfz (accounting for <global> and <group> defaults) and populate the regions vector with the new SfzRegions
	// The <control> header is also parsed into a separate object for easy access by the gui
	bool successfulParseFile = SfzParser::parseSfzFile(filePath, regions, m_controlsConfig);

	if (!successfulParseFile) { qDebug() << "[SFZ Player] An error occurred when parsing the SFZ file."; return; }

	// Hand off the vector of regions to SfzRegionManager, which will sort them out to optimize trigger selection
	// Don't immediately set m_regionManager, since the audio thread may still be accessing it; instead set the temporary object
	m_tempRegionManager = SfzRegionManager(regions);

	// The SfzParser generates all the SfzRegion objects, but it doesn't load any of the samples
	// The sample filenames are stored in the regions as from the `sample` opcode, so we just need to load the files into memory to use them
	// The samples are stored with relative paths with respect to the sfz file, so first find the parent directory:
	QDir parentDirectory = QFileInfo(filePath).absoluteDir();

	// Set the initial cc values based on any `set_ccN` opcodes in the <control> header
	m_sfzGlobalState.initializeMidiCCValues(m_controlsConfig);

	m_sfzFilePath = filePath;
	emit fileLoaded();

	// To prevent the main gui thread from freezing while all the samples are loaded, start up a separate thread to handle everything
	// This thread will send back a signal when each sample is loaded, along with a final signal when it's finished
	m_currentlyLoadingSamples = true;
	m_sampleLoadingThread = std::jthread([this, parentDirectory](){
		int i = 0;
		for (auto* region : m_tempRegionManager.allRegions())
		{
			qDebug() << "[SFZ Player] Loading sample" << i + 1 << "/" << m_tempRegionManager.allRegions().size() << region->m_sampleFile.value().value_or("N/A");
			// Initialize the sample into the temporary pool, so that it doesn't disturb the audio thread which may still be using the previous samples.
			bool successfulLoadSample = region->initializeSample(parentDirectory, m_tempSamplePool); // TODO does this copy the sample pool object?
			if (!successfulLoadSample) { qDebug() << "[SFZ Player] An error occured when loading a sample."; }
			//emit sampleLoaded(i, m_tempRegionManager.allRegions().size(), region->m_sampleFile.value().value_or("N/A"));
			i++;
		}
		qDebug() << "Loaded" << m_tempRegionManager.allRegions().size() << "regions and" << m_tempSamplePool.sampleCount() << "samples.";
		// When the thread is done loading all the samples, set the flag to let the audio thread know it can swap the data
		m_bufferCounterWhenDataReady = m_bufferCounter; // Save the current frame counter so the main thread knows when enough buffers have passed that it's safe to touch the temp objects again
		m_newSfzDataReady = true;
		m_currentlyLoadingSamples = false; // TODO this doesn't seem thread safe, since there would be a brief moment in time where this is false but the thread is still active? Maybe it doesn't matter since jthread handles destruction more nicely.
	});
}


void SfzSampler::saveSettings(QDomDocument& document, QDomElement& element)
{
	element.setAttribute("sfzfile", m_sfzFilePath);
}

void SfzSampler::loadSettings(const QDomElement& element)
{
	m_sfzFilePath = element.attribute("sfzfile");
	if (!m_sfzFilePath.isEmpty())
	{
		loadSfzFile(m_sfzFilePath, true); // Passing false to leave the user-set midi CC knobs alone, since they were loaded by the InstrumentTrack and shouldn't be reset to the SFZ's defaults.
	} // TODO add error handling, if path doesn't exist
}

QString SfzSampler::nodeName() const
{
	return sfzsampler_plugin_descriptor.name;
}

gui::PluginView* SfzSampler::instantiateView(QWidget* parent)
{
	return new gui::SfzSamplerView(this, parent);
}

} // namespace lmms
