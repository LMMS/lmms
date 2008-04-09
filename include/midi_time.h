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

const int DefaultTicksPerTact = 192;

class midiTime
{
public:
	inline midiTime( const tact _tact, const tick _ticks ) :
		m_tact( _tact ),
		m_ticks( _ticks )
	{
	}

	inline midiTime( const int _abs = 0 ) :
		m_tact( _abs / DefaultTicksPerTact ),
		m_ticks( _abs % DefaultTicksPerTact )
	{
	}

	inline midiTime( const midiTime & _t )
	{
		*this = _t;
	}

	inline midiTime toNearestTact( void ) const
	{
		if( m_ticks >= DefaultTicksPerTact/2 )
		{
			return( m_tact * DefaultTicksPerTact +
							DefaultTicksPerTact );
		}
		return( m_tact * DefaultTicksPerTact );
	}

	inline midiTime & operator=( const midiTime & _t )
	{
		m_tact = _t.m_tact;
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

	inline void setTact( tact _t )
	{
		m_tact = _t;
	}

	inline tact getTact( void ) const
	{
		return( m_tact );
	}

	inline void setTicks( tick _t )
	{
		m_ticks = _t;
	}

	inline tick getTicks( void ) const
	{
		return( m_ticks );
	}

	// converts time-class in an absolute value, useful for calculations,
	// comparisons and so on...
	inline operator int( void ) const
	{
		return( static_cast<int>( m_tact ) * DefaultTicksPerTact +
					static_cast<int>( m_ticks ) );
	}

	// calculate number of frame that are needed this time
	inline f_cnt_t frames( const float _frames_per_tick ) const
	{
		if( m_tact >= 0 )
		{
			return( static_cast<f_cnt_t>(
				( m_tact * DefaultTicksPerTact + m_ticks ) *
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


private:
	tact m_tact;
	tick m_ticks;

} ;


#endif

