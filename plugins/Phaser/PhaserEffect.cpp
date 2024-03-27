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
#include "Engine.h"
#include "GuiApplication.h"
#include "lmms_math.h"
#include "MainWindow.h"
#include "plugin_export.h"
#include "Song.h"

namespace lmms
{

extern "C"
{

Plugin::Descriptor PLUGIN_EXPORT phaser_plugin_descriptor =
{
	LMMS_STRINGIFY(PLUGIN_NAME),
	"Phaser",
	QT_TRANSLATE_NOOP("PluginBrowser", "A versatile phaser plugin"),
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
	Effect(&phaser_plugin_descriptor, parent, key),
	m_phaserControls(this)
{
	m_currentPeak[0] = m_currentPeak[1] = PHA_NOISE_FLOOR;

	m_lfo = new QuadratureLfo(Engine::audioEngine()->processingSampleRate());
	m_lfo->setFrequency(1.0 / m_phaserControls.m_rateModel.value());
	
	// Prepare the oversampling filters
	double coefs[PHASER_OVERSAMPLE_COEFS];
	hiir::PolyphaseIir2Designer::compute_coefs_spec_order_tbw(coefs, PHASER_OVERSAMPLE_COEFS, 0.04);
	for (int i = 0; i < 2; ++i)
	{
		m_oversampleAnalogIn1[i].set_coefs(coefs);
		m_oversampleAnalogIn2[i].set_coefs(coefs);
		m_oversampleAnalogOut[i].set_coefs(coefs);
		m_oversampleFeedbackIn[i].set_coefs(coefs);
		m_oversampleFeedbackOut[i].set_coefs(coefs);
	}

	changeSampleRate();

	connect(&m_phaserControls.m_attackModel, SIGNAL(dataChanged()), this, SLOT(calcAttack()), Qt::DirectConnection);
	connect(&m_phaserControls.m_releaseModel, SIGNAL(dataChanged()), this, SLOT(calcRelease()), Qt::DirectConnection);
	connect(&m_phaserControls.m_outGainModel, SIGNAL(dataChanged()), this, SLOT(calcOutGain()), Qt::DirectConnection);
	connect(&m_phaserControls.m_inGainModel, SIGNAL(dataChanged()), this, SLOT(calcInGain()), Qt::DirectConnection);
	connect(&m_phaserControls.m_phaseModel, SIGNAL(dataChanged()), this, SLOT(calcPhase()), Qt::DirectConnection);

	calcAttack();
	calcRelease();
	calcOutGain();
	calcInGain();
	calcPhase();	

	connect(Engine::audioEngine(), SIGNAL(sampleRateChanged()), this, SLOT(changeSampleRate()));
	connect(Engine::getSong(), SIGNAL(playbackStateChanged()), this, SLOT(restartLFO()));
}


PhaserEffect::~PhaserEffect()
{
	delete m_lfo;
}


void PhaserEffect::calcAttack()
{
	m_attCoeff = pow(10.f, (PHA_LOG / (m_phaserControls.m_attackModel.value() * 0.001)) / Engine::audioEngine()->processingSampleRate());
}

void PhaserEffect::calcRelease()
{
	m_relCoeff = pow(10.f, (-PHA_LOG / (m_phaserControls.m_releaseModel.value() * 0.001)) / Engine::audioEngine()->processingSampleRate());
}

void PhaserEffect::calcOutGain()
{
	m_outGain = dbfsToAmp(m_phaserControls.m_outGainModel.value());
}

void PhaserEffect::calcInGain()
{
	m_inGain = dbfsToAmp(m_phaserControls.m_inGainModel.value());
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

	sample_rate_t sample_rate =  Engine::audioEngine()->processingSampleRate();

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
	const ValueBuffer * modeBuf = m_phaserControls.m_modeModel.valueBuffer();
	const ValueBuffer * analogBuf = m_phaserControls.m_analogModel.valueBuffer();
	const ValueBuffer * analogDistBuf = m_phaserControls.m_analogDistModel.valueBuffer();
	const ValueBuffer * delayBuf = m_phaserControls.m_delayModel.valueBuffer();
	const ValueBuffer * cutoffControlBuf = m_phaserControls.m_cutoffControlModel.valueBuffer();
	const ValueBuffer * delayControlBuf = m_phaserControls.m_delayControlModel.valueBuffer();
	const ValueBuffer * doubleBuf = m_phaserControls.m_doubleModel.valueBuffer();
	const ValueBuffer * aliasBuf = m_phaserControls.m_aliasModel.valueBuffer();
	const ValueBuffer * distortionBuf = m_phaserControls.m_distortionModel.valueBuffer();
	const ValueBuffer * feedbackBuf = m_phaserControls.m_feedbackModel.valueBuffer();

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
		                Feedback diagrams


		                      Default

		              -------------------------
		              |                       |
		              |                       V
		x(t)-->(+)-->(+)-->[allpass]-->(+)-->(+)-->(*0.5)-->y(t)
		        ^                       |
		        |                       |
		        --{delay}<---(*k)<-------

		A regular phaser signal flow, as suggested in Vadim
		Zavalishin's "The Art of VA Filter Design" book.


		                   Standard

		        -------------------------------
		        |                             |
		        |                             V
		x(t)-->(+)-->(+)-->[allpass]-->(+)-->(+)-->(*0.5)-->y(t)
		              ^                 |
		              |                 |
		              --{delay}<--(*k)<--

		A more generic signal flow.


		                            Nested

		              ------------>(*-k)-----------------
		              |                                 |
		              |                                 V
		x(t)-->(+)-->(+)-->{delay}-->[allpass]-->(+)-->(+)-->(*0.5)-->y(t)
		        ^                                 |
		        |                                 |
		        -------------------(*k)<-----------

		A nested allpass filter.


		                        Yeet Mode

		              -------------------------------
		              |                             |
		              |                             V
		x(t)-->(+)-->(+)----->[allpass]----->(+)-->(+)-->(*0.5)-->y(t)
		        ^                             |
		        |                             |
		        --[allpass]<--{delay}<---(*k)<-

		Straight insanity.  Two allpass filters are shown,
		but only one filter buffer is used, so every allpass
		calculation borrows the buffer left over from the other.

		---------------------------------------------------------

		Usually the delays shown in the above diagrams never
		show up in any phasers, but I added them in because
		they sound really cool (adjustable via delay knob).

		The extra delay can turn this into an interesting
		allpass/comb hybrid.
		*/

		float drySignal[2] = {buf[f][0], buf[f][1]};

		drySignal[0] *= m_inGain;
		drySignal[1] *= m_inGain;

		sample_t s[2] = {drySignal[0], drySignal[1]};

		/*
		You can reflect an entire sound's frequencies
		around Nyquist by flipping every other sample.
		This Phaser's Alias button does this both before
		and after the allpass filters, linearly reflecting
		the resulting phase response around Nyquist.
		*/
		m_aliasFlip = -m_aliasFlip;

		const float cutoff = cutoffBuf ? cutoffBuf->value(f) : m_phaserControls.m_cutoffModel.value();
		const float resonance = resonanceBuf ? resonanceBuf->value(f) : m_phaserControls.m_resonanceModel.value();
		const float order = orderBuf ? orderBuf->value(f) : m_phaserControls.m_orderModel.value();
		const bool enableLFO = enableLFOBuf ? enableLFOBuf->value(f) : m_phaserControls.m_enableLFOModel.value();
		const float amount = amountBuf ? amountBuf->value(f) : m_phaserControls.m_amountModel.value();
		const float inFollow = inFollowBuf ? inFollowBuf->value(f) : m_phaserControls.m_inFollowModel.value();
		const bool invert = invertBuf ? invertBuf->value(f) : m_phaserControls.m_invertModel.value();
		const bool wetIsolate = wetBuf ? wetBuf->value(f) : m_phaserControls.m_wetModel.value();
		const int mode = modeBuf ? modeBuf->value(f) : m_phaserControls.m_modeModel.value();
		const bool analog = analogBuf ? analogBuf->value(f) : m_phaserControls.m_analogModel.value();
		const float analogDist = analogDistBuf ? analogDistBuf->value(f) : m_phaserControls.m_analogDistModel.value();
		const bool doubleVal = doubleBuf ? doubleBuf->value(f) : m_phaserControls.m_doubleModel.value();
		const bool alias = aliasBuf ? aliasBuf->value(f) : m_phaserControls.m_aliasModel.value();
		const float delay = delayBuf ? delayBuf->value(f) : m_phaserControls.m_delayModel.value();
		const float cutoffControl = cutoffControlBuf ? cutoffControlBuf->value(f) : m_phaserControls.m_cutoffControlModel.value();
		const float delayControl = delayControlBuf ? delayControlBuf->value(f) : m_phaserControls.m_delayControlModel.value();
		const float distortion = (distortionBuf ? distortionBuf->value(f) : m_phaserControls.m_distortionModel.value()) * 0.01f;
		const float feedback = (feedbackBuf ? feedbackBuf->value(f) : m_phaserControls.m_feedbackModel.value()) * 0.01f;

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
		float lfoResults[2] = {0, 0};
		if (enableLFO)
		{
			m_lfo->tick(&lfoResults[0], &lfoResults[1]);
		}

		const float realLfo[2] = {lfoResults[0] * amount, lfoResults[1] * amount};
		const float realInFollow[2] = {m_currentPeak[0] * inFollow, m_currentPeak[1] * inFollow};
		for (int i = 0; i < 2; i++)
		{
			m_displayCutoff[i] = qBound(20.f, detuneWithOctaves(cutoff, realLfo[i] + realInFollow[i]), 20000.f);
			m_realCutoff[i] = qBound(20.f, detuneWithOctaves(cutoff, (realLfo[i] + realInFollow[i]) * cutoffControl), 20000.f);
		}
		
		//Precalculate the allpass filter coefficients.
		float apCoeff1[2];
		float apCoeff2[2];
		for (int b = 0; b < 2; ++b)
		{
			const float w0 = m_twoPiOverSr * m_realCutoff[b];
			const float a0 = 1 + (sin(w0) / (resonance * 2.f));
			apCoeff1[b] = (1 - (a0 - 1)) / a0;
			apCoeff2[b] = (-2*cos(w0)) / a0;
		}

		float firstAdd[2] = {0, 0};
		float secondAdd[2] = {0, 0};
		float delayOut[2] = {0, 0};

		for (int i = 0; i < 2; ++i)
		{
			// Calculate delay amount to find read location
			float readLoc = m_filtFeedbackLoc -
				qMin(abs(delay + ((realLfo[i] + realInFollow[i]) * delayControl)), 20.f) *
				0.001f * Engine::audioEngine()->processingSampleRate();

			if (readLoc < 0) {readLoc += m_delayBufSize;}
			float readLocFrac = fraction(readLoc);

			// Read value from delay buffer
			if (readLoc < m_delayBufSize - 1)
			{
				delayOut[i] = m_filtDelayBuf[i][floor(readLoc)] * (1 - readLocFrac) + m_filtDelayBuf[i][ceil(readLoc)] * readLocFrac;
			}
			else// For when the interpolation wraps around to the beginning of the buffer
			{
				delayOut[i] = m_filtDelayBuf[i][m_delayBufSize - 1] * (1 - readLocFrac) + m_filtDelayBuf[i][0] * readLocFrac;
			}
		}

		float tempAdd[2] = {0, 0};
		switch (mode)
		{
			case 0: case 1:
			{
				tempAdd[0] = delayOut[0] * feedback;
				tempAdd[1] = delayOut[1] * feedback;
				break;
			}
			case 2:
			{
				tempAdd[0] = m_lastSecondAdd[0] * feedback;
				tempAdd[1] = m_lastSecondAdd[1] * feedback;
				break;
			}
			case 3:
			{
				tempAdd[0] = delayOut[0] * feedback;
				tempAdd[1] = delayOut[1] * feedback;
				for (int a = 0; a < order; ++a)
				{
					for (int b = 0; b < 2; ++b)
					{
						for (int c = 0; c < 1 + doubleVal; ++c)// Runs twice if doubleVal is true
						{
							tempAdd[b] = calcAllpassFilter(tempAdd[b], sample_rate, a, b, apCoeff1[b], apCoeff2[b]);
						}
					}
				}
			}
		}
		if (analog)
		{
			for (int i = 0; i < 2; ++i)
			{
				// Upsample
				float oversampledS1;
				float oversampledS2;
				m_oversampleAnalogIn1[i].process_sample(oversampledS1, oversampledS2, s[i]);
				
				float oversampledAdd1;
				float oversampledAdd2;
				m_oversampleAnalogIn2[i].process_sample(oversampledAdd1, oversampledAdd2, tempAdd[i]);
			
				// Waveshaping
				float analogOut1 = tanh(oversampledS1 * analogDist) / analogDist + tanh(oversampledAdd1 * analogDist) / analogDist;
				float analogOut2 = tanh(oversampledS2 * analogDist) / analogDist + tanh(oversampledAdd2 * analogDist) / analogDist;
				
				// Downsample
				float downsampleIn[2] = {analogOut1, analogOut2};
				firstAdd[i] = m_oversampleFeedbackOut[i].process_sample(&downsampleIn[0]);
			}
		}
		else
		{
			firstAdd[0] = s[0] + tempAdd[0];
			firstAdd[1] = s[1] + tempAdd[1];
		}
		

		
		// Unleash the allpass filters
		switch (mode)
		{
			case 0: case 1: case 3:
			{
				secondAdd[0] = firstAdd[0];
				secondAdd[1] = firstAdd[1];
				break;
			}
			case 2:
			{
				secondAdd[0] = delayOut[0];
				secondAdd[1] = delayOut[1];
				break;
			}
		}

		if (alias)
		{
			secondAdd[0] *= m_aliasFlip;
			secondAdd[1] *= m_aliasFlip;
		}
		for (int a = 0; a < order; ++a)
		{
			for (int b = 0; b < 2; ++b)
			{
				for (int c = 0; c < 1 + doubleVal; ++c)// Runs twice if doubleVal is true
				{
					secondAdd[b] = calcAllpassFilter(secondAdd[b], sample_rate, a, b, apCoeff1[b], apCoeff2[b]);
				}
			}
		}
		if (alias)
		{
			secondAdd[0] *= m_aliasFlip;
			secondAdd[1] *= m_aliasFlip;
		}

		// Get final result
		switch (mode)
		{
			case 0: case 3:
			{
				s[0] = (secondAdd[0] * (invert ? -1 : 1) + firstAdd[0] * !wetIsolate) / 2.f;
				s[1] = (secondAdd[1] * (invert ? -1 : 1) + firstAdd[1] * !wetIsolate) / 2.f;
				break;
			}
			case 1:
			{
				s[0] = (secondAdd[0] * (invert ? -1 : 1) + s[0] * !wetIsolate) / 2.f;
				s[1] = (secondAdd[1] * (invert ? -1 : 1) + s[1] * !wetIsolate) / 2.f;
				break;
			}
			case 2:
			{
				s[0] = (secondAdd[0] * (invert ? -1 : 1) + firstAdd[0] * !wetIsolate * -feedback) / 2.f;
				s[1] = (secondAdd[1] * (invert ? -1 : 1) + firstAdd[1] * !wetIsolate * -feedback) / 2.f;
				break;
			}
		}

		m_lastSecondAdd[0] = secondAdd[0];
		m_lastSecondAdd[1] = secondAdd[1];

		tempAdd[0] = tempAdd[1] = 0;
		switch (mode)
		{
			case 0: case 1: case 3:
			{
				tempAdd[0] = secondAdd[0];
				tempAdd[1] = secondAdd[1];
				break;
			}
			case 2:
			{
				tempAdd[0] = firstAdd[0];
				tempAdd[1] = firstAdd[1];
				break;
			}
		}
		
		const float trueDistVal = distortion * 1.5;
		for (int i = 0; i < 2; ++i)
		{
			// Upsample
			float tempAdd1;
			float tempAdd2;
			m_oversampleFeedbackIn[i].process_sample(tempAdd1, tempAdd2, tempAdd[i]);


			//Some gentle feedback distortion to prevent infinite loops.
			tempAdd1 = tanh(tempAdd1 * 0.25) * 4;
			tempAdd2 = tanh(tempAdd2 * 0.25) * 4;

			if (distortion)
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
				
				tempAdd1 = abs(tempAdd1 + 1.5 - trueDistVal) - 1.5 + trueDistVal;
				tempAdd2 = abs(tempAdd2 + 1.5 - trueDistVal) - 1.5 + trueDistVal;
			}
			
			// Downsample
			float downsampleIn[2] = {tempAdd1, tempAdd2};
			tempAdd[i] = m_oversampleFeedbackOut[i].process_sample(&downsampleIn[0]);
		}

		// Remove DC Offset
		for (int i = 0; i < 2; ++i)
		{
			// Just subtract the approximate average of the latest many
			// audio samples.
			m_sampAvg[i] = m_sampAvg[i] * (1.f - m_dcTimeConst) + tempAdd[i] * m_dcTimeConst;
			tempAdd[i] -= m_sampAvg[i];
		}


		// Increment delay ring buffer index
		++m_filtFeedbackLoc;
		if (m_filtFeedbackLoc >= m_delayBufSize)
		{
			m_filtFeedbackLoc -= m_delayBufSize;
		}

		// Send new value to delay line
		m_filtDelayBuf[0][m_filtFeedbackLoc] = tempAdd[0];
		m_filtDelayBuf[1][m_filtFeedbackLoc] = tempAdd[1];

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


sample_t PhaserEffect::calcAllpassFilter(sample_t inSamp, sample_rate_t Fs, int filtNum, int channel, float apCoeff1, float apCoeff2)
{
	// The original formula can be found here: https://pastebin.com/AyMH6k36
	// Much effort was put into CPU optimization, so now the filters
	// only require a very small number of operations.

	float filterOutput = apCoeff1 * (inSamp - m_filtY[filtNum][channel][1]) +
		apCoeff2 * (m_filtX[filtNum][channel][0] - m_filtY[filtNum][channel][0]) +
		m_filtX[filtNum][channel][1];

	m_filtX[filtNum][channel][1] = m_filtX[filtNum][channel][0];
	m_filtX[filtNum][channel][0] = inSamp;
	m_filtY[filtNum][channel][1] = m_filtY[filtNum][channel][0];
	m_filtY[filtNum][channel][0] = filterOutput;

	return filterOutput;
}


void PhaserEffect::changeSampleRate()
{
	m_lfo->setSampleRate(Engine::audioEngine()->processingSampleRate());
	m_twoPiOverSr = F_2PI / Engine::audioEngine()->processingSampleRate();
	m_dcTimeConst = 44.1f / Engine::audioEngine()->processingSampleRate();
	calcAttack();
	calcRelease();

	for (int b = 0; b < 2; ++b)
	{
		m_delayBufSize = Engine::audioEngine()->processingSampleRate() * 0.03 + 1;
		m_filtDelayBuf[b].resize(m_delayBufSize);
	}
}


void PhaserEffect::restartLFO()
{
	m_lfo->restart();
}


extern "C"
{

// necessary for getting instance out of shared lib
PLUGIN_EXPORT Plugin * lmms_plugin_main(Model* parent, void* data)
{
	return new PhaserEffect(parent, static_cast<const Plugin::Descriptor::SubPluginFeatures::Key *>(data));
}
}

} // namespace lmms
