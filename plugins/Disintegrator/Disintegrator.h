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

#include "Effect.h"
#include "ValueBuffer.h"


const int DISINTEGRATOR_BUFFER_SIZE = 200;


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

	inline float realfmod(float k, float n);

	inline void calcLowpassFilter(sample_t &outSamp, sample_t inSamp, int channel, float lpCutoff, sample_rate_t sampleRate);
	inline void calcHighpassFilter(sample_t &outSamp, sample_t inSamp, int channel, float lpCutoff, sample_rate_t sampleRate);

	float m_filtLPX[2][3] = {{0}};// [filter number][samples back in time]
	float m_filtLPY[2][3] = {{0}};// [filter number][samples back in time]
	float m_prevLPCutoff[2] = {0};

	float m_LPa0;
	float m_LPb0;
	float m_LPb1;
	float m_LPa1;
	float m_LPa2;

	float m_filtHPX[2][3] = {{0}};// [filter number][samples back in time]
	float m_filtHPY[2][3] = {{0}};// [filter number][samples back in time]
	float m_prevHPCutoff[2] = {0};

	float m_HPa0;
	float m_HPb0;
	float m_HPb1;
	float m_HPa1;
	float m_HPa2;

private:
	DisintegratorControls m_disintegratorControls;

	std::vector<float> m_inBuf[2];
	int m_inBufLoc = 0;

	float m_sineLoc = 0;

	friend class DisintegratorControls;

} ;

#endif
