/*
 * sweep_oscillator.h - sweep-oscillator
 *
 * Copyright (c) 2006 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifndef _SWEEP_OSCILLATOR_H
#define _SWEEP_OSCILLATOR_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "oscillator.h"
#include "effect_lib.h"


template<class FX = effectLib::stereoBypass<> >
class sweepOscillator
{
public:
	sweepOscillator( const FX & _fx = FX() ) :
		m_phase( 0.0f ),
		m_FX( _fx )
	{
	}

	virtual ~sweepOscillator()
	{
	}

	void update( sampleFrame * _ab, const fpab_t _frames, 
					const float _freq1, const float _freq2,
						const float _sample_rate )
	{
		const float df = _freq2 - _freq1;
		for( fpab_t frame = 0; frame < _frames; ++frame )
		{
			sample_t s = oscillator::sinSample( m_phase );
			for( ch_cnt_t ch = 0; ch < DEFAULT_CHANNELS; ++ch )
			{
				_ab[frame][ch] = s;
			}
			m_FX.nextSample( _ab[frame][0], _ab[frame][1] );
			m_phase += ( _freq1 + ( frame * df / _frames ) ) /
								_sample_rate;
		}
	}


private:
	float m_phase;
	FX m_FX;

	inline sample_t getSample( const float _sample );
	inline void FASTCALL recalcPhase( void );

} ;


#endif
