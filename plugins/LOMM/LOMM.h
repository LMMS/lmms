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

#include "AudioPluginInterface.h"
#include "LOMMControls.h"

#include "BasicFilters.h"
#include "lmms_math.h"

namespace lmms
{

constexpr inline float LOMM_MIN_FLOOR = 0.00012589f;// -72 dBFS
constexpr inline float LOMM_MAX_LOOKAHEAD = 20.f;
constexpr inline float LOMM_AUTO_TIME_ADJUST = 5.f;

class LOMMEffect : public DefaultEffectPluginInterface
{
	Q_OBJECT
public:
	LOMMEffect(Model* parent, const Descriptor::SubPluginFeatures::Key* key);
	~LOMMEffect() override = default;

	ProcessStatus processImpl(CoreAudioDataMut inOut) override;

	EffectControls* controls() override
	{
		return &m_lommControls;
	}
	
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
	
	BasicFilters<2> m_ap;
	
	bool m_needsUpdate;
	float m_coeffPrecalc;
	
	std::array<std::array<float, 2>, 3> m_yL;
	std::array<std::array<float, 2>, 3> m_rms;
	std::array<std::array<float, 2>, 3> m_gainResult;
	
	std::array<std::array<float, 2>, 3> m_displayIn;
	std::array<std::array<float, 2>, 3> m_displayOut;
	
	std::array<float, 2> m_crestPeakVal;
	std::array<float, 2> m_crestRmsVal;
	std::array<float, 2> m_crestFactorVal;
	float m_crestTimeConst = 0.0f;
	
	std::array<std::array<float, 2>, 3> m_prevOut;
	
	std::array<std::array<std::vector<float>, 2>, 3> m_inLookBuf;
	std::array<std::array<std::vector<float>, 2>, 3> m_scLookBuf;
	
	int m_lookWrite = 0;
	int m_lookBufLength = 0;
	
	friend class LOMMControls;
	friend class gui::LOMMControlDialog;
};


} // namespace lmms

#endif // LMMS_LOMM_H
