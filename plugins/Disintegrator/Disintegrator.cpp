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
	QT_TRANSLATE_NOOP("pluginBrowser", "A delay modulation effect for creating very aggressive digital distortion."),
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
	m_disintegratorControls(this)
{
	// Fill buffer with DISINTEGRATOR_BUFFER_SIZE number of samples
	for (int i = 0; i < DISINTEGRATOR_BUFFER_SIZE; ++i)
	{
		m_inBuf[i].reserve(DISINTEGRATOR_BUFFER_SIZE);
		for (int j = 0; j < 2; ++j)
		{
			m_inBuf[j].push_back(0);
		}
	}
}




DisintegratorEffect::~DisintegratorEffect()
{
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
	
	const ValueBuffer * lowCutBuf = m_disintegratorControls.m_lowCutModel.valueBuffer();
	const ValueBuffer * highCutBuf = m_disintegratorControls.m_highCutModel.valueBuffer();
	const ValueBuffer * amountBuf = m_disintegratorControls.m_amountModel.valueBuffer();
	const ValueBuffer * typeBuf = m_disintegratorControls.m_typeModel.valueBuffer();
	const ValueBuffer * freqBuf = m_disintegratorControls.m_lowCutModel.valueBuffer();

	sample_rate_t sampleRate = Engine::mixer()->processingSampleRate();

	for (fpp_t f = 0; f < frames; ++f)
	{
		const float lowCut = lowCutBuf ? lowCutBuf->value(f) : m_disintegratorControls.m_lowCutModel.value();
		const float highCut = highCutBuf ? highCutBuf->value(f) : m_disintegratorControls.m_highCutModel.value();
		const float amount = amountBuf ? amountBuf->value(f) : m_disintegratorControls.m_amountModel.value();
		const int type = typeBuf ? typeBuf->value(f) : m_disintegratorControls.m_typeModel.value();
		const float freq = freqBuf ? freqBuf->value(f) : m_disintegratorControls.m_freqModel.value();

		outSum += buf[f][0]*buf[f][0] + buf[f][1]*buf[f][1];
	
		sample_t s[2] = {buf[f][0], buf[f][1]};

		// Increment buffer read point
		++m_inBufLoc;
		if (m_inBufLoc >= DISINTEGRATOR_BUFFER_SIZE)
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
		switch (type)
		{
			case 0:// Mono Noise
			{
				newInBufLoc[0] = fast_rand() / (float)FAST_RAND_MAX;

				calcHighpassFilter(newInBufLoc[0], newInBufLoc[0], 0, lowCut, sampleRate);
				calcLowpassFilter(newInBufLoc[0], newInBufLoc[0], 0, highCut, sampleRate);

				newInBufLoc[0] = realfmod(m_inBufLoc - newInBufLoc[0] * amount, DISINTEGRATOR_BUFFER_SIZE);
				newInBufLoc[1] = newInBufLoc[0];

				// Distance between samples
				newInBufLocFrac[0] = fmod(newInBufLoc[0], 1);
				newInBufLocFrac[1] = newInBufLocFrac[0];

				break;
			}
			case 1:// Stereo Noise
			{
				newInBufLoc[0] = fast_rand() / (float)FAST_RAND_MAX;
				newInBufLoc[1] = fast_rand() / (float)FAST_RAND_MAX;

				calcHighpassFilter(newInBufLoc[0], newInBufLoc[0], 0, lowCut, sampleRate);
				calcHighpassFilter(newInBufLoc[1], newInBufLoc[1], 1, lowCut, sampleRate);
				calcLowpassFilter(newInBufLoc[0], newInBufLoc[0], 0, highCut, sampleRate);
				calcLowpassFilter(newInBufLoc[1], newInBufLoc[1], 1, highCut, sampleRate);

				newInBufLoc[0] = realfmod(m_inBufLoc - newInBufLoc[0] * amount, DISINTEGRATOR_BUFFER_SIZE);
				newInBufLoc[1] = realfmod(m_inBufLoc - newInBufLoc[1] * amount, DISINTEGRATOR_BUFFER_SIZE);

				// Distance between samples
				newInBufLocFrac[0] = fmod(newInBufLoc[0], 1);
				newInBufLocFrac[1] = fmod(newInBufLoc[1], 1);

				break;
			}
			case 2:// Sine Wave
			{
				m_sineLoc = fmod(m_sineLoc + (freq / (float)sampleRate * F_2PI), F_2PI);

				newInBufLoc[0] = (sin(m_sineLoc) + 1) * 0.5f;

				newInBufLoc[0] = realfmod(m_inBufLoc - newInBufLoc[0] * amount, DISINTEGRATOR_BUFFER_SIZE);
				newInBufLoc[1] = newInBufLoc[0];

				// Distance between samples
				newInBufLocFrac[0] = fmod(newInBufLoc[0], 1);
				newInBufLocFrac[1] = newInBufLocFrac[0];

				break;
			}
		}

		for (int i = 0; i < 2; ++i)
		{
			if (fmod(newInBufLoc[i], 1) == 0)
			{
				s[i] = m_inBuf[i][newInBufLoc[i]];
			}
			else
			{
				if (newInBufLoc[i] < DISINTEGRATOR_BUFFER_SIZE - 1)
				{
					s[i] = m_inBuf[i][floor(newInBufLoc[i])] * (1 - newInBufLocFrac[i]) + m_inBuf[i][ceil(newInBufLoc[i])] * newInBufLocFrac[i];
				}
				else// For when the interpolation wraps around to the beginning of the buffer
				{
					s[i] = m_inBuf[i][DISINTEGRATOR_BUFFER_SIZE - 1] * (1 - newInBufLocFrac[i]) + m_inBuf[i][0] * newInBufLocFrac[i];
				}
			}
		}

		buf[f][0] = d * buf[f][0] + w * s[0];
		buf[f][1] = d * buf[f][1] + w * s[1];
	}

	checkGate(outSum / frames);

	return isRunning();
}



