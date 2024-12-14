/*
 * Bitcrush.h - A native bitcrusher
 *
 * Copyright (c) 2014 Vesa Kivimäki <contact/dot/diizy/at/nbl/dot/fi>
 * Copyright (c) 2006-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifndef BITCRUSH_H
#define BITCRUSH_H

#include "AudioPluginInterface.h"
#include "BitcrushControls.h"
#include "BasicFilters.h"


namespace lmms
{


class BitcrushEffect : public DefaultEffectPluginInterface
{
public:
	BitcrushEffect( Model* parent, const Descriptor::SubPluginFeatures::Key* key );
	~BitcrushEffect() override;

	ProcessStatus processImpl(CoreAudioDataMut inOut) override;

	EffectControls* controls() override
	{
		return &m_controls;
	}

private:
	void sampleRateChanged();
	float depthCrush( float in );
	float noise( float amt );

	BitcrushControls m_controls;
	
	SampleFrame* m_buffer;
	float m_sampleRate;
	StereoLinkwitzRiley m_filter;
	
	float m_bitCounterL;
	float m_rateCoeffL;
	float m_bitCounterR;
	float m_rateCoeffR;
	bool m_rateEnabled;
	
	float m_left;
	float m_right;

	int m_levels;
	float m_levelsRatio;
	bool m_depthEnabled;
	
	float m_inGain;
	float m_outGain;
	float m_outClip;

	bool m_needsUpdate;
	
	int m_silenceCounter;

	friend class BitcrushControls;
};


} // namespace lmms

#endif
