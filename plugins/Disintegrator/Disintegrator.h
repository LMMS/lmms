/*
 * Disintegrator.h
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


#ifndef DISINTEGRATOR_H
#define DISINTEGRATOR_H

#include "DisintegratorControls.h"

#include "BasicFilters.h"
#include "Effect.h"
#include "ValueBuffer.h"


class DisintegratorEffect : public Effect
{
public:
	DisintegratorEffect(Model* parent, const Descriptor::SubPluginFeatures::Key* key);
	bool processAudioBuffer(sampleFrame* buf, const fpp_t frames) override;

	EffectControls* controls() override
	{
		return &m_disintegratorControls;
	}

	void sampleRateChanged();

	inline float realfmod(float k, float n);

	void clearFilterHistories();

private:
	DisintegratorControls m_disintegratorControls;

	std::vector<float> m_inBuf[2];
	int m_inBufLoc = 0;

	float m_sineLoc = 0;

	BasicFilters<2> * m_lp;
	BasicFilters<2> * m_hp;
	bool m_needsUpdate;

	float m_sampleRate;
	float m_sampleRateMult;
	int m_bufferSize = 500;

	friend class DisintegratorControls;

} ;

#endif
