/*
 * PhaserEffect.h
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


#ifndef PHASER_H
#define PHASER_H

#include "Effect.h"
#include "PhaserControls.h"
#include "ValueBuffer.h"
#include "../Flanger/QuadratureLfo.h"
#include "RmsHelper.h"

class PhaserEffect : public Effect
{
public:
	PhaserEffect(Model * parent, const Descriptor::SubPluginFeatures::Key * key);
	virtual ~PhaserEffect();
	virtual bool processAudioBuffer(sampleFrame * buf, const fpp_t frames);

	virtual EffectControls* controls()
	{
		return &m_phaserControls;
	}

	float m_realCutoff[2] = {0, 0};

private:
	inline void calcAllpassFilter(sample_t &outSamp, sample_t inSamp, sample_rate_t Fs, int filtNum, int channel, float b0, float b1);
	inline float detuneWithOctaves(float pitchValue, float detuneValue);
	void changeSampleRate();
	void restartLFO();
	void calcAttack();
	void calcRelease();

	PhaserControls m_phaserControls;

	float m_filtX[32][2][4] = {{{0}}};// [filter number][channel][samples back in time]
	float m_filtY[32][2][4] = {{{0}}};// [filter number][channel][samples back in time]
	std::vector<float> m_filtDelayBuf[2];
	int m_filtFeedbackLoc = 0;

	QuadratureLfo * m_lfo;

	RmsHelper * m_rms [2];

	float m_twoPiOverSr;

	float m_b0[2];
	float m_b1[2];

	float m_currentPeak[2];

	double m_attCoeff;
	double m_relCoeff;

	float m_outGain;
	float m_inGain;

	friend class PhaserControls;

} ;

#endif
