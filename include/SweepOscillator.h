/*
 * SweepOscillator.h - sweeping oscillator
 *
 * Copyright (c) 2006-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include "Oscillator.h"
#include "effect_lib.h"


template<class FX = effectLib::stereoBypass>
class SweepOscillator
{
public:
	SweepOscillator( const FX & _fx = FX() ) :
		m_phase( 0.0f ),
		m_FX( _fx )
	{
	}

	virtual ~SweepOscillator()
	{
	}

	void update( sampleFrame * _ab, const fpp_t _frames, 
					const float _freq1, const float _freq2,
						const float _sample_rate )
	{
		const float df = _freq2 - _freq1;
		for( fpp_t frame = 0; frame < _frames; ++frame )
		{
			const sample_t s = Oscillator::sinSample( m_phase );
			_ab[frame][0] = s;
			_ab[frame][1] = s;
			m_FX.nextSample( _ab[frame][0], _ab[frame][1] );
			m_phase += ( _freq1 + ( frame * df / _frames ) ) /
								_sample_rate;
		}
	}


private:
	float m_phase;
	FX m_FX;

//	inline sample_t getSample( const float _sample );
//	inline void recalcPhase();

} ;


#endif