inline void DisintegratorEffect::calcLowpassFilter(sample_t &outSamp, sample_t inSamp, int channel, float lpCutoff, sample_rate_t sampleRate)
{
	// "if" statement is here so the filter coefficients only need to be calculated when they change
	if (m_prevLPCutoff[channel] != lpCutoff)
	{
		m_prevLPCutoff[channel] = lpCutoff;
		const float w0 = D_2PI * lpCutoff / sampleRate;
		const float tempCosW0 = cos(w0);
		const float alpha = sin(w0) / 0.3535f;

		m_LPa0 = 1 + alpha;
		m_LPb0 = (1 - tempCosW0) * 0.5f;
		m_LPb1 = 1 - tempCosW0;
		m_LPa1 = -(2 * tempCosW0);
		m_LPa2 = 1 - alpha;
	}

	m_filtLPY[channel][0] = (m_LPb0 * (inSamp + m_filtLPX[channel][2]) + m_LPb1 * m_filtLPX[channel][1] -
				m_LPa1 * m_filtLPY[channel][1] - m_LPa2 * m_filtLPY[channel][2]) / m_LPa0;

	m_filtLPX[channel][2] = m_filtLPX[channel][1];
	m_filtLPX[channel][1] = inSamp;
	m_filtLPY[channel][2] = m_filtLPY[channel][1];
	m_filtLPY[channel][1] = m_filtLPY[channel][0];

	outSamp = m_filtLPY[channel][0];
}



inline void DisintegratorEffect::calcHighpassFilter(sample_t &outSamp, sample_t inSamp, int channel, float hpCutoff, sample_rate_t sampleRate)
{
	// "if" statement is here so the filter coefficients only need to be calculated when they change
	if (m_prevHPCutoff[channel] != hpCutoff)
	{
		m_prevHPCutoff[channel] = hpCutoff;
		const float w0 = D_2PI * hpCutoff / sampleRate;
		const float tempCosW0 = cos(w0);
		const float alpha = sin(w0) / 0.3535f;

		m_HPa0 = 1 + alpha;
		m_HPb0 = (1 + tempCosW0) * 0.5f;
		m_HPb1 = -(1 + tempCosW0);
		m_HPa1 = -2 * tempCosW0;
		m_HPa2 = 1 - alpha;
	}

	m_filtHPY[channel][0] = (m_HPb0 * (inSamp + m_filtHPX[channel][2]) + m_HPb1 * m_filtHPX[channel][1] -
				m_HPa1 * m_filtHPY[channel][1] - m_HPa2 * m_filtHPY[channel][2]) / m_HPa0;

	m_filtHPX[channel][2] = m_filtHPX[channel][1];
	m_filtHPX[channel][1] = inSamp;
	m_filtHPY[channel][2] = m_filtHPY[channel][1];
	m_filtHPY[channel][1] = m_filtHPY[channel][0];

	outSamp = m_filtHPY[channel][0];
}



// Handles negative values properly, unlike fmod.
inline float DisintegratorEffect::realfmod(float k, float n)
{
	float r = fmod(k, n);
	return r < 0 ? r + n : r;
}



extern "C"
{

// necessary for getting instance out of shared lib
PLUGIN_EXPORT Plugin * lmms_plugin_main(Model* parent, void* data)
{
	return new DisintegratorEffect(parent, static_cast<const Plugin::Descriptor::SubPluginFeatures::Key *>(data));
}

}

