/*
 * Dispersion.h
 *
 * Copyright (c) 2023 Lost Robot <r94231/at/gmail/dot/com>
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


#ifndef LMMS_DISPERSION_H
#define LMMS_DISPERSION_H

#include "DispersionControls.h"
#include "Effect.h"


namespace lmms
{

constexpr inline int MAX_DISPERSION_FILTERS = 999;

class DispersionEffect : public Effect
{
public:
	DispersionEffect(Model* parent, const Descriptor::SubPluginFeatures::Key* key);
	~DispersionEffect() override = default;

	ProcessStatus processImpl(SampleFrame* buf, const fpp_t frames) override;

	EffectControls* controls() override
	{
		return &m_dispersionControls;
	}
	
	void runDispersionAP(const int filtNum, const float apCoeff1, const float apCoeff2, std::array<sample_t, 2> &put);

private:
	DispersionControls m_dispersionControls;
	
	float m_sampleRate;
	
	int m_amountVal;
	
	using Filter = std::array<sample_t, MAX_DISPERSION_FILTERS * 2>;
	struct FilterState {
		Filter x0{};
		Filter x1{};
		Filter y0{};
		Filter y1{};
	};
	FilterState m_state = {};
	
	std::array<float, 2> m_feedbackVal{};
	std::array<float, 2> m_integrator{};

	friend class DispersionControls;
};


} // namespace lmms

#endif // LMMS_DISPERSION_H
