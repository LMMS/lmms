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
#include "SampleLoader.h"
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
		processTrigger(SfzTrigger::noteOnEvent(event.key(), event.velocity()));
		return true;
	}
	else if (event.type() == MidiNoteOff)
	{
		processTrigger(SfzTrigger::noteOffEvent(event.key(), event.velocity()));
		return true;
	}
	else if (event.type() == MidiControlChange)
	{
		processTrigger(SfzTrigger::controlChangeEvent(event.controllerNumber(), event.controllerValue()));
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
	// Parse all the <region> headers of the .sfz (accounting for <global> and <group> defaults) and populate m_sfzRegions with the new SfzRegion
	bool successfulParseFile = SfzParser::parseSfzFile(filePath, m_sfzRegions);

	qDebug() << "was okay?" << successfulParseFile;
	qDebug() << "num regions" << m_sfzRegions.size();

	// The SfzParser generates all the SfzRegion objects, but it doesn't load any of the samples
	// The sample filenames are stored in the regions as from the `sample` opcode, so we just need to load the files into memory to use them
	// The samples are stored with relative paths with respect to the sfz file, so first find the parent directory:
	QDir parentDirectory = QFileInfo(filePath).absoluteDir();
	for (auto& region : m_sfzRegions)
	{
		bool successfulLoadSample = region.initializeSample(parentDirectory);
		qDebug() << "sample was load okay?" << successfulLoadSample;
	}
}

void SfzSampler::saveSettings(QDomDocument& document, QDomElement& element)
{
}

void SfzSampler::loadSettings(const QDomElement& element)
{
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
