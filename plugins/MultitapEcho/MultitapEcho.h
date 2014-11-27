/*
 * MultitapEcho.h - a multitap echo delay plugin
 *
 * Copyright (c) 2014 Vesa Kivimäki <contact/dot/diizy/at/nbl/dot/fi>
 * Copyright (c) 2008-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 *
 * This file is part of Linux MultiMedia Studio - http://lmms.sourceforge.net
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

#include "Effect.h"
#include "MultitapEchoControls.h"
#include "ValueBuffer.h"
#include "RingBuffer.h"
#include "lmms_math.h"

class OnePole
{
public:
	OnePole()
	{
		m_a0 = 1.0; 
		m_b1 = 0.0; 
		m_z1 = 0.0;
	}
	virtual ~OnePole() {}
	
	inline void setFc( float fc )
	{
		m_b1 = expf( -2.0f * F_PI * fc );
		m_a0 = 1.0f - m_b1;
	}
	
	inline float update( float s )
	{
		return m_z1 = s * m_a0 + m_z1 * m_b1;
	}
	
private:
	float m_a0, m_b1, m_z1;
};

typedef OnePole StereoOnePole [2];

class MultitapEchoEffect : public Effect
{
public:
	MultitapEchoEffect( Model* parent, const Descriptor::SubPluginFeatures::Key* key );
	virtual ~MultitapEchoEffect();
	virtual bool processAudioBuffer( sampleFrame* buf, const fpp_t frames );

	virtual EffectControls* controls()
	{
		return &m_controls;
	}

private:
	void updateFilters( int begin, int end );
	void runFilter( sampleFrame * dst, sampleFrame * src, StereoOnePole & filter, const fpp_t frames );

	MultitapEchoControls m_controls;
	
	float m_amp [20];
	float m_lpFreq [20];

	RingBuffer m_buffer;
	StereoOnePole m_filter [20];
	
	float m_sampleRate;
	float m_sampleRatio;

	friend class MultitapEchoControls;

};


#endif
