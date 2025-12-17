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




int keyStringToInt(QString keyString)
{
	bool ok;
	int asInt = keyString.toInt(&ok);
	if (ok) { return asInt; }
	qDebug() << "yyayyyy";
	QString octaveString = keyString.right(1);
	QString keyName = keyString.chopped(1).toLower();
	qDebug() << octaveString << keyName;
	bool ok2;
	int octave = octaveString.toInt(&ok2);
	if (!ok2) { qDebug() << "AAAA bad octave"; return 0; }
	int keyOffset = 0;
	if (keyName == "a") { keyOffset = 0; }
	else if (keyName == "a#" || keyName == "bb") { keyOffset = 1; }
	else if (keyName == "b") { keyOffset = 2; }
	else if (keyName == "c") { keyOffset = 3; }
	else if (keyName == "c#" || keyName == "db") { keyOffset = 4; }
	else if (keyName == "d") { keyOffset = 5; }
	else if (keyName == "d#" || keyName == "eb") { keyOffset = 6; }
	else if (keyName == "e") { keyOffset = 7; }
	else if (keyName == "f") { keyOffset = 8; }
	else if (keyName == "f#" || keyName == "gb") { keyOffset = 9; }
	else if (keyName == "g") { keyOffset = 10; }
	else if (keyName == "g#" || keyName == "ab") { keyOffset = 11; }
	else { qDebug() << "AAAA bad key";  return 0; }
	qDebug() << "returning" << 21 + octave * 12 + keyOffset;
	return 21 + octave * 12 + keyOffset;
}


SfzSampler::SfzSampler(InstrumentTrack* instrumentTrack)
	: Instrument(instrumentTrack, &sfzsampler_plugin_descriptor, nullptr, Flag::IsSingleStreamed)
	, m_originalSample1()
	, m_originalSample2()
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
		handle->m_pluginData = ;
	}
}

void SfzSampler::deleteNotePluginData(NotePlayHandle* handle)
{
	//delete static_cast<Sample::PlaybackState*>(handle->m_pluginData);
}


void SfzSampler::loadFile(const QString& filepath)
{
	QString parentDirectory = QFileInfo(filepath).absoluteDir().path() + "/";
	qDebug() << "File!" << filepath;
	qDebug() << "Dir!" << parentDirectory;
	QFile file(filepath);

	if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) { qDebug() << "Could not read!!"; return; }

	while (!file.atEnd())
	{
		QString line = file.readLine();
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
