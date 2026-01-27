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
			qDebug() << "Spawning sound!" << region->m_sampleFile.value_or("N/A");
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
	loadSfzFile(filePath);
	// Reset the instrument track's midi CC knobs to the defaults of the SFZ
	for (int i = 0; i < SfzOpcodeState::NumMidiCCs; ++i)
	{
		m_parentTrack->midiCCModel(i)->setInitValue(m_sfzGlobalState.midiCCValue(i));
		// For some reason it seems calling `setValue` on the CC models doesn't send a midi event to the instrument when doing drag/drop,
		// so we also send a trigger to update the region's
		processTrigger(SfzTrigger::controlChangeEvent(0, i, m_parentTrack->midiCCModel(i)->value())); // TODO there may be a cleaner way to do this
	}
}


void SfzSampler::loadSfzFile(const QString& filePath)
{
	// Prevent the audio thread from accidentally looping through the regions while they are being edited
	const auto guard = Engine::audioEngine()->requestChangesGuard();
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
	m_regionManager = SfzRegionManager(regions); // TODO should move semantics be used here?

	// The SfzParser generates all the SfzRegion objects, but it doesn't load any of the samples
	// The sample filenames are stored in the regions as from the `sample` opcode, so we just need to load the files into memory to use them
	// The samples are stored with relative paths with respect to the sfz file, so first find the parent directory:
	QDir parentDirectory = QFileInfo(filePath).absoluteDir();
	// Reset any loaded samples
	m_samplePool = SfzSamplePool();
	int i = 0;
	for (auto* region : m_regionManager.allRegions())
	{
		qDebug() << "[SFZ Player] Loading sample" << i + 1 << "/" << m_regionManager.allRegions().size() << region->m_sampleFile.value_or("N/A");
		bool successfulLoadSample = region->initializeSample(parentDirectory, m_samplePool);
		if (!successfulLoadSample) { qDebug() << "[SFZ Player] An error occured when loading a sample."; }
		i++;
	}
	qDebug() << "Loaded" << m_regionManager.allRegions().size() << "regions and" << m_samplePool.sampleCount() << "samples.";


	// Set the initial cc values based on any `set_ccN` opcodes in the <control> header
	m_sfzGlobalState.initializeMidiCCValues(m_controlsConfig);

	m_sfzFilePath = filePath;
	emit fileLoaded();
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
		// Using `loadSfzFile` instead of `loadFile` to bypass resetting the midi CC knobs
		loadSfzFile(m_sfzFilePath);
	}
	// Sync the internal CC values so that saved presets/projects work normally upon loading
	for (int i = 0; i < SfzOpcodeState::NumMidiCCs; ++i)
	{
		processTrigger(SfzTrigger::controlChangeEvent(0, i, m_parentTrack->midiCCModel(i)->value()));
	}
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
