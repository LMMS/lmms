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

#include "PhaserControls.h"

#include "QuadratureLfo.h"
#include "Effect.h"
#include "ValueBuffer.h"

class PhaserEffect : public Effect
{
	Q_OBJECT
public:
	PhaserEffect(Model * parent, const Descriptor::SubPluginFeatures::Key * key);
	~PhaserEffect() override;
	bool processAudioBuffer(sampleFrame * buf, const fpp_t frames) override;

	EffectControls* controls() override
	{
		return &m_phaserControls;
	}

	float getCutoff(int channel)
	{
		return m_displayCutoff[channel];
	}

private slots:
	void calcAttack();
	void calcRelease();
	void calcOutGain();
	void calcInGain();
	void calcPhase();
	void changeSampleRate();
	void restartLFO();

private:
	sample_t calcAllpassFilter(sample_t inSamp, sample_rate_t Fs, int filtNum, int channel, float b0, float b1);
	static float detuneWithOctaves(float pitchValue, float detuneValue);

	PhaserControls m_phaserControls;

	struct FilterState
	{
		float x[2] = {};
		float y[2] = {};
	};
	FilterState m_filt[32][2] = {{}};// [filter number][channel]
	std::vector<float> m_filtDelayBuf[2];
	int m_delayBufSize;
	int m_filtFeedbackLoc = 0;

	QuadratureLfo * m_lfo;

	float m_twoPiOverSr;
	float m_dcCoeff;

	float m_currentPeak[2];

	double m_attCoeff;
	double m_relCoeff;

	float m_outGain;
	float m_inGain;

	float m_sampAvg[2] = {0};
	float m_oscillateTracker1[2] = {};
	float m_oscillateTracker2[2] = {};

	sampleFrame m_displayCutoff;
	sampleFrame m_realCutoff;

	float lastSecondAdd[2] = {};

	float m_dcTimeConst;
	float m_oscillateTimeConst;

	float m_aliasFlip = 1;

	friend class PhaserControls;

} ;

#endif

