/*
 * Tuner.cpp - determine the fundamental frequency of audio signals
 *
 * Copyright (c) 2022 saker <sakertooth@gmail.com>
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

#include "Tuner.h"

#include <cmath>

#include "embed.h"
#include "plugin_export.h"

extern "C" {

Plugin::Descriptor PLUGIN_EXPORT tuner_plugin_descriptor = {STRINGIFY(PLUGIN_NAME), "Tuner",
	QT_TRANSLATE_NOOP("pluginBrowser", "Determine the fundamental frequency of audio signals"),
	"saker <sakertooth@gmail.com>", 0x0100, Plugin::Effect, new PluginPixmapLoader("logo"), NULL, NULL};
};

extern "C" {
Plugin* PLUGIN_EXPORT lmms_plugin_main(Model* parent, void* _data)
{
	return new Tuner(parent, static_cast<const Plugin::Descriptor::SubPluginFeatures::Key*>(_data));
}
}

Tuner::Tuner(Model* parent, const Descriptor::SubPluginFeatures::Key* key)
	: Effect(&tuner_plugin_descriptor, parent, key)
	, m_tunerControls(this)
{
	calculateNoteFrequencies();
}

bool Tuner::processAudioBuffer(sampleFrame* buf, const fpp_t frames)
{
	// TODO
	return false;
}

std::pair<Tuner::Note, float> Tuner::calculateClosestNote(float frequency, float reference)
{
	float cents = 1200 * std::log2(frequency / reference);
	auto closestNote = std::min_element(m_noteFrequencies.begin(), m_noteFrequencies.end(),
		[frequency](auto& a, auto& b) { return std::abs(frequency - a.second) < std::abs(frequency - b.second); });
	return {closestNote->first, cents};
}

void Tuner::calculateNoteFrequencies()
{
	float referenceFrequency = m_tunerControls.m_referenceFreqModel.value();
	for (int i = 0; i < static_cast<int>(Tuner::Note::Count); ++i)
	{
		auto note = static_cast<Tuner::Note>(i);
		float noteFrequency = referenceFrequency * std::exp2(i / 12.0f);

		if (m_noteFrequencies.find(note) != m_noteFrequencies.end()) { m_noteFrequencies[note] = noteFrequency; }
		else
		{
			m_noteFrequencies.emplace(note, noteFrequency);
		}
	}
}

EffectControls* Tuner::controls()
{
	return new TunerControls(this);
}