/*
 * Compressor.h
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


#ifndef COMPRESSOR_H
#define COMPRESSOR_H

#include "AudioPluginInterface.h"
#include "CompressorControls.h"

namespace lmms
{


constexpr float COMP_LOG = -2.2f;

class CompressorEffect : public DefaultEffectPluginInterface
{
	Q_OBJECT
public:
	CompressorEffect(Model* parent, const Descriptor::SubPluginFeatures::Key* key);
	~CompressorEffect() override = default;

	ProcessStatus processImpl(CoreAudioDataMut inOut) override;
	void processBypassedImpl() override;

	EffectControls* controls() override
	{
		return &m_compressorControls;
	}

private slots:
	void calcAutoMakeup();
	void calcAttack();
	void calcRelease();
	void calcAutoAttack();
	void calcAutoRelease();
	void calcHold();
	void calcOutGain();
	void calcRatio();
	void calcRange();
	void resizeRMS();
	void calcLookaheadLength();
	void calcThreshold();
	void calcKnee();
	void calcInGain();
	void calcTiltCoeffs();
	void calcMix();
	void changeSampleRate();
	void redrawKnee();

private:
	CompressorControls m_compressorControls;

	float msToCoeff(float ms);

	inline void calcTiltFilter(sample_t inputSample, sample_t &outputSample, int filtNum);
	inline int realmod(int k, int n);
	inline float realfmod(float k, float n);

	enum class StereoLinkMode { Unlinked, Maximum, Average, Minimum, Blend };

	std::array<std::vector<float>, 2> m_inLookBuf;
	std::array<std::vector<float>, 2> m_scLookBuf;
	int m_lookWrite;
	int m_lookBufLength;

	float m_attCoeff;
	float m_relCoeff;
	float m_autoAttVal;
	float m_autoRelVal;

	int m_holdLength = 0;
	int m_holdTimer[2] = {0, 0};

	int m_lookaheadLength;
	float m_thresholdAmpVal;
	float m_autoMakeupVal;
	float m_outGainVal;
	float m_inGainVal;
	float m_rangeVal;
	float m_tiltVal;
	float m_mixVal;

	float m_coeffPrecalc;

	SampleFrame m_maxLookaheadVal;

	int m_maxLookaheadTimer[2] = {1, 1};
	
	float m_rmsTimeConst;
	float m_rmsVal[2] = {0, 0};

	float m_crestPeakVal[2] = {0, 0};
	float m_crestRmsVal[2] = {0, 0};
	float m_crestFactorVal[2] = {0, 0};
	float m_crestTimeConst;

	float m_tiltOut[2] = {0};

	bool m_cleanedBuffers = false;

	float m_sampleRate;

	float m_lgain;
	float m_hgain;
	float m_a0;
	float m_b1;

	float m_prevOut[2] = {0};

	float m_yL[2];
	float m_gainResult[2];
	float m_displayPeak[2];
	float m_displayGain[2];

	float m_kneeVal;
	float m_thresholdVal;
	float m_ratioVal;

	bool m_redrawKnee = true;
	bool m_redrawThreshold = true;

	friend class CompressorControls;
	friend class gui::CompressorControlDialog;
} ;


} // namespace lmms

#endif
