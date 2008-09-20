/*
 * midi_time.h - declaration of class midiTime which provides data-type for
 *               position- and length-variables
 *
 * Copyright (c) 2004-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifndef _MIDI_TIME_H
#define _MIDI_TIME_H

#include "types.h"
#include "export.h"

const int DefaultTicksPerTact = 192;
const int DefaultStepsPerTact = 16;
const int DefaultBeatsPerTact = DefaultTicksPerTact / DefaultStepsPerTact;


class EXPORT midiTime
{
public:
	inline midiTime( const tact _tact, const tick _ticks ) :
		m_ticks( _tact * s_ticksPerTact + _ticks )
	{
	}

	inline midiTime( const tick _ticks = 0 ) :
		m_ticks( _ticks )
	{
	}

	inline midiTime( const midiTime & _t ) :
		m_ticks( _t.m_ticks )
	{
	}

	inline midiTime toNearestTact( void ) const
	{
		if( m_ticks % s_ticksPerTact >= s_ticksPerTact/2 )
		{
			return( ( getTact() + 1 ) * s_ticksPerTact );
		}
		return( getTact() * s_ticksPerTact );
	}

	inline midiTime & operator=( const midiTime & _t )
	{
		m_ticks = _t.m_ticks;
		return( *this );
	}

	inline midiTime & operator+=( const midiTime & _t )
	{
		return( *this = static_cast<int>( *this ) +
						static_cast<int>( _t ) );
	}

	inline midiTime & operator-=( const midiTime & _t )
	{
		return( *this = static_cast<int>( *this ) -
						static_cast<int>( _t ) );
	}

	inline tact getTact( void ) const
	{
		return( m_ticks / s_ticksPerTact );
	}

	inline tact nextFullTact( void ) const
	{
		if( m_ticks % s_ticksPerTact == 0 )
		{
			return( m_ticks / s_ticksPerTact );
		}
		return( m_ticks / s_ticksPerTact + 1 );
	}

	inline void setTicks( tick _t )
	{
		m_ticks = _t;
	}

	inline tick getTicks( void ) const
	{
		return( m_ticks );
	}

	inline operator int( void ) const
	{
		return( m_ticks );
	}

	// calculate number of frame that are needed this time
	inline f_cnt_t frames( const float _frames_per_tick ) const
	{
		if( m_ticks >= 0 )
		{
			return( static_cast<f_cnt_t>( m_ticks *
							_frames_per_tick ) );
		}
		return( 0 );
	}

	static inline midiTime fromFrames( const f_cnt_t _frames,
					const float _frames_per_tick )
	{
		return( midiTime( static_cast<int>( _frames /
							_frames_per_tick ) ) );
	}


	static tick ticksPerTact( void )
	{
		return( s_ticksPerTact );
	}

	static int stepsPerTact( void )
	{
		return( ticksPerTact() / DefaultBeatsPerTact );
	}

	static void setTicksPerTact( tick _tpt )
	{
		s_ticksPerTact = _tpt;
	}

private:
	tick m_ticks;

	static tick s_ticksPerTact;

} ;


#endif

