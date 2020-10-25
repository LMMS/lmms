/*
 * StereoControl.cpp
 *
 * Copyright (c) 2020 Lost Robot <r94231@gmail.com>
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

#include "StereoControl.h"

#include "embed.h"
#include "interpolation.h"
#include "lmms_math.h"
#include "plugin_export.h"

extern "C"
{

Plugin::Descriptor PLUGIN_EXPORT stereocontrol_plugin_descriptor =
{
	STRINGIFY(PLUGIN_NAME),
	"Stereo Control",
	QT_TRANSLATE_NOOP("pluginBrowser", "General utility and stereo processing, including multiple types of panning and stereo enhancement."),
	"Lost Robot <r94231@gmail.com>",
	0x0100,
	Plugin::Effect,
	new PluginPixmapLoader("logo"),
	NULL,
	NULL
};

}



StereoControlEffect::StereoControlEffect(Model* parent, const Descriptor::SubPluginFeatures::Key* key) :
	Effect(&stereocontrol_plugin_descriptor, parent, key),
	m_stereocontrolControls(this)
{
	m_sampleRate = Engine::mixer()->processingSampleRate();

	m_delayBufSize = int(m_sampleRate / 160.f) + 1.f;
	m_delayBuf[0].resize(m_delayBufSize);
	m_delayBuf[1].resize(m_delayBufSize);

	m_haasBufSize = int(m_sampleRate * 0.002f) + 1.f;
	m_haasBuf[0].resize(m_haasBufSize);
	m_haasBuf[1].resize(m_haasBufSize);
}




StereoControlEffect::~StereoControlEffect()
{
}




bool StereoControlEffect::processAudioBuffer(sampleFrame* buf, const fpp_t frames)
{
	if(!isEnabled() || !isRunning ())
	{
		return false;
	}

	double outSum = 0.0;
	const float d = dryLevel();
	const float w = wetLevel();
	
	const ValueBuffer * gainBuf = m_stereocontrolControls.m_gainModel.valueBuffer();
	const ValueBuffer * stereoizerBuf = m_stereocontrolControls.m_stereoizerModel.valueBuffer();
	const ValueBuffer * widthBuf = m_stereocontrolControls.m_widthModel.valueBuffer();
	const ValueBuffer * panModeBuf = m_stereocontrolControls.m_panModeModel.valueBuffer();
	const ValueBuffer * panBuf = m_stereocontrolControls.m_panModel.valueBuffer();
	const ValueBuffer * monoBuf = m_stereocontrolControls.m_monoModel.valueBuffer();
	const ValueBuffer * dcBuf = m_stereocontrolControls.m_dcModel.valueBuffer();
	const ValueBuffer * muteBuf = m_stereocontrolControls.m_muteModel.valueBuffer();
	const ValueBuffer * monoBassBuf = m_stereocontrolControls.m_monoBassModel.valueBuffer();
	const ValueBuffer * auditionBuf = m_stereocontrolControls.m_auditionModel.valueBuffer();
	const ValueBuffer * invertLBuf = m_stereocontrolControls.m_invertLModel.valueBuffer();
	const ValueBuffer * invertRBuf = m_stereocontrolControls.m_invertRModel.valueBuffer();
	const ValueBuffer * soloChannelBuf = m_stereocontrolControls.m_soloChannelModel.valueBuffer();
	const ValueBuffer * monoBassFreqBuf = m_stereocontrolControls.m_monoBassFreqModel.valueBuffer();
	const ValueBuffer * stereoizerLPBuf = m_stereocontrolControls.m_stereoizerLPModel.valueBuffer();
	const ValueBuffer * stereoizerHPBuf = m_stereocontrolControls.m_stereoizerHPModel.valueBuffer();
	const ValueBuffer * panSpectralBuf = m_stereocontrolControls.m_panSpectralModel.valueBuffer();
	const ValueBuffer * panDelayBuf = m_stereocontrolControls.m_panDelayModel.valueBuffer();
	const ValueBuffer * panDualLBuf = m_stereocontrolControls.m_panDualLModel.valueBuffer();
	const ValueBuffer * panDualRBuf = m_stereocontrolControls.m_panDualRModel.valueBuffer();

	for(fpp_t f = 0; f < frames; ++f)
	{
		const float gain = dbfsToAmp(gainBuf ? gainBuf->value(f) : m_stereocontrolControls.m_gainModel.value());
		const float stereoizer = (stereoizerBuf ? stereoizerBuf->value(f) : m_stereocontrolControls.m_stereoizerModel.value()) * 0.01f;
		const float width = (widthBuf ? widthBuf->value(f) : m_stereocontrolControls.m_widthModel.value()) * 0.01f;
		const int panMode = panModeBuf ? panModeBuf->value(f) : m_stereocontrolControls.m_panModeModel.value();
		const float pan = (panBuf ? panBuf->value(f) : m_stereocontrolControls.m_panModel.value()) * 0.01f;
		const bool mono = monoBuf ? monoBuf->value(f) : m_stereocontrolControls.m_monoModel.value();
		const bool dc = dcBuf ? dcBuf->value(f) : m_stereocontrolControls.m_dcModel.value();
		const bool mute = muteBuf ? muteBuf->value(f) : m_stereocontrolControls.m_muteModel.value();
		const bool monoBass = monoBassBuf ? monoBassBuf->value(f) : m_stereocontrolControls.m_monoBassModel.value();
		const bool audition = auditionBuf ? auditionBuf->value(f) : m_stereocontrolControls.m_auditionModel.value();
		const bool invertL = invertLBuf ? invertLBuf->value(f) : m_stereocontrolControls.m_invertLModel.value();
		const bool invertR = invertRBuf ? invertRBuf->value(f) : m_stereocontrolControls.m_invertRModel.value();
		const int soloChannel = soloChannelBuf ? soloChannelBuf->value(f) : m_stereocontrolControls.m_soloChannelModel.value();
		const float monoBassFreq = monoBassFreqBuf ? monoBassFreqBuf->value(f) : m_stereocontrolControls.m_monoBassFreqModel.value();
		const float stereoizerLP = stereoizerLPBuf ? stereoizerLPBuf->value(f) : m_stereocontrolControls.m_stereoizerLPModel.value();
		const float stereoizerHP = stereoizerHPBuf ? stereoizerHPBuf->value(f) : m_stereocontrolControls.m_stereoizerHPModel.value();
		const float panSpectral = (panSpectralBuf ? panSpectralBuf->value(f) : m_stereocontrolControls.m_panSpectralModel.value()) * 0.01f;
		const float panDelay = (panDelayBuf ? panDelayBuf->value(f) : m_stereocontrolControls.m_panDelayModel.value()) * 0.01f;
		const float panDualL = (panDualLBuf ? panDualLBuf->value(f) : m_stereocontrolControls.m_panDualLModel.value()) * 0.01f;
		const float panDualR = (panDualRBuf ? panDualRBuf->value(f) : m_stereocontrolControls.m_panDualRModel.value()) * 0.01f;

		sample_t s[2] = { buf[f][0], buf[f][1] };

		switch (soloChannel)
		{
			case 0:
			{
				break;
			}
			case 1:
			{
				s[1] = s[0];
				break;
			}
			case 2:
			{
				s[0] = s[1];
				break;
			}
		}

		if (invertL)
		{
			s[0] *= -1.f;
		}

		if (invertR)
		{
			s[1] *= -1.f;
		}

		// VVV Stereoizer VVV

		/*
		The Stereoizer delays the audio slightly, inverts one channel of the delayed signal, and mixes that with the dry signal.
		This results in an extremely wide result, even with a mono input.
		Also, when collapsed to mono, any previously-mid part of that delayed signal will be cancelled out, so it's very mono-compatible.

		Of course, delaying audio and mixing it with the dry signal creates a comb filter, which is undesirable.  With certain inputs and delay
		amounts, it'll even result in silence.  To help reduce this effect, the delayed signal is sent through some allpass filters, which makes
		the delay vary depending on frequency, so the comb filtering is significantly less obvious.
		*/

		float allpassFreq = 0;
		float delayLength = 0;
		allpassFreq = linearInterpolate(370.f, 170.f, stereoizer * 0.5f);// Arbitrary
		delayLength = m_sampleRate / allpassFreq;

		// Filter coefficient calculation
		const float w0 = (F_2PI / m_sampleRate) * allpassFreq;
		const float a0 = 1 + (sin(w0) / (0.707f * 2.f));
		m_filtA = (1 - (a0 - 1)) / a0;
		m_filtB = (-2*cos(w0)) / a0;
		
		float delayResult[2] = {0};
		for (int i = 0; i < 2; ++i)
		{
			// Read next value from delay buffer
			float readLoc = m_delayIndex[i] - delayLength;
			if (readLoc < 0) {readLoc += m_delayBufSize;}
			float readLocFrac = fraction(readLoc);
			if (readLoc < m_delayBufSize - 1)
			{
				delayResult[i] = linearInterpolate(m_delayBuf[i][floor(readLoc)], m_delayBuf[i][ceil(readLoc)], readLocFrac);
			}
			else// For when the interpolation wraps around to the beginning of the buffer
			{
				delayResult[i] = linearInterpolate(m_delayBuf[i][m_delayBufSize - 1], m_delayBuf[i][0], readLocFrac);
			}

			// Increment delay ring buffer index
			++m_delayIndex[i];
			if (m_delayIndex[i] >= m_delayBufSize)
			{
				m_delayIndex[i] -= m_delayBufSize;
			}
			// Send new value to delay line
			m_delayBuf[i][m_delayIndex[i]] = s[i];
		}

		for (int i = 0; i < 2; ++i)
		{
			float allpassValue = (delayResult[0] + delayResult[1]) * 0.5f;

			for (int j = 0; j < 2; ++j)
			{
				allpassValue = calcAllpassFilter(allpassValue, m_sampleRate, j, i, m_filtA, m_filtB);
			}

			allpassValue *= i ? -1.f : 1.f;

			float lp;
			float hp;
			float hp2;
			multimodeFilter(allpassValue, multimodeCoeff(stereoizerHP), lp, hp, m_stereoizerFilter[0][i]);
			multimodeFilter(hp, multimodeCoeff(stereoizerLP), lp, hp2, m_stereoizerFilter[1][i]);

			s[i] += lp * qMin(stereoizer, 1.f);
		}
		// ^^^ Stereoizer ^^^

		const float sMid = (s[0] + s[1]) * 0.5f;
		const float sSide = (s[0] - s[1]) * 0.5f;

		s[0] = sMid + sSide * width;
		s[1] = sMid - sSide * width;

		if (mono)
		{
			s[0] = (s[0] + s[1]) * 0.5f;
			s[1] = s[0];
		}

		if (monoBass)
		{
			float lp[2];
			float hp[2];

			for (int i = 0; i < 2; ++i)
			{
				// We apply two filters because a single filter doesn't have a strong enough slope
				multimodeFilter(s[i], multimodeCoeff(monoBassFreq), lp[i], hp[i], m_monoBassFilter[i][0]);
				multimodeFilter(s[i], multimodeCoeff(monoBassFreq), lp[i], hp[i], m_monoBassFilter[i][1]);
			}

			float lowmono = (lp[0] + lp[1]) * 0.5f;

			if (audition)
			{
				s[0] = lowmono;
				s[1] = lowmono;
			}
			else
			{
				s[0] = lowmono - hp[0];// Subtraction is needed due to filter phase shift
				s[1] = lowmono - hp[1];
			}
		}

		switch (panMode)
		{
			case 0:// Gain
			{
				const float lGain = pan > 0 ? 1.f - pan : 1.f;
				const float rGain = pan < 0 ? 1.f + pan : 1.f;
				s[0] *= lGain;
				s[1] *= rGain;
				break;
			}
			case 1:// Stereo
			{
				const float lGainL = panDualL > 0 ? 1.f - panDualL : 1.f;
				const float rGainL = panDualL < 0 ? 1.f + panDualL : 1.f;
				const float lGainR = panDualR > 0 ? 1.f - panDualR : 1.f;
				const float rGainR = panDualR < 0 ? 1.f + panDualR : 1.f;
				const float temp = s[0];
				s[0] = s[0] * lGainL + s[1] * lGainR;
				s[1] = temp * rGainL + s[1] * rGainR;
				break;
			}
			case 2:// Haas
			{
				float haasResult = 0;
				float haasDelayVal[2] = {0};

				if (pan >= 0)
				{
					haasDelayVal[0] = linearInterpolate(0, 0.0008f * m_sampleRate * panDelay, abs(pan));
				}
				else
				{
					haasDelayVal[1] = linearInterpolate(0, 0.0008f * m_sampleRate * panDelay, abs(pan));
				}

				for (int i = 0; i < 2; ++i)
				{
					if ((i == 0 && pan > 0) || (i == 1 && pan < 0))
					{
						// Read next value from delay buffer
						float readLoc = m_haasIndex[i] - haasDelayVal[i];
						if (readLoc < 0) {readLoc += m_haasBufSize;}
						float readLocFrac = fraction(readLoc);
						if (readLoc < m_haasBufSize - 1)
						{
							haasResult = linearInterpolate(m_haasBuf[i][floor(readLoc)], m_haasBuf[i][ceil(readLoc)], readLocFrac);
						}
						else// For when the interpolation wraps around to the beginning of the buffer
						{
							haasResult = linearInterpolate(m_haasBuf[i][m_haasBufSize - 1], m_haasBuf[i][0], readLocFrac);
						}
					}

					// Increment delay ring buffer index
					++m_haasIndex[i];
					if (m_haasIndex[i] >= m_haasBufSize)
					{
						m_haasIndex[i] -= m_haasBufSize;
					}
					// Send new value to delay line
					m_haasBuf[i][m_haasIndex[i]] = s[i];
				}

				if (pan != 0)
				{
					const float minGain = linearInterpolate(1.f, 0.15f, abs(pan));
					if (pan >= 0)
					{
						const float lGain = ((1.f - pan) + minGain) / (1.f + minGain);
						s[0] = haasResult * lGain;
					}
					else
					{
						const float rGain = ((1.f + pan) + minGain) / (1.f + minGain);
						s[1] = haasResult * rGain;
					}
				}

				const int lowChnl = (pan <= 0);
				const int highChnl = !lowChnl;

				float lp;
				float hp;

				// Filter parameters are semi-arbitrary, but are vaguely based on some random graphs I saw during research
				float temp = -abs(pan) + 1.f;
				float filtFreq = 2000.f + 8000.f * temp * temp;
				float hpGain = abs(pan) < 0.5 ? 1.f - abs(pan) * 2.f : 0.f;
				hpGain = linearInterpolate(1.f, hpGain, panSpectral);
				multimodeFilter(s[lowChnl], multimodeCoeff(filtFreq), lp, hp, m_haasSpectralPanFilter[lowChnl]);

				s[lowChnl] = lp + hp * hpGain;

				filtFreq = 2000.f;
				hpGain = abs(pan) + 1.f;
				hpGain = linearInterpolate(1.f, hpGain, panSpectral);
				multimodeFilter(s[highChnl], multimodeCoeff(filtFreq), lp, hp, m_haasSpectralPanFilter[highChnl]);

				s[highChnl] = lp + hp * hpGain;
			}
		}

		if (dc)
		{
			for (int i = 0; i < 2; ++i)
			{
				m_dcInfo[i] = m_dcInfo[i] * 0.999f + s[i] * 0.001f;
				s[i] -= m_dcInfo[i];
			}
		}

		if (mute)
		{
			s[0] = 0;
			s[1] = 0;
		}

		s[0] *= gain;
		s[1] *= gain;

		buf[f][0] = d * buf[f][0] + w * s[0];
		buf[f][1] = d * buf[f][1] + w * s[1];

		outSum += buf[f][0] * buf[f][0] + buf[f][1] * buf[f][1];
	}

	checkGate(outSum / frames);

	return isRunning();
}


