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
	: Instrument(instrumentTrack, &sfzsampler_plugin_descriptor, nullptr, Flag::IsSingleStreamed | Flag::IsMidiBased)
	, m_originalSample1()
	, m_originalSample2()
	, m_tempBuffer(new SampleFrame[Engine::audioEngine()->framesPerPeriod()])
	, m_parentTrack(instrumentTrack)
{
	QString path = ConfigManager::inst()->userSamplesDir() + "sfz/jlearman.jRhodes3c-master/jRhodes3c-looped-flac-sfz/";

	//loadFile(path + "_jRhodes-stereo-looped.sfz");

	auto iph = new InstrumentPlayHandle(this, instrumentTrack);
	Engine::audioEngine()->addPlayHandle( iph );

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
		m_noteStates[key].pressed = true;
		m_noteStates[key].velocity = velocity;
		m_noteStates[key].frameCounter = -offset;
		m_noteStates[key].playbackState = Sample::PlaybackState(AudioResampler::Mode::Linear);
		qDebug() << "Note on!" << key << velocity;
	}
	if (event.type() == MidiNoteOff)
	{
		int key = event.key();
		int velocity = event.velocity();
		m_noteStates[key].pressed = false;
		//m_noteStates[key].velocity = velocity;
		m_noteStates[key].frameReleased = m_noteStates[key].frameCounter;
		qDebug() << "Note off!" << key << velocity;
	}

	float sampleRate = Engine::audioEngine()->outputSampleRate();

	// Loop through all regions, and find ones which match the current state
	// If they match, play them
	if (event.type() == MidiNoteOn || event.type() == MidiNoteOff)
	{
		int key = event.key();
		int velocity = event.velocity();
		for (auto& group: m_sfz.m_groups)
		{
			for (auto& region: group.m_regions)
			{
				int lokey = region.m_settings.lokey.value_or(group.m_globalSettings.lokey.value_or(m_sfz.m_globalSettings.lokey.value_or(0)));
				int hikey = region.m_settings.hikey.value_or(group.m_globalSettings.hikey.value_or(m_sfz.m_globalSettings.hikey.value_or(0)));
				int pitch_keycenter = region.m_settings.pitch_keycenter.value_or(group.m_globalSettings.pitch_keycenter.value_or(m_sfz.m_globalSettings.pitch_keycenter.value_or(0)));
				int lovel = region.m_settings.lovel.value_or(group.m_globalSettings.lovel.value_or(m_sfz.m_globalSettings.lovel.value_or(255)));
				int hivel = region.m_settings.hivel.value_or(group.m_globalSettings.hivel.value_or(m_sfz.m_globalSettings.hivel.value_or(0)));
				float ampeg_hold = region.m_settings.ampeg_hold.value_or(group.m_globalSettings.ampeg_hold.value_or(m_sfz.m_globalSettings.ampeg_hold.value_or(0)));
				float ampeg_decay = region.m_settings.ampeg_decay.value_or(group.m_globalSettings.ampeg_decay.value_or(m_sfz.m_globalSettings.ampeg_decay.value_or(0)));
				float ampeg_sustain = region.m_settings.ampeg_sustain.value_or(group.m_globalSettings.ampeg_sustain.value_or(m_sfz.m_globalSettings.ampeg_sustain.value_or(0)));
				float ampeg_release = region.m_settings.ampeg_release.value_or(group.m_globalSettings.ampeg_release.value_or(m_sfz.m_globalSettings.ampeg_release.value_or(0)));
				//qDebug() << "Checking!" << lokey << hikey << pitch_keycenter << lovel << hivel;
				if (key >= lokey && key <= hikey && velocity >= lovel && velocity <= hivel)
				{
					qDebug() << "Found a match!!!" << region.m_settings.lokey.value_or(-1) << region.m_settings.hikey.value_or(-1) << region.m_settings.lovel.value_or(-1) << region.m_settings.hivel.value_or(-1) << region.m_settings.sampleFile.value_or("idk");
					m_noteStates[key].sampleIndex = region.m_settings.sampleIndex;
					int keydiff = key - pitch_keycenter;
					double pitchRatio = std::exp2(-keydiff / 12.0);
					m_noteStates[key].pitchRatio = pitchRatio;
					qDebug() << "Pitch ratio:" << pitchRatio;
					m_noteStates[key].holdFrames = ampeg_hold * sampleRate;
					m_noteStates[key].decayFrames = ampeg_decay * sampleRate;
					m_noteStates[key].sustainLevel = ampeg_sustain / 100.0f;
					m_noteStates[key].releaseFrames = ampeg_release * sampleRate;
				}
			}
		}
	}

	return true;
}


