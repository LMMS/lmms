/*
 * LOMM.h
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


#ifndef LMMS_LOMM_H
#define LMMS_LOMM_H

#include "LOMMControls.h"
#include "Effect.h"

#include "BasicFilters.h"
#include "lmms_math.h"

namespace lmms
{

constexpr inline float LOMM_MIN_FLOOR = 0.00012589;// -72 dBFS
constexpr inline float LOMM_MAX_LOOKAHEAD = 20.f;
constexpr inline float LOMM_AUTO_TIME_ADJUST = 5.f;

class LOMMEffect : public Effect
{
	Q_OBJECT
public:
	LOMMEffect(Model* parent, const Descriptor::SubPluginFeatures::Key* key);
	~LOMMEffect() override = default;
	bool processAudioBuffer(sampleFrame* buf, const fpp_t frames) override;

	EffectControls* controls() override
	{
		return &m_lommControls;
	}
	
	void clearFilterHistories();
	
	inline float msToCoeff(float ms)
	{
		return (ms == 0) ? 0 : exp(m_coeffPrecalc / ms);
	}

private slots:
	void changeSampleRate();

private:
	LOMMControls m_lommControls;
	
	float m_sampleRate;
	
	StereoLinkwitzRiley m_lp1;
	StereoLinkwitzRiley m_lp2;
	
	StereoLinkwitzRiley m_hp1;
	StereoLinkwitzRiley m_hp2;
	
	bool m_needsUpdate;
	float m_coeffPrecalc;
	
	float m_yL[3][2] = {{}};
	float m_rms[3][2] = {{}};
	float m_gainResult[3][2] = {{}};
	
	float m_displayIn[3][2] = {{}};
	float m_displayOut[3][2] = {{}};
	
	float m_crestPeakVal[2] = {};
	float m_crestRmsVal[2] = {};
	float m_crestFactorVal[2] = {};
	float m_crestTimeConst;
	
	float m_prevOut[3][2] = {{}};
	
	std::vector<float> m_inLookBuf[3][2];
	std::vector<float> m_scLookBuf[3][2];
	int m_lookWrite;
	int m_lookBufLength;

	friend class LOMMControls;
	friend class gui::LOMMControlDialog;
};


} // namespace lmms

#endif // LMMS_LOMM_H
