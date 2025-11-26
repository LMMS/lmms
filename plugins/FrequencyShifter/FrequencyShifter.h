/*
 * FrequencyShifter.h
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

#ifndef LMMS_FREQUENCY_SHIFTER_H
#define LMMS_FREQUENCY_SHIFTER_H

#include "Effect.h"
#include "FrequencyShifterControls.h"

#include "HilbertTransform.h"
#include "lmms_math.h"

#include <array>
#include <cmath>
#include <numbers>
#include <vector>

constexpr int FREQUENCY_SHIFTER_PREFILTER_STAGES = 4;
constexpr float PrefilterBandwidth = 0.96f; // 96% of nyquist

namespace lmms
{

class FrequencyShifterEffect : public Effect
{
	Q_OBJECT
public:
	FrequencyShifterEffect(Model* parent, const Descriptor::SubPluginFeatures::Key* key);
	~FrequencyShifterEffect() override = default;

	ProcessStatus processImpl(SampleFrame* buf, const fpp_t frames) override;
	EffectControls* controls() override
	{
		return &m_controls;
	}

private slots:
	void updateSampleRate();

private:
	FrequencyShifterControls m_controls;
	float m_fs[2] = {0.f, 0.f};
	float m_phase[2] = {0.f, 0.f};
	float m_sampleRate = 44100.f;

	int m_ringBufSize = 0;
	std::vector<std::array<float, 2>> m_ringBuf;
	int m_writeIndex = 0;

	HilbertIIRFloat<2> m_hilbert1;
	HilbertIIRFloat<2> m_hilbert2;

	float m_lp[2] = {0.f, 0.f};
	float m_hp[2] = {0.f, 0.f};
	float m_lfoPhase[2] = {0.f, 0.f};

	struct PrefilterLP
	{
		float v0z = 0.f;
		float v1 = 0.f;
		float v2 = 0.f;
		float g1 = 0.f;
		float g2 = 0.f;
		float g3 = 0.f;
		float g4 = 0.f;

		void setCoefs(float sr, float cutoff)
		{
			const float g = std::tan(std::numbers::pi_v<float> * cutoff / sr);
			const float ginv = g / (1 + g * (g + std::numbers::sqrt2_v<float>));
			g1 = ginv;
			g2 = 2 * (g + std::numbers::sqrt2_v<float>) * ginv;
			g3 = g * ginv;
			g4 = 2 * ginv;
		}

		float process(float in)
		{
			const float v1z = v1;
			const float v3 = in + v0z - 2 * v2;
			v1 += g1 * v3 - g2 * v1z;
			v2 += g3 * v3 + g4 * v1z;
			v0z = in;
			return v2;
		}
	};

	std::array<PrefilterLP, 2> m_prefilterLP;

	std::array<float, 2> m_dampState = {0.f, 0.f};
	std::array<float, 2> m_toneState = {0.f, 0.f};

	float m_shiftPrePhase[2] = {0.f, 0.f};
	float m_shiftPostPhase[2] = {0.f, 0.f};

	float m_trueShift[2] = {0.f, 0.f};
	float m_truePhase = 0.f;
	float m_trueDelay = 1.f;

	bool m_prevResetShifter = false;
	bool m_prevResetLfo = false;
};

} // namespace lmms

#endif // LMMS_FREQUENCY_SHIFTER_H