sample_t StereoControlEffect::calcAllpassFilter(sample_t inSamp, sample_rate_t Fs, int filtNum, int channel, float a, float b)
{
	float filterOutput = a * (inSamp - m_filtY[filtNum][channel][1]) +
		b * (m_filtX[filtNum][channel][0] - m_filtY[filtNum][channel][0]) +
		m_filtX[filtNum][channel][1];

	m_filtX[filtNum][channel][1] = m_filtX[filtNum][channel][0];
	m_filtX[filtNum][channel][0] = inSamp;
	m_filtY[filtNum][channel][1] = m_filtY[filtNum][channel][0];
	m_filtY[filtNum][channel][0] = filterOutput;

	return filterOutput;
}


void StereoControlEffect::multimodeFilter(sample_t in, float g, sample_t &lp, sample_t &hp, sample_t &filtBuf)
{
	lp = in * g + filtBuf;
	hp = in - lp;
	filtBuf = hp * g * 2.f + filtBuf;
}

float StereoControlEffect::multimodeCoeff(float freq)
{
	const float g = tan(F_PI * freq / m_sampleRate);
	return g / (1.f + g);
}




extern "C"
{

// necessary for getting instance out of shared lib
PLUGIN_EXPORT Plugin * lmms_plugin_main(Model* parent, void* data)
{
	return new StereoControlEffect(parent, static_cast<const Plugin::Descriptor::SubPluginFeatures::Key *>(data));
}

}

