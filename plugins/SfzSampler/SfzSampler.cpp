/*
 * SfzSampler.cpp - Simple SFZ instrument loader/editor
 *
 * Copyright (c) 2023 Daniel Kauss Serna <daniel.kauss.serna@gmail.com>
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
	nullptr,
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
	//QString path = ConfigManager::inst()->userSamplesDir() + "sfz/jlearman.jRhodes3c-master/jRhodes3c-looped-flac-sfz/";
	//loadFile(path + "_jRhodes-stereo-looped.sfz");

	//loadFile(ConfigManager::inst()->userSamplesDir() + "sfz/SplendidGrandPiano-master/Splendid\ Grand\ Piano.sfz");

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
	int noteIndex = handle->key();
	const fpp_t frames = handle->framesLeftForCurrentPeriod();
	const f_cnt_t offset = handle->noteOffset();
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
	for (auto& region : m_sfzRegions)
	{
		region.processTrigger(m_sfzGlobalState, trigger);
	}
}





void SfzSampler::play(SampleFrame* workingBuffer)
{
	const fpp_t frames = Engine::audioEngine()->framesPerPeriod();

	for (auto& region : m_sfzRegions)
	{
		// Render audio from each of the regions
		// This amounts to the regions themselves rendering the audio from each of their active SfzRegionPlayStates
		// We pass a temporary buffer which can be used for rendering samples and later summing it to the working buffer.
		bool anythingPlayed = region.play(workingBuffer, frames);
	}
}




void SfzSampler::loadFile(const QString& filePath)
{
	// Prevent the audio thread from accidentally looping through the region vector while it's being edited
	const auto guard = Engine::audioEngine()->requestChangesGuard();
	// Reset the note counts, midi cc values, etc
	m_sfzGlobalState = SfzGlobalState();
	// And any info about control labels, default values, etc
	m_controlsConfig = SfzControlsConfig();
	// Reset any loaded samples
	m_samplePool = SfzSamplePool();

	// Parse all the <region> headers of the .sfz (accounting for <global> and <group> defaults) and populate m_sfzRegions with the new SfzRegion
	// The <control> header is also parsed into a separate object for easy access by the gui
	bool successfulParseFile = SfzParser::parseSfzFile(filePath, m_sfzRegions, m_controlsConfig);

	if (!successfulParseFile) { qDebug() << "[SFZ Player] An error occurred when parsing the SFZ file."; return; }

	// The SfzParser generates all the SfzRegion objects, but it doesn't load any of the samples
	// The sample filenames are stored in the regions as from the `sample` opcode, so we just need to load the files into memory to use them
	// The samples are stored with relative paths with respect to the sfz file, so first find the parent directory:
	QDir parentDirectory = QFileInfo(filePath).absoluteDir();
	int i = 0;
	for (auto& region : m_sfzRegions)
	{
		qDebug() << "[SFZ Player] Loading sample" << i + 1 << "/" << m_sfzRegions.size() << region.m_sampleFile.value_or("N/A");
		bool successfulLoadSample = region.initializeSample(parentDirectory, m_samplePool);
		if (!successfulLoadSample) { qDebug() << "[SFZ Player] An error occured when loading a sample."; }
		i++;
	}
	qDebug() << "Loaded" << m_sfzRegions.size() << "regions and" << m_samplePool.sampleCount() << "samples.";

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
		loadFile(m_sfzFilePath);
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
