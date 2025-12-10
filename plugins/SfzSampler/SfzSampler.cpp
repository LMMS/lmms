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

#include <QDomElement>
#include <QDebug>

#include "Engine.h"
#include "InstrumentTrack.h"
#include "PathUtil.h"
#include "ConfigManager.h"
#include "SampleLoader.h"
#include "SfzSamplerView.h"
#include "Song.h"
#include "embed.h"
#include "interpolation.h"
#include "plugin_export.h"

namespace lmms {

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
	: Instrument(instrumentTrack, &sfzsampler_plugin_descriptor)
	, m_originalSample1()
	, m_originalSample2()
	, m_parentTrack(instrumentTrack)
{
	QString path = ConfigManager::inst()->userSamplesDir() + "sfz/jlearman.jRhodes3c-master/jRhodes3c-looped-flac-sfz/";
	if (auto buffer = gui::SampleLoader::createBufferFromFile(path + "As_029__F1_279-stereo.flac"))
	{
		m_originalSample1 = Sample(std::move(buffer));
	}
	if (auto buffer = gui::SampleLoader::createBufferFromFile(path + "As_035__B1_281-stereo.flac"))
	{
		m_originalSample2 = Sample(std::move(buffer));
	}

	emit dataChanged();
}

void SfzSampler::playNote(NotePlayHandle* handle, SampleFrame* workingBuffer)
{
	//if (m_originalSample.sampleSize() <= 1) { return; }

	int noteIndex = handle->key();
	const fpp_t frames = handle->framesLeftForCurrentPeriod();
	const f_cnt_t offset = handle->noteOffset();

	const f_cnt_t startFrame = 0;

	if (!handle->m_pluginData) { handle->m_pluginData = new Sample::PlaybackState(AudioResampler::Mode::Linear, startFrame); }

	auto playbackState = static_cast<Sample::PlaybackState*>(handle->m_pluginData);

	if (noteIndex % 2 == 0)
	{
		m_originalSample1.play(workingBuffer + offset, playbackState, frames, Sample::Loop::Off);
	}
	else
	{
		m_originalSample2.play(workingBuffer + offset, playbackState, frames, Sample::Loop::Off);
	}
}

void SfzSampler::deleteNotePluginData(NotePlayHandle* handle)
{
	delete static_cast<Sample::PlaybackState*>(handle->m_pluginData);
}


void SfzSampler::loadFile(const QString& file)
{
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