void SfzSampler::play(SampleFrame* workingBuffer)
{
	const fpp_t frames = Engine::audioEngine()->framesPerPeriod();
	for (int key = 0; key < 128; ++key)
	{
		//m_noteStates[key].playbackState
		f_cnt_t offsetFrames = 0;
		if (m_noteStates[key].frameCounter < 0 && frames - m_noteStates[key].frameCounter > 0)
		{
			offsetFrames = frames - m_noteStates[key].frameCounter;
		}
		if (m_noteStates[key].sampleIndex != -1)
		{
			// don't play if past release
			if (!m_noteStates[key].pressed && m_noteStates[key].frameCounter - m_noteStates[key].frameReleased > m_noteStates[key].releaseFrames) { continue; }
			
			m_samples.at(m_noteStates[key].sampleIndex).play(m_tempBuffer + offsetFrames, &m_noteStates[key].playbackState, frames - offsetFrames, Sample::Loop::Off, m_noteStates[key].pitchRatio);
			
			for (f_cnt_t f = 0; f < frames; ++f)
			{
				float ampeg_multiplier = 1.0f;
				ampeg_multiplier *= (m_noteStates[key].velocity / 127.0f);
				if (m_noteStates[key].pressed)
				{
					if (m_noteStates[key].frameCounter > 0 && m_noteStates[key].frameCounter < 0) // TODO attack
					{
						ampeg_multiplier *= 1.0f; // TODO
						//qDebug() << "Attack" << m_noteStates[key].frameCounter << m_noteStates[key].attackFrames << m_noteStates[key].holdFrames << m_noteStates[key].decayFrames << m_noteStates[key].sustainLevel << m_noteStates[key].releaseFrames;
					}
					else if (m_noteStates[key].frameCounter < m_noteStates[key].holdFrames)
					{
						ampeg_multiplier *= 1.0f;
						//qDebug() << "Hold" << m_noteStates[key].frameCounter << m_noteStates[key].attackFrames << m_noteStates[key].holdFrames << m_noteStates[key].decayFrames << m_noteStates[key].sustainLevel << m_noteStates[key].releaseFrames;
					}
					else if (m_noteStates[key].frameCounter < m_noteStates[key].holdFrames + m_noteStates[key].decayFrames)
					{
						ampeg_multiplier *= 1.0f - (1.0f - m_noteStates[key].sustainLevel) * 1.0f * (m_noteStates[key].frameCounter - m_noteStates[key].holdFrames) / m_noteStates[key].decayFrames;
						//qDebug() << 1.0f * (m_noteStates[key].frameCounter - m_noteStates[key].holdFrames) / m_noteStates[key].decayFrames << ampeg_multiplier;
						//qDebug() << "Decay" << m_noteStates[key].frameCounter << m_noteStates[key].attackFrames << m_noteStates[key].holdFrames << m_noteStates[key].decayFrames << m_noteStates[key].sustainLevel << m_noteStates[key].releaseFrames;
					}
					else
					{
						ampeg_multiplier *= m_noteStates[key].sustainLevel;
						//qDebug() << "Sustain" << m_noteStates[key].frameCounter << m_noteStates[key].attackFrames << m_noteStates[key].holdFrames << m_noteStates[key].decayFrames << m_noteStates[key].sustainLevel << m_noteStates[key].releaseFrames;
					}
				}
				else
				{
					if (m_noteStates[key].frameCounter - m_noteStates[key].frameReleased < m_noteStates[key].releaseFrames)
					{
						ampeg_multiplier *= 1.0f - 1.0f * (m_noteStates[key].frameCounter - m_noteStates[key].frameReleased) / m_noteStates[key].releaseFrames;
						//qDebug() << "Release" << m_noteStates[key].frameCounter << m_noteStates[key].attackFrames << m_noteStates[key].holdFrames << m_noteStates[key].decayFrames << m_noteStates[key].sustainLevel << m_noteStates[key].releaseFrames;
					}
					else
					{
						ampeg_multiplier *= 0.0f;
					}
				}
				workingBuffer[f] += m_tempBuffer[f] * ampeg_multiplier;
				m_noteStates[key].frameCounter += 1;
			}
		}
	}
}



