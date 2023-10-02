/*
 * Tuner.cpp - estimate the pitch of an audio signal
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

#include "ArrayVector.h"
#include "embed.h"
#include "plugin_export.h"

namespace lmms {

extern "C" {

Plugin::Descriptor PLUGIN_EXPORT tuner_plugin_descriptor
	= {LMMS_STRINGIFY(PLUGIN_NAME), "Tuner", QT_TRANSLATE_NOOP("pluginBrowser", "Estimate the pitch of audio signals"),
		"saker <sakertooth@gmail.com>", 0x0100, Plugin::Type::Effect, new PluginPixmapLoader("logo"), NULL, NULL};
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
	, m_aubioPitch(new_aubio_pitch("default", WINDOW_SIZE, HOP_SIZE, Engine::audioEngine()->processingSampleRate()))
	, m_numAubioInputFrames(0)
{
}

Tuner::~Tuner()
{
	del_aubio_pitch(m_aubioPitch);
}

auto Tuner::processAudioBuffer(sampleFrame* buf, const fpp_t frames) -> bool
{
	if (!isEnabled() || !isRunning()) { return false; }

	auto outSum = 0.0f;
	auto monoSignal = ArrayVector<float, DEFAULT_BUFFER_SIZE>(frames);
	for (size_t frame = 0; frame < frames; ++frame)
	{
		monoSignal[frame] = std::accumulate(buf[frame].begin(), buf[frame].end(), 0.0f) / DEFAULT_CHANNELS;
		outSum += buf[frame][0] * buf[frame][0] + buf[frame][1] * buf[frame][1];
	}

	if (typeInfo<float>::isEqual(outSum, 0.0f))
	{
		checkGate(outSum / frames);
		return false;
	}

	if (monoSignal.size() > HOP_SIZE)
	{
		for (int hop = 0; hop < monoSignal.size() / HOP_SIZE; ++hop)
		{
			detectPitch(monoSignal.begin() + hop * HOP_SIZE, HOP_SIZE);
		}
	}
	else if (m_numAubioInputFrames + monoSignal.size() <= HOP_SIZE)
	{
		std::copy(monoSignal.begin(), monoSignal.end(), m_aubioInput.begin() + m_numAubioInputFrames);
		m_numAubioInputFrames += monoSignal.size();

		if (m_numAubioInputFrames == HOP_SIZE)
		{
			detectPitch(m_aubioInput.data(), m_aubioInput.size());
			m_numAubioInputFrames = 0;
		}
	}
	else if (monoSignal.size() == HOP_SIZE) { detectPitch(monoSignal.data(), monoSignal.size()); }

	checkGate(outSum / frames);
	return isRunning();
}

auto Tuner::detectPitch(float* data, size_t size) -> float
{
	float pitchOut = 0.0f;

	auto in = fvec_t{static_cast<uint_t>(size), data};
	auto out = fvec_t{1, &pitchOut};

	aubio_pitch_do(m_aubioPitch, &in, &out);
	emit frequencyCalculated(pitchOut);

	return pitchOut;
}

auto Tuner::controls() -> EffectControls*
{
	return &m_tunerControls;
}
} // namespace lmms
