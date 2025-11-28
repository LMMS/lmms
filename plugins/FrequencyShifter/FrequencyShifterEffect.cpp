/*
 * FrequencyShifter.cpp
 *
 * Copyright (c) 2025 Lost Robot <r94231/at/gmail/dot/com>
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

#include "FrequencyShifterEffect.h"

#include "embed.h"
#include "plugin_export.h"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace lmms
{

extern "C"
{
Plugin::Descriptor PLUGIN_EXPORT frequencyshifter_plugin_descriptor =
{
	LMMS_STRINGIFY(PLUGIN_NAME),
	"Frequency Shifter",
	QT_TRANSLATE_NOOP("PluginBrowser", "A frequency shifter (not a pitch shifter) and barberpole phaser plugin"),
	"Lost Robot <r94231/at/gmail/dot/com>",
	0x0100,
	Plugin::Type::Effect,
	new PluginPixmapLoader("logo"),
	nullptr,
	nullptr,
};
}

FrequencyShifterEffect::FrequencyShifterEffect(Model* parent, const Descriptor::SubPluginFeatures::Key* key) :
	Effect(&frequencyshifter_plugin_descriptor, parent, key),
	m_controls(this)
{
	m_ringBuf.resize(1, {0.f, 0.f});

	connect(Engine::audioEngine(), &AudioEngine::sampleRateChanged,
		this, &FrequencyShifterEffect::updateSampleRate);
	updateSampleRate();
}

Effect::ProcessStatus FrequencyShifterEffect::processImpl(SampleFrame* buf, const fpp_t frames)
{
	const float twoPi = std::numbers::pi_v<float> * 2.0f;

	const float mix = m_controls.m_mix.value() * wetLevel();
	const float fs = m_controls.m_freqShift.value();
	const float spread = m_controls.m_spreadShift.value();
	const float ring = m_controls.m_ring.value();
	const float feedback = m_controls.m_feedback.value();
	const float delayLen = (m_controls.m_delayLengthLong.value() + m_controls.m_delayLengthShort.value()) * 0.001f * m_sampleRate;
	const float delayDamp = m_controls.m_delayDamp.value();
	const float delayGlide = m_controls.m_delayGlide.value();
	const float lfoAmt = m_controls.m_lfoAmount.value();
	const float lfoRate = (m_controls.m_lfoRate.value() / m_sampleRate) * twoPi;
	const float lfoSt = m_controls.m_lfoStereoPhase.value() * twoPi;
	const bool antireflect = m_controls.m_antireflect.value();
	const int routeMode = m_controls.m_routeMode.value();
	const float harmonics = m_controls.m_harmonics.value();
	const float glide = m_controls.m_glide.value();
	const float tone = m_controls.m_tone.value();
	const float phase = m_controls.m_phase.value() * twoPi;

	const bool resetShifterBtn = m_controls.m_resetShifter.value();
	const bool resetLfoBtn = m_controls.m_resetLfo.value();

	if (!m_prevResetShifter && resetShifterBtn)
	{
		m_phase[0] = 0.f;
		m_phase[1] = 0.f;
	}
	if (!m_prevResetLfo && resetLfoBtn)
	{
		m_lfoPhase = 0.f;
	}
	m_prevResetShifter = resetShifterBtn;
	m_prevResetLfo = resetLfoBtn;

	const float twoPiOverSr = twoPi / m_sampleRate;
	const float invRing = 1.f - ring;
	const bool parallelFB = (routeMode >= 1);
	const bool routeAdd = (routeMode == 1);

	const bool doHarm = (harmonics > 0.f);
	const float harmFactor = harmonics * 20.f + 1.f;
	const float harmDiv = 1.f / (harmonics * 0.3f + 1.f);

	const float dampCoeff = std::exp(-2.f * std::numbers::pi_v<float> * delayDamp / m_sampleRate);
	const float toneCoeff = std::exp(-2.f * std::numbers::pi_v<float> * tone / m_sampleRate);
	const float delayGlideCoeff = delayGlide ? std::exp(-1.f / (delayGlide * m_sampleRate)) : 0.f;
	const float glideCoeff = glide ? std::exp(-1.f / (glide * m_sampleRate)) : 0.f;

	const int wantSize = static_cast<int>(delayLen) + 4;// +4 provides space for interpolation
	if (wantSize > m_ringBufSize)
	{
		m_ringBufSize = wantSize;
		m_ringBuf.resize(wantSize);
	}

	// we only bother with wrapping phases once per buffer
	m_lfoPhase = std::fmod(m_lfoPhase, twoPi);
	for (int ch = 0; ch < 2; ++ch)
	{
		m_phase[ch] = std::fmod(m_phase[ch], twoPi);
	}

	for (fpp_t i = 0; i < frames; ++i)
	{
		float lfo0;
		float lfo1;
		if (lfoAmt > 0.f)
		{
			lfo0 = std::sin(m_lfoPhase) * lfoAmt;
			lfo1 = std::sin(m_lfoPhase + lfoSt) * lfoAmt;
		}
		else
		{
			lfo0 = 0.f;
			lfo1 = 0.f;
		}
		m_lfoPhase += lfoRate;

		const float base0 = fs - spread;
		const float base1 = fs + spread;
		m_trueShift[0] = (1.f - glideCoeff) * base0 + glideCoeff * m_trueShift[0];
		m_trueShift[1] = (1.f - glideCoeff) * base1 + glideCoeff * m_trueShift[1];

		m_trueDelay = std::max((1.f - delayGlideCoeff) * delayLen + delayGlideCoeff * m_trueDelay, 1.f);
		m_truePhase = (1.f - glideCoeff) * phase + glideCoeff * m_truePhase;

		float readIndex = static_cast<float>(m_writeIndex) - m_trueDelay;
		if (readIndex < 0.f) { readIndex += static_cast<float>(m_ringBufSize); }

		const int indexFloor = static_cast<int>(readIndex);
		const float frac = readIndex - static_cast<float>(indexFloor);

		const std::array<float, 2> dly = getHermiteSample(indexFloor, frac);

		const float inL = buf[i][0];
		const float inR = buf[i][1];
		const float fxInL = parallelFB ? (dly[0] * feedback) : (inL + dly[0] * feedback);
		const float fxInR = parallelFB ? (dly[1] * feedback) : (inR + dly[1] * feedback);
		
		++m_writeIndex;
		if (m_writeIndex == m_ringBufSize)
		{
			m_writeIndex = 0;
		}

		// delta phase
		const float dPh0 = (m_trueShift[0] + lfo0) * twoPiOverSr;
		const float dPh1 = (m_trueShift[1] + lfo1) * twoPiOverSr;

		float outL;
		float outR;

		{
			float fxIn[2] = {fxInL, fxInR};
			float dPh[2] = {dPh0, dPh1};
			float out[2];

			for (int ch = 0; ch < 2; ++ch)
			{
				const float phaseValue = m_phase[ch] + m_truePhase;
				
				const float s = std::sin(phaseValue);
				const float c = std::cos(phaseValue);
				
				float cosP = c;
				float sinP = s;

				if (doHarm)
				{
					// arbitrary distortion function, crossfaded with original signal
					const float xc = std::clamp(harmFactor * cosP, -3.f, 3.f);
					const float xs = std::clamp(harmFactor * sinP, -3.f, 3.f);
					const float xc2 = xc * xc;
					const float xs2 = xs * xs;
					const float tc = xc * (27.f + xc2) / (27.f + 9.f * xc2);
					const float ts = xs * (27.f + xs2) / (27.f + 9.f * xs2);
					cosP = std::lerp(cosP, tc * harmDiv, harmonics);
					sinP = std::lerp(sinP, ts * harmDiv, harmonics);
				}

				float analytic1[2];
				m_hilbert1.processReal(fxIn[ch], ch, analytic1);

				const float oscR = cosP;
				const float oscI = sinP * invRing;

				const float modR = analytic1[0] * oscR - analytic1[1] * oscI;
				const float modI = analytic1[0] * oscI + analytic1[1] * oscR;

				float shiftedR;
				float shiftedI;

				if (antireflect)
				{
					float mod[2] = {modR, modI};
					float analytic2[2];
					m_hilbert2.processComplex(mod, ch, analytic2);
					shiftedR = analytic2[0] * 0.5f;
					shiftedI = analytic2[1] * 0.5f;
				}
				else
				{
					shiftedR = modR;
					shiftedI = modI;
				}

				m_phase[ch] += dPh[ch];
				out[ch] = shiftedR + ring * shiftedI;
			}

			outL = out[0];
			outR = out[1];
		}

		float delayInL = outL + (parallelFB ? inL : 0.f);
		float delayInR = outR + (parallelFB ? inR : 0.f);

		// saturate feedback loop to ensure it doesn't explode
		constexpr float FbSaturation = 16.f;
		delayInL = (FbSaturation * delayInL) / (FbSaturation + std::fabs(delayInL));
		delayInR = (FbSaturation * delayInR) / (FbSaturation + std::fabs(delayInR));

		// 1-pole lowpass in feedback loop
		m_dampState[0] = (1.f - dampCoeff) * delayInL + dampCoeff * m_dampState[0];
		m_dampState[1] = (1.f - dampCoeff) * delayInR + dampCoeff * m_dampState[1];
		m_ringBuf[m_writeIndex][0] = m_dampState[0];
		m_ringBuf[m_writeIndex][1] = m_dampState[1];

		// 1-pole lowpass on entire signal
		m_toneState[0] = (1.f - toneCoeff) * outL + toneCoeff * m_toneState[0];
		m_toneState[1] = (1.f - toneCoeff) * outR + toneCoeff * m_toneState[1];
		outL = m_toneState[0];
		outR = m_toneState[1];

		if (routeAdd)
		{
			buf[i][0] = inL + mix * outL;
			buf[i][1] = inR + mix * outR;
		}
		else
		{
			const float dry = 1.f - mix;
			buf[i][0] = dry * inL + mix * outL;
			buf[i][1] = dry * inR + mix * outR;
		}
	}

	return ProcessStatus::ContinueIfNotQuiet;
}

void FrequencyShifterEffect::updateSampleRate()
{
	m_sampleRate = Engine::audioEngine()->outputSampleRate();

	m_hilbert1 = HilbertIIRFloat<2>(m_sampleRate, 2.0f);
	m_hilbert2 = HilbertIIRFloat<2>(m_sampleRate, 2.0f);
}

extern "C"
{
PLUGIN_EXPORT Plugin* lmms_plugin_main(Model* parent, void* data)
{
	return new FrequencyShifterEffect(parent, static_cast<const Plugin::Descriptor::SubPluginFeatures::Key*>(data));
}
}

} // namespace lmms