/*void SfzSampler::playNote(NotePlayHandle* handle, SampleFrame* workingBuffer)
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
}*/


void SfzSampler::loadFile(const QString& filepath)
{

	m_samples.clear();

	QString parentDirectory = QFileInfo(filepath).absoluteDir().path() + "/";
	qDebug() << "File!" << filepath;
	qDebug() << "Dir!" << parentDirectory;
	QFile file(filepath);

	if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) { qDebug() << "Could not read!!"; return; }

	bool inGlobal = false;
	bool inGroup = false;
	bool inRegion = false;

	while (!file.atEnd())
	{
		QString line = file.readLine();
		// Trim comments off end
		line = line.split("//")[0];
		line = line.trimmed();
		qDebug() << line;
		// Split by "=" TODO make better
		QStringList segments = line.split("=");
		if (segments.length() == 0) { continue; }
		if (segments.length() == 1)
		{
			if (line == "<global>")
			{
				qDebug() << "Global!";
				inGlobal = true;
				inGroup = false;
				inRegion = false;
			}
			if (line == "<group>")
			{
				qDebug() << "New Group!";
				inGlobal = false;
				inGroup = true;
				inRegion = false;
				m_sfz.m_groups.push_back(SfzGroup());
			}
			if (line == "<region>")
			{
				qDebug() << "New Region!";
				inGlobal = false;
				inGroup = false;
				inRegion = true;
				m_sfz.m_groups.back().m_regions.push_back(SfzRegion());
			}
		}
		if (segments.length() == 2)
		{
			QString opcode = segments[0];
			QString value = segments[1];
			SfzSettingState& currentSettingsState = inGlobal
				? m_sfz.m_globalSettings
				: inGroup
					? m_sfz.m_groups.back().m_globalSettings
					: m_sfz.m_groups.back().m_regions.back().m_settings;
			if (opcode == "sample")
			{
				currentSettingsState.sampleFile = value;
				qDebug() << "LOADING SAMPLE!!!" << value;
				if (auto buffer = gui::SampleLoader::createBufferFromFile(parentDirectory + value))
				{
					m_samples.push_back(Sample(std::move(buffer)));
					currentSettingsState.sampleIndex = m_samples.size() - 1;
				}
			}
			else if (opcode == "lokey")
			{
				currentSettingsState.lokey = keyStringToInt(value);
			}
			else if (opcode == "hikey")
			{
				currentSettingsState.hikey = keyStringToInt(value);
			}
			else if (opcode == "pitch_keycenter")
			{
				currentSettingsState.pitch_keycenter = keyStringToInt(value);
			}
			else if (opcode == "lovel")
			{
				currentSettingsState.lovel = value.toInt();
			}
			else if (opcode == "hivel")
			{
				currentSettingsState.hivel = value.toInt();
			}
			else if (opcode == "loop_mode")
			{
				if (value == "loop_continuous") { currentSettingsState.loop_mode = LoopMode::LoopContinuous; }
				else { qDebug() << "Oops loop val"; }
			}
			else if (opcode == "ampeg_hold")
			{
				currentSettingsState.ampeg_hold = value.toFloat();
			}
			else if (opcode == "ampeg_sustain")
			{
				currentSettingsState.ampeg_sustain = value.toFloat();
			}
			else if (opcode == "ampeg_decay")
			{
				currentSettingsState.ampeg_decay = value.toFloat();
			}
			else if (opcode == "ampeg_release")
			{
				currentSettingsState.ampeg_release = value.toFloat();
			}
			else
			{
				qDebug() << "AAAAAAAA uknown opcode" << opcode << value;
			}
		}

	}

	qDebug() << "Yay!" << m_sfz.m_groups.size();
	for (auto& group : m_sfz.m_groups)
	{
		qDebug() << "wooo" << group.m_regions.size();
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
