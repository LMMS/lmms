/*
 * PhaserEffect.cpp
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

#include "PhaserEffect.h"

#include "embed.h"
#include "plugin_export.h"
#include "lmms_math.h"

extern "C"
{

Plugin::Descriptor PLUGIN_EXPORT phaser_plugin_descriptor =
{
	STRINGIFY(PLUGIN_NAME),
	"Phaser",
	QT_TRANSLATE_NOOP("pluginBrowser", "A versatile phaser plugin"),
	"Lost Robot <r94231@gmail.com>",
	0x0100,
	Plugin::Effect,
	new PluginPixmapLoader("logo"),
	NULL,
	NULL
} ;

}


const float PHA_NOISE_FLOOR = 0.00001f;
const double PHA_LOG = 5.0;


PhaserEffect::PhaserEffect(Model* parent, const Descriptor::SubPluginFeatures::Key* key) :
	Effect(&phaser_plugin_descriptor, parent, key),
	m_phaserControls(this)
{
	m_currentPeak[0] = m_currentPeak[1] = PHA_NOISE_FLOOR;

	m_lfo = new QuadratureLfo(Engine::mixer()->processingSampleRate());

	m_rms[0] = new RmsHelper(64 * Engine::mixer()->processingSampleRate() / 44100);
	m_rms[1] = new RmsHelper(64 * Engine::mixer()->processingSampleRate() / 44100);

	m_twoPiOverSr = F_2PI / Engine::mixer()->processingSampleRate();

	calcAttack();
	calcRelease();

	m_outGain = dbfsToAmp(m_phaserControls.m_outGainModel.value());
	m_inGain = dbfsToAmp(m_phaserControls.m_inGainModel.value());

	m_lfo->setFrequency(1.0 / m_phaserControls.m_rateModel.value());

	for (int b = 0; b < 2; ++b)
	{
		m_filtDelayBuf[b].resize(m_phaserControls.m_delayModel.value());
	}
}



PhaserEffect::~PhaserEffect()
{
	if (m_lfo)
	{
		delete m_lfo;
	}

	delete m_rms[0];
	delete m_rms[1];
}



inline void PhaserEffect::calcAttack()
{
	m_attCoeff = exp10((PHA_LOG / (m_phaserControls.m_attackModel.value() * 0.001)) / Engine::mixer()->processingSampleRate());
}

inline void PhaserEffect::calcRelease()
{
	m_relCoeff = exp10((-PHA_LOG / (m_phaserControls.m_releaseModel.value() * 0.001)) / Engine::mixer()->processingSampleRate());
}




bool PhaserEffect::processAudioBuffer(sampleFrame* buf, const fpp_t frames)
{
	if (!isEnabled() || !isRunning ())
	{
		m_currentPeak[0] = m_currentPeak[1] = PHA_NOISE_FLOOR;
		return false;
	}

	sample_rate_t sample_rate =  Engine::mixer()->processingSampleRate();

	double outSum = 0.0;
	const float d = dryLevel();
	const float w = wetLevel();
	
	const ValueBuffer * cutoffBuf = m_phaserControls.m_cutoffModel.valueBuffer();
	const ValueBuffer * resonanceBuf = m_phaserControls.m_resonanceModel.valueBuffer();
	const ValueBuffer * feedbackBuf = m_phaserControls.m_feedbackModel.valueBuffer();
	const ValueBuffer * orderBuf = m_phaserControls.m_orderModel.valueBuffer();
	const ValueBuffer * enableLFOBuf = m_phaserControls.m_enableLFOModel.valueBuffer();
	const ValueBuffer * amountBuf = m_phaserControls.m_amountModel.valueBuffer();
	const ValueBuffer * wetDryBuf = m_phaserControls.m_wetDryModel.valueBuffer();
	const ValueBuffer * inFollowBuf = m_phaserControls.m_inFollowModel.valueBuffer();

	float leftLfo;
	float rightLfo;

	float lOutPeak = 0.0;
	float rOutPeak = 0.0;
	float lInPeak = 0.0;
	float rInPeak = 0.0;

	if (m_phaserControls.m_attackModel.isValueChanged())
	{
		calcAttack();
	}
	if (m_phaserControls.m_releaseModel.isValueChanged())
	{
		calcRelease();
	}
	if (m_phaserControls.m_outGainModel.isValueChanged())
	{
		m_outGain = dbfsToAmp(m_phaserControls.m_outGainModel.value());
	}
	if (m_phaserControls.m_inGainModel.isValueChanged())
	{
		m_inGain = dbfsToAmp(m_phaserControls.m_inGainModel.value());
	}
	if (m_phaserControls.m_delayModel.isValueChanged())
	{
		for (int b = 0; b < 2; ++b)
		{
			m_filtDelayBuf[b].resize(m_phaserControls.m_delayModel.value());
		}
	}
	if (m_phaserControls.m_inFollowModel.isValueChanged() && !m_phaserControls.m_inFollowModel.value())
	{
		m_currentPeak[0] = m_currentPeak[1] = PHA_NOISE_FLOOR;
	}
	if (m_phaserControls.m_rateModel.isValueChanged())
	{
		m_lfo->setFrequency(1.0 / m_phaserControls.m_rateModel.value());
	}

	for (fpp_t f = 0; f < frames; ++f)
	{
		outSum += buf[f][0]*buf[f][0] + buf[f][1]*buf[f][1];

		buf[f][0] *= m_inGain;
		buf[f][1] *= m_inGain;

		sample_t s[2] = { buf[f][0], buf[f][1] };

		const float cutoff = cutoffBuf ? cutoffBuf->value(f) : m_phaserControls.m_cutoffModel.value();
		const float resonance = resonanceBuf ? resonanceBuf->value(f) : m_phaserControls.m_resonanceModel.value();
		const float feedback = feedbackBuf ? feedbackBuf->value(f) : m_phaserControls.m_feedbackModel.value();
		const float order = orderBuf ? orderBuf->value(f) : m_phaserControls.m_orderModel.value();
		const bool enableLFO = enableLFOBuf ? enableLFOBuf->value(f) : m_phaserControls.m_enableLFOModel.value();
		const float amount = amountBuf ? amountBuf->value(f) : m_phaserControls.m_amountModel.value();
		const float wetDry = wetDryBuf ? wetDryBuf->value(f) : m_phaserControls.m_wetDryModel.value();
		const float inFollow = inFollowBuf ? inFollowBuf->value(f) : m_phaserControls.m_inFollowModel.value();

		if (inFollow)
		{
			const double rmsResult[2] = {m_rms[0]->update(s[0]), m_rms[1]->update(s[1])};

			for (int i = 0; i < 2; i++)
			{
				if (rmsResult[i] > m_currentPeak[i])
				{
					m_currentPeak[i] = qMin(m_currentPeak[i] * m_attCoeff, rmsResult[i]);
				}
				else
				if (rmsResult[i] < m_currentPeak[i])
				{
					m_currentPeak[i] = qMax(m_currentPeak[i] * m_relCoeff, rmsResult[i]);
				}

				m_currentPeak[i] = qBound(PHA_NOISE_FLOOR, m_currentPeak[i], 10.0f);
			}
		}

		if (enableLFO)
		{
			m_lfo->tick(&leftLfo, &rightLfo);
			m_realCutoff[0] = qBound(20.f, detuneWithOctaves(cutoff, leftLfo * amount + inFollow * m_currentPeak[0]), 20000.f);
			m_realCutoff[1] = qBound(20.f, detuneWithOctaves(cutoff, rightLfo * amount + inFollow * m_currentPeak[1]), 20000.f);
		}
		else
		{
			m_realCutoff[0] = qBound(20.f, detuneWithOctaves(cutoff, inFollow * m_currentPeak[0]), 20000.f);
			m_realCutoff[1] = qBound(20.f, detuneWithOctaves(cutoff, inFollow * m_currentPeak[1]), 20000.f);
		}

		++m_filtFeedbackLoc;
		if (m_filtFeedbackLoc > m_filtDelayBuf[0].size() - 1)
		{
			m_filtFeedbackLoc = 0;
		}

		s[0] += m_filtDelayBuf[0][m_filtFeedbackLoc];
		s[1] += m_filtDelayBuf[1][m_filtFeedbackLoc];

		// Precalculate the allpass filter coefficients.
		// Because of very aggressive CPU optimization, some of
		// the coefficient names are sometimes inaccurate concerning
		// what they actually do.
		for (int b = 0; b < 2; ++b)
		{
			const float w0 = m_twoPiOverSr * m_realCutoff[b];
			const float a0 = 1 + (sin(w0) / (resonance * 2.f));
			m_b0[b] = (1 - (a0 - 1)) / a0;
			m_b1[b] = (-2*cos(w0)) / a0;
		}

		for (int a = 0; a < order; ++a)
		{
			for (int b = 0; b < 2; ++b)
			{
				sample_t outSamp;
				calcAllpassFilter(outSamp, s[b], sample_rate, a, b, m_b0[b], m_b1[b]);
				s[b] = outSamp;
			}
		}

		const float realFeedback = feedback * 0.01f;
		m_filtDelayBuf[0][m_filtFeedbackLoc] = s[0] * realFeedback;
		m_filtDelayBuf[1][m_filtFeedbackLoc] = s[1] * realFeedback;

		s[0] = (1 - wetDry) * buf[f][0] + wetDry * s[0];
		s[1] = (1 - wetDry) * buf[f][1] + wetDry * s[1];

		s[0] *= m_outGain;
		s[1] *= m_outGain;

		lInPeak = buf[f][0] > lInPeak ? buf[f][0] : lInPeak;
		rInPeak = buf[f][1] > rInPeak ? buf[f][1] : rInPeak;
		lOutPeak = s[0] > lOutPeak ? s[0] : lOutPeak;
		rOutPeak = s[1] > rOutPeak ? s[1] : rOutPeak;

		buf[f][0] = d * buf[f][0] + w * s[0];
		buf[f][1] = d * buf[f][1] + w * s[1];
	}

	checkGate(outSum / frames);

	m_phaserControls.m_outPeakL = lOutPeak;
	m_phaserControls.m_outPeakR = rOutPeak;
	m_phaserControls.m_inPeakL = lInPeak;
	m_phaserControls.m_inPeakR = rInPeak;

	return isRunning();
}



inline void PhaserEffect::calcAllpassFilter(sample_t &outSamp, sample_t inSamp, sample_rate_t Fs, int filtNum, int channel, float b0, float b1)
{
	// The original formula can be found here: https://pastebin.com/AyMH6k36
	// Much effort was put into CPU optimization, so now the filters
	// only require a very small num_mber of operations.
	m_filtY[filtNum][channel][0] = b0 * (inSamp - m_filtY[filtNum][channel][2]) +
		b1 * (m_filtX[filtNum][channel][1] - m_filtY[filtNum][channel][1]) +
		m_filtX[filtNum][channel][2];

	m_filtX[filtNum][channel][2] = m_filtX[filtNum][channel][1];
	m_filtX[filtNum][channel][1] = inSamp;
	m_filtY[filtNum][channel][2] = m_filtY[filtNum][channel][1];
	m_filtY[filtNum][channel][1] = m_filtY[filtNum][channel][0];

	outSamp = m_filtY[filtNum][channel][0];
}


void PhaserEffect::changeSampleRate()
{
	m_lfo->setSampleRate(Engine::mixer()->processingSampleRate());
	m_twoPiOverSr = F_2PI / Engine::mixer()->processingSampleRate();
	calcAttack();
	calcRelease();
}


void PhaserEffect::restartLFO()
{
	m_lfo->restart();
}



// Takes input of original Hz and the number of cents to detune it by, and returns the detuned result in Hz.
inline float PhaserEffect::detuneWithOctaves(float pitchValue, float detuneValue)
{
	return pitchValue * std::exp2(detuneValue); 
}



extern "C"
{

// necessary for getting instance out of shared lib
PLUGIN_EXPORT Plugin * lmms_plugin_main(Model* parent, void* data)
{
	return new PhaserEffect(parent, static_cast<const Plugin::Descriptor::SubPluginFeatures::Key *>(data));
}

}

