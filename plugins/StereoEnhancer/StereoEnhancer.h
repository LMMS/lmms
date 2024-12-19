/*
 * StereoEnhancer.h - stereo-enhancer-effect-plugin
 *
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


#ifndef _STEREO_ENHANCER_H
#define _STEREO_ENHANCER_H

#include "AudioPluginInterface.h"
#include "DspEffectLibrary.h"
#include "StereoEnhancerControls.h"

namespace lmms
{


class StereoEnhancerEffect : public DefaultEffectPluginInterface
{
public:
	StereoEnhancerEffect( Model * parent,
	                      const Descriptor::SubPluginFeatures::Key * _key );
	~StereoEnhancerEffect() override;

	ProcessStatus processImpl(CoreAudioDataMut inOut) override;

	EffectControls * controls() override
	{
		return( &m_bbControls );
	}

	void clearMyBuffer();


private:
	DspEffectLibrary::StereoEnhancer m_seFX;
	
	SampleFrame* m_delayBuffer;
	int m_currFrame;
	
	StereoEnhancerControls m_bbControls;

	friend class StereoEnhancerControls;
} ;


} // namespace lmms

#endif
