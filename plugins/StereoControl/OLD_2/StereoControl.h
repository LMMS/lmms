/*
 * StereoControl.h
 *
 * Copyright (c) 2020 Lost Robot <r94231@gmail.com>
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


#ifndef STEREOCONTROL_H
#define STEREOCONTROL_H

#include "Effect.h"
#include "StereoControlControls.h"
#include "ValueBuffer.h"

class StereoControlEffect : public Effect
{
public:
	StereoControlEffect( Model* parent, const Descriptor::SubPluginFeatures::Key* key );
	virtual ~StereoControlEffect();
	virtual bool processAudioBuffer( sampleFrame* buf, const fpp_t frames );

	virtual EffectControls* controls()
	{
		return &m_stereocontrolControls;
	}


private:
	inline sample_t calcAllpassFilter(sample_t inSamp, sample_rate_t Fs, int filtNum, int channel, float a, float b);
	inline void multimodeFilter(sample_t in, float g, sample_t &lp, sample_t &hp, sample_t &filtBuf);
	inline float multimodeCoeff(float freq);

	StereoControlControls m_stereocontrolControls;

	float m_filtX[2][2][2] = {{{0}}};// [filter number][channel][samples back in time]
	float m_filtY[2][2][2] = {{{0}}};
	float m_filtA = 0;
	float m_filtB = 0;
	std::vector<float> m_delayBuf[2];
	int m_delayBufSize = 0;
	int m_delayIndex[2] = {0};

	std::vector<float> m_haasBuf[2];
	int m_haasBufSize = 0;
	int m_haasIndex[2] = {0};

	float m_sampleRate = 0;

	float m_dcInfo[2] = {0};

	float m_monoBassFilter[2] = {0};
	float m_stereoizerFilter[2][2] = {0};

	float m_haasSpectralPanFilter[2] = {0};

	friend class StereoControlControls;

} ;

#endif
