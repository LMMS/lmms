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

#ifndef LMMS_FREQUENCY_SHIFTER_EFFECT_H
#define LMMS_FREQUENCY_SHIFTER_EFFECT_H

#include "Effect.h"
#include "FrequencyShifterControls.h"

#include "HilbertTransform.h"
#include "interpolation.h"
#include "lmms_math.h"

#include <array>
#include <cmath>
#include <numbers>
#include <vector>

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
	std::array<float, 2> getHermiteSample(int indexFloor, float fraction)
	{
		const int size = m_ringBufSize;

		const int i0 = (indexFloor == 0) ? (size - 1) : (indexFloor - 1);
		const int i1 = indexFloor;

		int i2 = indexFloor + 1;
		int i3 = indexFloor + 2;
		if (i2 >= size) i2 -= size;
		if (i3 >= size) i3 -= size;

		std::array<float, 2> out;

		for (int ch = 0; ch < 2; ++ch)
		{
			const float v0 = m_ringBuf[i0][ch];
			const float v1 = m_ringBuf[i1][ch];
			const float v2 = m_ringBuf[i2][ch];
			const float v3 = m_ringBuf[i3][ch];

			out[ch] = hermiteInterpolate(v0, v1, v2, v3, fraction);
		}

		return out;
	}
	
	HilbertIIRFloat<2> m_hilbert1;
	HilbertIIRFloat<2> m_hilbert2;
	
	std::vector<std::array<float, 2>> m_ringBuf;
	
	std::array<float, 2> m_phase{};
	std::array<float, 2> m_trueShift{};
	std::array<float, 2> m_dampState{};
	std::array<float, 2> m_toneState{};
	
	float m_lfoPhase{};
	float m_truePhase{};
	float m_trueDelay{1.f};
	
	float m_twoPiOverSr{};
	float m_sampleRate{};
	
	int m_ringBufSize{};
	int m_writeIndex{};
	
	bool m_prevResetShifter{};
	bool m_prevResetLfo{};

	FrequencyShifterControls m_controls;
};

} // namespace lmms

#endif // LMMS_FREQUENCY_SHIFTER_EFFECT_H

