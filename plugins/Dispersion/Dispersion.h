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


#ifndef DISPERSION_H
#define DISPERSION_H

#include "Effect.h"
#include "DispersionControls.h"
#include "lmms_math.h"

namespace lmms
{

constexpr int MAX_DISPERSION_FILTERS = 999;

class DispersionEffect : public Effect
{
public:
	DispersionEffect(Model* parent, const Descriptor::SubPluginFeatures::Key* key);
	~DispersionEffect() override = default;
	bool processAudioBuffer(sampleFrame* buf, const fpp_t frames) override;

	EffectControls* controls() override
	{
		return &m_dispersionControls;
	}
	
	void runDispersionAP(int filtNum, float apCoeff1, float apCoeff2, sample_t* put);

private:
	DispersionControls m_dispersionControls;
	
	float m_sampleRate;
	
	sample_t m_apX0[MAX_DISPERSION_FILTERS][2] = {{0}};
	sample_t m_apX1[MAX_DISPERSION_FILTERS][2] = {{0}};
	sample_t m_apY0[MAX_DISPERSION_FILTERS][2] = {{0}};
	sample_t m_apY1[MAX_DISPERSION_FILTERS][2] = {{0}};
	
	float m_feedbackVal[2] = {0};
	float m_integrator[2] = {0};
	
	int m_amountVal;

	friend class DispersionControls;
} ;


} // namespace lmms

#endif
