/*
 * SlewDistortion.h
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

#ifndef LMMS_SLEW_DISTORTION_H
#define LMMS_SLEW_DISTORTION_H

#include "Effect.h"
#include "SlewDistortionControls.h"

#include "BasicFilters.h"
#include "lmms_math.h"
#include "OversamplingHelpers.h"

namespace lmms
{
constexpr inline float SLEW_DISTORTION_MIN_FLOOR = 0.0012589f;// -72 dBFS
constexpr inline float SLEW_DISTORTION_DC_FREQ = 7.f;

class SlewDistortion : public Effect
{
	Q_OBJECT
public:
	SlewDistortion(Model* parent, const Descriptor::SubPluginFeatures::Key* key);
	~SlewDistortion() override = default;
	ProcessStatus processImpl(SampleFrame* buf, const fpp_t frames) override;

	EffectControls* controls() override
	{
		return &m_slewdistortionControls;
	}
	
	float msToCoeff(float ms)
	{
		return (ms == 0) ? 0 : std::exp(m_coeffPrecalc / ms);
	}
private slots:
	void changeSampleRate();
private:
	alignas(16) std::array<float, 4> m_inPeakDisplay = {0};
	alignas(16) std::array<float, 4> m_slewOut = {0};
	alignas(16) std::array<float, 4> m_dcOffset = {0};
	alignas(16) std::array<float, 4> m_inEnv = {0};
	alignas(16) std::array<float, 4> m_outEnv = {0};
	alignas(16) std::array<float, 4> m_outPeakDisplay = {0};
	alignas(16) std::array<std::array<float, 1 << SLEWDIST_MAX_OVERSAMPLE_STAGES>, 2> m_overOuts = {{0}};
	
	float m_sampleRate = 44100.f;
	
	int m_oldOversampleVal = -1;
	float m_coeffPrecalc = 0;
	float m_dcCoeff = 0;
	float m_biasInterpCoef = 0;
	float m_trueBias1 = 0;
	float m_trueBias2 = 0;
	
	std::array<Upsampler<SLEWDIST_MAX_OVERSAMPLE_STAGES>, 2> m_upsampler;
	std::array<Downsampler<SLEWDIST_MAX_OVERSAMPLE_STAGES>, 2> m_downsampler;
	
	StereoLinkwitzRiley m_lp;
	StereoLinkwitzRiley m_hp;
	
	SlewDistortionControls m_slewdistortionControls;
	
	friend class SlewDistortionControls;
	friend class gui::SlewDistortionControlDialog;
};

} // namespace lmms

#endif // LMMS_SLEW_DISTORTION_H
