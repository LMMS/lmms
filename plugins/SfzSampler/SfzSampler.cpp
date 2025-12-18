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
	, m_tempBuffer(new SampleFrame[Engine::audioEngine()->framesPerPeriod()])
	, m_parentTrack(instrumentTrack)
{
	QString path = ConfigManager::inst()->userSamplesDir() + "sfz/jlearman.jRhodes3c-master/jRhodes3c-looped-flac-sfz/";

	loadFile(path + "_jRhodes-stereo-looped.sfz");

	auto iph = new InstrumentPlayHandle(this, instrumentTrack);
	Engine::audioEngine()->addPlayHandle(iph);

	emit dataChanged();
}


SfzSampler::~SfzSampler()
{
	delete[] m_tempBuffer;
}



bool SfzSampler::handleMidiEvent(const MidiEvent& event, const TimePos& time, f_cnt_t offset)
{
	if (event.type() == MidiNoteOn)
	{
		int key = event.key();
		int velocity = event.velocity();
		qDebug() << "Note on!" << key << velocity;
	}
	return true;
}


void SfzSampler::play(SampleFrame* workingBuffer)
{
	const fpp_t frames = Engine::audioEngine()->framesPerPeriod();
}



void SfzSampler::playNote(NotePlayHandle* handle, SampleFrame* workingBuffer)
{
	int noteIndex = handle->key();
	const fpp_t frames = handle->framesLeftForCurrentPeriod();
	const f_cnt_t offset = handle->noteOffset();

	if (!handle->m_pluginData) 
	{
		qDebug() << "Note play handle no plugin data!";
		// Find an empty active note array
		for (int i = 0; i < MAX_ACTIVE_NOTES; ++i)
		{
			if (m_activeNoteArrays[i] == std::nullopt)
			{
				handle->m_pluginData = &m_activeNoteArrays[i];
			}
		}
		// Did we find an open array?
		if (!handle->m_pluginData) { qDebug() << "[SFZ Player] Could not find vacant note array in buffer!"; return; }
		
		auto* activeNoteArray = static_cast<std::optional<std::array<SfzRegionPlayState, MAX_SOUNDS_PER_NOTE_PRESS>>*>(handle->m_pluginData);

		// Now loop through the regions are check which ones meet the conditions to spawn a new note

		for (auto region : m_sfzRegions)
		{
			// TODO
		}
	}
}

void SfzSampler::deleteNotePluginData(NotePlayHandle* handle)
{
	//delete static_cast<Sample::PlaybackState*>(handle->m_pluginData);
}


void SfzSampler::loadFile(const QString& filepath)
{
	bool successful = SfzParser::parseSfzFile(filepath, m_sfzRegions);
	qDebug() << "was okay?" << successful;
	qDebug() << "num regions" << m_sfzRegions.size();
	qDebug() << "first region sample" << m_sfzRegions[0].m_sample.value_or("aaaa");
	qDebug() << "first region lokey" << m_sfzRegions[0].m_lokey.value_or(-1);
	qDebug() << "first region hikey" << m_sfzRegions[0].m_hikey.value_or(-1);
	qDebug() << "first region lovel" << m_sfzRegions[0].m_lovel.value_or(-1);
	qDebug() << "first region hivel" << m_sfzRegions[0].m_hivel.value_or(-1);
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
