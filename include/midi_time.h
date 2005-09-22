/*
 * midi_time.h - declaration of class midiTime which provides data-type for
 *               position- and length-variables
 *
 * Linux MultiMedia Studio
 * Copyright (c) 2004-2005 Tobias Doerffel <tobydox@users.sourceforge.net>
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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */


#ifndef _MIDI_TIME_H
#define _MIDI_TIME_H

#include "types.h"


class midiTime
{
public:
	inline midiTime( tact _tact, tact64th _tact_64th ) :
		m_tact( _tact ),
		m_tact64th( _tact_64th )
	{
	}

	inline midiTime( Sint32 _abs = 0 ) :
		m_tact( _abs / 64 ),
		m_tact64th( _abs % 64 )
	{
	}

	inline midiTime( const midiTime & _t )
	{
		*this = _t;
	}

	inline midiTime & operator=( const midiTime & _t )
	{
		m_tact = _t.m_tact;
		m_tact64th = _t.m_tact64th;
		return( *this );
	}

	inline midiTime & operator+=( const midiTime & _t )
	{
		return( *this = static_cast<Sint32>( *this ) +
						static_cast<Sint32>( _t ) );
	}

	inline midiTime & operator-=( const midiTime & _t )
	{
		return( *this = static_cast<Sint32>( *this ) -
						static_cast<Sint32>( _t ) );
	}

	inline void setTact( tact _t )
	{
		m_tact = _t;
	}

	inline tact getTact( void ) const
	{
		return( m_tact );
	}

	inline void setTact64th( tact64th _t )
	{
		m_tact64th = _t;
	}

	inline tact64th getTact64th( void ) const
	{
		return( m_tact64th );
	}

	// converts time-class in an absolute value, useful for calculations,
	// comparisons and so on...
	inline operator Sint32( void ) const
	{
		return( static_cast<Sint32>( m_tact ) * 64 +
					static_cast<Sint32>( m_tact64th ) );
	}

	// calculate number of frame that are needed this time
	inline Uint32 frames( float _frames_per_tact ) const
	{
		if( m_tact >= 0 )
		{
			return( static_cast<Uint32>( m_tact *
							_frames_per_tact +
							m_tact64th *
							_frames_per_tact /
							64.0f ) );
		}
		return( 0 );
	}


private:
	tact m_tact;
	tact64th m_tact64th;

} ;


#endif

