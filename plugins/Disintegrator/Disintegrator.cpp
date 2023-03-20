/*
 * Disintegrator.cpp
 *
 * Copyright (c) 2019 Lost Robot <r94231@gmail.com>
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

#include "Disintegrator.h"

#include "embed.h"
#include "lmms_math.h"
#include "plugin_export.h"

extern "C"
{

Plugin::Descriptor PLUGIN_EXPORT disintegrator_plugin_descriptor =
{
	STRINGIFY(PLUGIN_NAME),
	"Disintegrator",
	QT_TRANSLATE_NOOP("pluginBrowser", "A delay modulation effect for very aggressive digital distortion."),
	"Lost Robot <r94231@gmail.com>",
	0x0100,
	Plugin::Effect,
	new PluginPixmapLoader("logo"),
	NULL,
	NULL
} ;

}



DisintegratorEffect::DisintegratorEffect(Model* parent, const Descriptor::SubPluginFeatures::Key* key) :
	Effect(&disintegrator_plugin_descriptor, parent, key),
	m_disintegratorControls(this),
	m_needsUpdate(true)
{
	sampleRateChanged();
}



void DisintegratorEffect::sampleRateChanged()
{
	m_sampleRate = Engine::mixer()->processingSampleRate();
	m_2PiOverSR =  F_2PI / m_sampleRate;
	m_lp = new BasicFilters<2>(Engine::mixer()->processingSampleRate());
	m_hp = new BasicFilters<2>(Engine::mixer()->processingSampleRate());
	m_lp->setFilterType(0);
	m_hp->setFilterType(1);
	m_needsUpdate = true;

	m_bufferSize = m_sampleRate * 0.01f + 1.f;
	for (int i = 0; i < 2; ++i)
	{
		m_inBuf[i].resize(m_bufferSize);
	}
}



bool DisintegratorEffect::processAudioBuffer(sampleFrame* buf, const fpp_t frames)
{
	if (!isEnabled() || !isRunning ())
	{
		return false;
	}

	double outSum = 0.0;
	const float d = dryLevel();
	const float w = wetLevel();
	
	const ValueBuffer * amountBuf = m_disintegratorControls.m_amountModel.valueBuffer();
	const ValueBuffer * typeBuf = m_disintegratorControls.m_typeModel.valueBuffer();
	const ValueBuffer * freqBuf = m_disintegratorControls.m_lowCutModel.valueBuffer();

	// Update filters
	if(m_needsUpdate || m_disintegratorControls.m_highCutModel.isValueChanged())
	{
		m_lp->calcFilterCoeffs(m_disintegratorControls.m_highCutModel.value(), 0.5);
		
	}
	if(m_needsUpdate || m_disintegratorControls.m_lowCutModel.isValueChanged())
	{
		m_hp->calcFilterCoeffs(m_disintegratorControls.m_lowCutModel.value(), 0.5);
	}
	m_needsUpdate = false;

	for (fpp_t f = 0; f < frames; ++f)
	{
		const float amount = (amountBuf ? amountBuf->value(f) : m_disintegratorControls.m_amountModel.value()) * 0.001f * m_sampleRate;
		const int type = typeBuf ? typeBuf->value(f) : m_disintegratorControls.m_typeModel.value();
		const float freq = freqBuf ? freqBuf->value(f) : m_disintegratorControls.m_freqModel.value();
	
		sample_t s[2] = {buf[f][0], buf[f][1]};

		// Increment buffer read point
		++m_inBufLoc;
		if (m_inBufLoc >= m_bufferSize)
		{
			m_inBufLoc = 0;
		}

		// Write dry input to buffer
		m_inBuf[0][m_inBufLoc] = s[0];
		m_inBuf[1][m_inBufLoc] = s[1];

		float newInBufLoc[2] = {0, 0};
		float newInBufLocFrac[2] = {0, 0};

		// Generate white noise or sine wave, apply filters, subtract the
		// result from the buffer read point and store in a variable.
		sampleFrame delayModInput = {0, 0};
		switch (type)
		{
			case 0://Mono Noise
			{
				delayModInput[0] = fast_rand() / (float)FAST_RAND_MAX * 2.f - 1.f;
				delayModInput[1] = delayModInput[0];
				break;
			}
			case 1://Stereo Noise
			{
				delayModInput[0] = fast_rand() / (float)FAST_RAND_MAX * 2.f - 1.f;
				delayModInput[1] = fast_rand() / (float)FAST_RAND_MAX * 2.f - 1.f;
				break;
			}
			case 2://Sine Wave
			{
				m_sineLoc = fmod(m_sineLoc + (freq * m_2PiOverSR), F_2PI);
				delayModInput[0] = sin(m_sineLoc);
				delayModInput[1] = delayModInput[0];
				break;
			}
			case 3:// Self-Modulation
			{
				delayModInput[0] = qBound(-1.f, s[0], 1.f);
				delayModInput[1] = qBound(-1.f, s[1], 1.f);
				break;
			}
		}

		for (int i = 0; i < 2; ++i)
		{
			newInBufLoc[i] = delayModInput[i];

			if (type != 2)// Sine mode doesn't use filters
			{
				newInBufLoc[i] = m_hp->update(newInBufLoc[i], i);
				newInBufLoc[i] = m_lp->update(newInBufLoc[i], i);
			}

			newInBufLoc[i] = (newInBufLoc[i] + 1) * 0.5f;

			newInBufLoc[i] = realfmod(m_inBufLoc - newInBufLoc[i] * amount, m_bufferSize);

			// Distance between samples
			newInBufLocFrac[i] = fmod(newInBufLoc[i], 1);
		}

		

		for (int i = 0; i < 2; ++i)
		{
			if (newInBufLocFrac[i] == 0)
			{
				s[i] = m_inBuf[i][newInBufLoc[i]];
			}
			else
			{
				if (newInBufLoc[i] < m_bufferSize - 1)
				{
					s[i] = m_inBuf[i][floor(newInBufLoc[i])] * (1 - newInBufLocFrac[i]) + m_inBuf[i][ceil(newInBufLoc[i])] * newInBufLocFrac[i];
				}
				else// For when the interpolation wraps around to the beginning of the buffer
				{
					s[i] = m_inBuf[i][m_bufferSize - 1] * (1 - newInBufLocFrac[i]) + m_inBuf[i][0] * newInBufLocFrac[i];
				}
			}
		}

		buf[f][0] = d * buf[f][0] + w * s[0];
		buf[f][1] = d * buf[f][1] + w * s[1];

		outSum += buf[f][0] + buf[f][1];
	}

	checkGate(outSum / frames);

	return isRunning();
}



// Handles negative values properly, unlike fmod.
inline float DisintegratorEffect::realfmod(float k, float n)
{
	float r = fmod(k, n);
	return r < 0 ? r + n : r;
}



void DisintegratorEffect::clearFilterHistories()
{
	m_lp->clearHistory();
	m_hp->clearHistory();
}



extern "C"
{

// necessary for getting instance out of shared lib
PLUGIN_EXPORT Plugin * lmms_plugin_main(Model* parent, void* data)
{
	return new DisintegratorEffect(parent, static_cast<const Plugin::Descriptor::SubPluginFeatures::Key *>(data));
}

}

