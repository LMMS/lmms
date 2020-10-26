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
#include "Engine.h"
#include "Song.h"
#include "GuiApplication.h"
#include "MainWindow.h"

extern "C"
{

Plugin::Descriptor PLUGIN_EXPORT Phaser_plugin_descriptor =
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


constexpr float PHA_NOISE_FLOOR = 0.00001f;
constexpr double PHA_LOG = 2.2;


PhaserEffect::PhaserEffect(Model* parent, const Descriptor::SubPluginFeatures::Key* key) :
	Effect(&Phaser_plugin_descriptor, parent, key),
	m_phaserControls(this)
{
	m_currentPeak[0] = m_currentPeak[1] = PHA_NOISE_FLOOR;

	m_lfo = new QuadratureLfo(Engine::mixer()->processingSampleRate());

	m_lfo->setFrequency(1.0 / m_phaserControls.m_rateModel.value());

	changeSampleRate();

	connect(&m_phaserControls.m_attackModel, SIGNAL(dataChanged()), this, SLOT(calcAttack()), Qt::DirectConnection);
	connect(&m_phaserControls.m_releaseModel, SIGNAL(dataChanged()), this, SLOT(calcRelease()), Qt::DirectConnection);
	connect(&m_phaserControls.m_distortionModel, SIGNAL(dataChanged()), this, SLOT(calcDistortion()), Qt::DirectConnection);
	connect(&m_phaserControls.m_feedbackModel, SIGNAL(dataChanged()), this, SLOT(calcFeedback()), Qt::DirectConnection);
	connect(&m_phaserControls.m_outGainModel, SIGNAL(dataChanged()), this, SLOT(calcOutGain()), Qt::DirectConnection);
	connect(&m_phaserControls.m_inGainModel, SIGNAL(dataChanged()), this, SLOT(calcInGain()), Qt::DirectConnection);
	connect(&m_phaserControls.m_delayModel, SIGNAL(dataChanged()), this, SLOT(calcDelay()), Qt::DirectConnection);
	connect(&m_phaserControls.m_phaseModel, SIGNAL(dataChanged()), this, SLOT(calcPhase()), Qt::DirectConnection);

	calcAttack();
	calcRelease();
	calcDistortion();
	calcFeedback();
	calcOutGain();
	calcInGain();
	calcDelay();
	calcPhase();	

	connect(Engine::mixer(), SIGNAL(sampleRateChanged()), this, SLOT(changeSampleRate()));
	connect(Engine::getSong(), SIGNAL(playbackStateChanged()), this, SLOT(restartLFO()));
}



PhaserEffect::~PhaserEffect()
{
	delete m_lfo;
}



void PhaserEffect::calcAttack()
{
	m_attCoeff = exp10((PHA_LOG / (m_phaserControls.m_attackModel.value() * 0.001)) / Engine::mixer()->processingSampleRate());
}

void PhaserEffect::calcRelease()
{
	m_relCoeff = exp10((-PHA_LOG / (m_phaserControls.m_releaseModel.value() * 0.001)) / Engine::mixer()->processingSampleRate());
}

void PhaserEffect::calcDistortion()
{
	m_distVal = m_phaserControls.m_distortionModel.value() * 0.01f;
}

void PhaserEffect::calcFeedback()
{
	m_feedbackVal = m_phaserControls.m_feedbackModel.value() * 0.01f;
}

void PhaserEffect::calcOutGain()
{
	m_outGain = dbfsToAmp(m_phaserControls.m_outGainModel.value());
}

void PhaserEffect::calcInGain()
{
	m_inGain = dbfsToAmp(m_phaserControls.m_inGainModel.value());
}

void PhaserEffect::calcDelay()
{
	m_delayVal = m_phaserControls.m_delayModel.value() * 0.001f * Engine::mixer()->processingSampleRate();
}

void PhaserEffect::calcPhase()
{
	m_lfo->setOffset(m_phaserControls.m_phaseModel.value() / 180 * D_PI);
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

	// The following are sample-exact.
	const ValueBuffer * cutoffBuf = m_phaserControls.m_cutoffModel.valueBuffer();
	const ValueBuffer * resonanceBuf = m_phaserControls.m_resonanceModel.valueBuffer();
	const ValueBuffer * orderBuf = m_phaserControls.m_orderModel.valueBuffer();
	const ValueBuffer * enableLFOBuf = m_phaserControls.m_enableLFOModel.valueBuffer();
	const ValueBuffer * amountBuf = m_phaserControls.m_amountModel.valueBuffer();
	const ValueBuffer * inFollowBuf = m_phaserControls.m_inFollowModel.valueBuffer();
	const ValueBuffer * invertBuf = m_phaserControls.m_invertModel.valueBuffer();
	const ValueBuffer * wetBuf = m_phaserControls.m_wetModel.valueBuffer();

	float lOutPeak = 0.0;
	float rOutPeak = 0.0;
	float lInPeak = 0.0;
	float rInPeak = 0.0;

	if (m_phaserControls.m_inFollowModel.isValueChanged() && m_phaserControls.m_inFollowModel.value() == 0)
	{
		m_currentPeak[0] = m_currentPeak[1] = PHA_NOISE_FLOOR;
	}
	
	m_lfo->setFrequency(1.0 / m_phaserControls.m_rateModel.value());


	for (fpp_t f = 0; f < frames; ++f)
	{
		/*
		                Feedback diagram

		              -------------------------
		              |                       |
		              |                       V
		x(t)-->(+)-->(+)-->[allpass]-->(+)-->(+)-->(*0.5)-->y(t)
		        ^                       |
		        |                       |
		        --{delay}<---(*k)<-------


		Usually the delay in the bottom isn't there, but I added
		one in because it sounds really cool (adjustable via
		delay knob).  When the delay is at 0, there really is 0
		delay, not just a 1-sample delay like some may expect.
		The extra delay can turn this into an interesting
		allpass/comb hybrid.
		*/

		float drySignal[2] = {buf[f][0], buf[f][1]};

		drySignal[0] *= m_inGain;
		drySignal[1] *= m_inGain;

		sample_t s[2] = {drySignal[0], drySignal[1]};

		const float cutoff = cutoffBuf ? cutoffBuf->value(f) : m_phaserControls.m_cutoffModel.value();
		const float resonance = resonanceBuf ? resonanceBuf->value(f) : m_phaserControls.m_resonanceModel.value();
		const float order = orderBuf ? orderBuf->value(f) : m_phaserControls.m_orderModel.value();
		const bool enableLFO = enableLFOBuf ? enableLFOBuf->value(f) : m_phaserControls.m_enableLFOModel.value();
		const float amount = amountBuf ? amountBuf->value(f) : m_phaserControls.m_amountModel.value();
		const float inFollow = inFollowBuf ? inFollowBuf->value(f) : m_phaserControls.m_inFollowModel.value();
		const bool invert = invertBuf ? invertBuf->value(f) : m_phaserControls.m_invertModel.value();
		const bool wetIsolate = wetBuf ? wetBuf->value(f) : m_phaserControls.m_wetModel.value();

		// Calculate input follower values
		const double sAbs[2] = {abs(s[0]), abs(s[1])};
		for (int i = 0; i < 2; i++)
		{
			if (sAbs[i] > m_currentPeak[i])
			{
				m_currentPeak[i] = qMin(m_currentPeak[i] * m_attCoeff, sAbs[i]);
			}
			else
			if (sAbs[i] < m_currentPeak[i])
			{
				m_currentPeak[i] = qMax(m_currentPeak[i] * m_relCoeff, sAbs[i]);
			}

			m_currentPeak[i] = qBound(PHA_NOISE_FLOOR, m_currentPeak[i], 10.0f);
		}

		// Calculate real allpass filter frequencies
		if (enableLFO)
		{
			float leftLfo;
			float rightLfo;
			m_lfo->tick(&leftLfo, &rightLfo);
			m_realCutoff[0] = qBound(20.f, detuneWithOctaves(cutoff, leftLfo * amount + inFollow * m_currentPeak[0]), 20000.f);
			m_realCutoff[1] = qBound(20.f, detuneWithOctaves(cutoff, rightLfo * amount + inFollow * m_currentPeak[1]), 20000.f);
		}
		else
		{
			m_realCutoff[0] = qBound(20.f, detuneWithOctaves(cutoff, inFollow * m_currentPeak[0]), 20000.f);
			m_realCutoff[1] = qBound(20.f, detuneWithOctaves(cutoff, inFollow * m_currentPeak[1]), 20000.f);
		}

		/*
		Precalculate the allpass filter coefficients.
		Because of very aggressive CPU optimization, some of
		the coefficient names are inaccurate concerning what
		they actually do.
		*/
		float b0[2];
		float b1[2];
		for (int b = 0; b < 2; ++b)
		{
			const float w0 = m_twoPiOverSr * m_realCutoff[b];
			const float a0 = 1 + (sin(w0) / (resonance * 2.f));
			b0[b] = (1 - (a0 - 1)) / a0;
			b1[b] = (-2*cos(w0)) / a0;
		}

		float firstAdd[2];
		float secondAdd[2];

		// Read next value from delay buffer
		float readLoc = m_filtFeedbackLoc - m_delayVal;
		if (readLoc < 0) {readLoc += m_delayBufSize;}
		float readLocFrac = fraction(readLoc);
		if (readLoc < m_delayBufSize - 1)
		{
			for (int i = 0; i < 2; ++i)
			{
				secondAdd[i] = m_filtDelayBuf[i][floor(readLoc)] * (1 - readLocFrac) + m_filtDelayBuf[i][ceil(readLoc)] * readLocFrac;
			}
		}
		else// For when the interpolation wraps around to the beginning of the buffer
		{
			for (int i = 0; i < 2; ++i)
			{
				secondAdd[i] = m_filtDelayBuf[i][m_delayBufSize - 1] * (1 - readLocFrac) + m_filtDelayBuf[i][0] * readLocFrac;
			}
		}


		// Add feedback results to dry signal
		firstAdd[0] = s[0] + secondAdd[0];
		firstAdd[1] = s[1] + secondAdd[1];

		
		// Unleash the allpass filters
		secondAdd[0] = firstAdd[0];
		secondAdd[1] = firstAdd[1];
		for (int a = 0; a < order; ++a)
		{
			for (int b = 0; b < 2; ++b)
			{
				secondAdd[b] = calcAllpassFilter(secondAdd[b], sample_rate, a, b, b0[b], b1[b]);
			}
		}


		// Get final result
		s[0] = (secondAdd[0] * (invert ? -1 : 1) + firstAdd[0] * !wetIsolate) / 2.f;
		s[1] = (secondAdd[1] * (invert ? -1 : 1) + firstAdd[1] * !wetIsolate) / 2.f;


		/*
		Some gentle feedback distortion to prevent infinite loops.

		I used tanh(x/2)*2 rather than just tanh(x) because tanh(x)
		decreased the volume too much for my liking.
		*/
		secondAdd[0] = tanh(secondAdd[0] * 0.5) * 2;
		secondAdd[1] = tanh(secondAdd[1] * 0.5) * 2;

		if (m_distVal)
		{
			/*
			This behaves as a full-wave rectifier at 100% distortion.

			Why would I put a full-wave rectifier in a feedback loop?
			  1. I needed a distortion type that made a noticeable change
			to the sound that wouldn't result in infinite feedback loops.
			An overdriven symmetrical distortion wouldn't work for that
			reason.
			  2. I just tried this strange idea as an experiment, and,
			surprisingly, it sounded fantastic.

			Note that the DC offset added here by this distortion is
			removed later on.
			*/
			secondAdd[0] = abs(secondAdd[0] + 1.5 - m_distVal * 1.5);
			secondAdd[1] = abs(secondAdd[1] + 1.5 - m_distVal * 1.5);
		}

		// Remove DC Offset
		for (int i = 0; i < 2; ++i)
		{
			// Just subtract the approximate average of the latest many
			// audio samples.
			m_sampAvg[i] = m_sampAvg[i] * 0.999 + secondAdd[i] * 0.001;
			secondAdd[i] -= m_sampAvg[i];
		}


		// Increment delay ring buffer index
		++m_filtFeedbackLoc;
		if (m_filtFeedbackLoc >= m_delayBufSize)
		{
			m_filtFeedbackLoc -= m_delayBufSize;
		}
		// Send new value to delay line
		m_filtDelayBuf[0][m_filtFeedbackLoc] = secondAdd[0] * m_feedbackVal;
		m_filtDelayBuf[1][m_filtFeedbackLoc] = secondAdd[1] * m_feedbackVal;

		s[0] *= m_outGain;
		s[1] *= m_outGain;

		// Calculate values for displaying volume in the two volume meters
		lInPeak = drySignal[0] > lInPeak ? drySignal[0] : lInPeak;
		rInPeak = drySignal[1] > rInPeak ? drySignal[1] : rInPeak;
		lOutPeak = s[0] > lOutPeak ? s[0] : lOutPeak;
		rOutPeak = s[1] > rOutPeak ? s[1] : rOutPeak;

		buf[f][0] = d * buf[f][0] + w * s[0];
		buf[f][1] = d * buf[f][1] + w * s[1];

		outSum += buf[f][0] + buf[f][1];
	}

	checkGate(outSum / frames);

	m_phaserControls.m_outPeakL = lOutPeak;
	m_phaserControls.m_outPeakR = rOutPeak;
	m_phaserControls.m_inPeakL = lInPeak;
	m_phaserControls.m_inPeakR = rInPeak;

	return isRunning();
}



sample_t PhaserEffect::calcAllpassFilter(sample_t inSamp, sample_rate_t Fs, int filtNum, int channel, float b0, float b1)
{
	// The original formula can be found here: https://pastebin.com/AyMH6k36
	// Much effort was put into CPU optimization, so now the filters
	// only require a very small number of operations.

	float filterOutput = b0 * (inSamp - m_filtY[filtNum][channel][1]) +
		b1 * (m_filtX[filtNum][channel][0] - m_filtY[filtNum][channel][0]) +
		m_filtX[filtNum][channel][1];

	m_filtX[filtNum][channel][1] = m_filtX[filtNum][channel][0];
	m_filtX[filtNum][channel][0] = inSamp;
	m_filtY[filtNum][channel][1] = m_filtY[filtNum][channel][0];
	m_filtY[filtNum][channel][0] = filterOutput;

	return filterOutput;
}


void PhaserEffect::changeSampleRate()
{
	m_lfo->setSampleRate(Engine::mixer()->processingSampleRate());
	m_twoPiOverSr = F_2PI / Engine::mixer()->processingSampleRate();
	calcAttack();
	calcRelease();

	for (int b = 0; b < 2; ++b)
	{
		m_delayBufSize = Engine::mixer()->processingSampleRate() * 0.03 + 1;
		m_filtDelayBuf[b].resize(m_delayBufSize);
	}
}


void PhaserEffect::restartLFO()
{
	m_lfo->restart();
}



// Takes input of original Hz and the number of cents to detune it by, and returns the detuned result in Hz.
float PhaserEffect::detuneWithOctaves(float pitchValue, float detuneValue)
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


