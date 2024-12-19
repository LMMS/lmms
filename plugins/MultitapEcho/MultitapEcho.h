/*
 * MultitapEcho.h - a multitap echo delay plugin
 *
 * Copyright (c) 2014 Vesa Kivimäki <contact/dot/diizy/at/nbl/dot/fi>
 * Copyright (c) 2008-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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
 
#ifndef MULTITAP_ECHO_H
#define MULTITAP_ECHO_H

#include "AudioPluginInterface.h"
#include "MultitapEchoControls.h"
#include "RingBuffer.h"
#include "BasicFilters.h"

namespace lmms
{


class MultitapEchoEffect : public DefaultEffectPluginInterface
{
public:
	MultitapEchoEffect( Model* parent, const Descriptor::SubPluginFeatures::Key* key );
	~MultitapEchoEffect() override;

	ProcessStatus processImpl(CoreAudioDataMut inOut) override;

	EffectControls* controls() override
	{
		return &m_controls;
	}

private:
	void updateFilters( int begin, int end );
	void runFilter( SampleFrame* dst, SampleFrame* src, StereoOnePole & filter, const fpp_t frames );

	inline void setFilterFreq( float fc, StereoOnePole & f )
	{
		const float b1 = expf( -2.0f * F_PI * fc );
		f.setCoeffs( 1.0f - b1, b1 );
	}

	int m_stages;

	MultitapEchoControls m_controls;
	
	float m_amp [32];
	float m_lpFreq [32];

	RingBuffer m_buffer;
	StereoOnePole m_filter [32][4];
	
	float m_sampleRate;
	float m_sampleRatio;
	
	SampleFrame* m_work;

	friend class MultitapEchoControls;

};


} // namespace lmms

#endif
