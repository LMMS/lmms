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


const int DISINTEGRATOR_BUFFER_SIZE = 201;


class DisintegratorEffect : public Effect
{
public:
	DisintegratorEffect(Model* parent, const Descriptor::SubPluginFeatures::Key* key);
	virtual ~DisintegratorEffect();
	virtual bool processAudioBuffer(sampleFrame* buf, const fpp_t frames);

	virtual EffectControls* controls()
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

	StereoLinkwitzRiley m_lp;
	StereoLinkwitzRiley m_hp;
	bool m_needsUpdate;

	friend class DisintegratorControls;

} ;

#endif
