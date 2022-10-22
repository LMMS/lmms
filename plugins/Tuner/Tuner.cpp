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
#include <iostream>

#include "embed.h"
#include "plugin_export.h"

extern "C" {

Plugin::Descriptor PLUGIN_EXPORT tuner_plugin_descriptor
	= {STRINGIFY(PLUGIN_NAME), "Tuner", QT_TRANSLATE_NOOP("pluginBrowser", "Estimate the pitch of audio signals"),
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
	, m_referenceFrequency(440.0f)
	, m_aubioPitch(new_aubio_pitch("default", m_windowSize, m_hopSize, Engine::audioEngine()->processingSampleRate()))
	, m_inputBuffer(new_fvec(m_hopSize))
	, m_outputBuffer(new_fvec(1))
{
}

Tuner::~Tuner()
{
	del_fvec(m_inputBuffer);
	del_fvec(m_outputBuffer);
	del_aubio_pitch(m_aubioPitch);
}

bool Tuner::processAudioBuffer(sampleFrame* buf, const fpp_t frames)
{
	if (!isEnabled() || !isRunning()) { return false; }

	float outSum = 0.0f;
	for (fpp_t f = 0; f < frames; ++f)
	{
		outSum += buf[f][0] * buf[f][0] + buf[f][1] * buf[f][1];
	}

	if (outSum > 0.0f) 
	{
		for (int i = 0; i < frames; ++i)
		{
			if (m_samplesCounter % m_hopSize == 0 && m_samplesCounter > 0) 
			{
				aubio_pitch_do(m_aubioPitch, m_inputBuffer, m_outputBuffer);
				emit frequencyCalculated(fvec_get_sample(m_outputBuffer, 0));
				m_samplesCounter = 0;
			}

			fvec_set_sample(m_inputBuffer, (buf[i][0] + buf[i][1]) * 0.5f, m_samplesCounter);
			++m_samplesCounter;
		}
	}

	checkGate(outSum / frames);
	return isRunning();
}

EffectControls* Tuner::controls()
{
	return &m_tunerControls;
}