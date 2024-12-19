/*
 * CrossoverEQ.h - A native 4-band Crossover Equalizer 
 * good for simulating tonestacks or simple peakless (flat-band) equalization
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

#ifndef CROSSOVEREQ_H
#define CROSSOVEREQ_H

#include "AudioPluginInterface.h"
#include "CrossoverEQControls.h"
#include "BasicFilters.h"

namespace lmms
{


class CrossoverEQEffect : public DefaultEffectPluginInterface
{
public:
	CrossoverEQEffect( Model* parent, const Descriptor::SubPluginFeatures::Key* key );
	~CrossoverEQEffect() override;

	ProcessStatus processImpl(CoreAudioDataMut inOut) override;

	EffectControls* controls() override
	{
		return &m_controls;
	}

	void clearFilterHistories();
	
private:
	CrossoverEQControls m_controls;

	void sampleRateChanged();

	float m_sampleRate;
	
	float m_gain1;
	float m_gain2;
	float m_gain3;
	float m_gain4;
	
	StereoLinkwitzRiley m_lp1;
	StereoLinkwitzRiley m_lp2;
	StereoLinkwitzRiley m_lp3;
	
	StereoLinkwitzRiley m_hp2;
	StereoLinkwitzRiley m_hp3;
	StereoLinkwitzRiley m_hp4;
	
	SampleFrame* m_tmp1;
	SampleFrame* m_tmp2;
	SampleFrame* m_work;
	
	bool m_needsUpdate;
	
	friend class CrossoverEQControls;
};


} // namespace lmms

#endif
