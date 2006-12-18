/*
 * plucked_string_synth.h - declaration of class pluckedStringSynth which
 *                          is a synth for plucked string-sounds
 *
 * Copyright (c) 2004-2006 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifndef _PLUCKED_STRING_SYNTH_H
#define _PLUCKED_STRING_SYNTH_H

#include "instrument.h"


class knob;
class notePlayHandle;


// the actual synth
class pluckSynth
{
public:
	pluckSynth( const float _pitch, const float _pick, const float _pickup,
					const sample_rate_t _sample_rate );

	inline ~pluckSynth( void )
	{
		pluckSynth::freeDelayLine( m_upperRail );
		pluckSynth::freeDelayLine( m_lowerRail );
	}

	inline sample_t nextStringSample( void )
	{
		// Output at pickup position
		sample_t outsamp = rgDlAccess( m_upperRail, m_pickupLoc );
		outsamp += lgDlAccess( m_lowerRail, m_pickupLoc );

		// Sample traveling into "bridge"
		sample_t ym0 = lgDlAccess( m_lowerRail, 1 );
		// Sample to "nut"
		sample_t ypM = rgDlAccess( m_upperRail,
						m_upperRail->length - 2 );

		// String state update

		// Decrement pointer and then update
		rgDlUpdate( m_upperRail, -bridgeReflection( ym0 ) );
		// Update and then increment pointer
		lgDlUpdate( m_lowerRail, -ypM );

		return( outsamp );
	}


private:
	struct delayLine
	{
		sample_t * data;
		int length;
		sample_t * pointer;
		sample_t * end;
	} ;

	delayLine * m_upperRail;
	delayLine * m_lowerRail;
	int m_pickupLoc;

	static delayLine * FASTCALL initDelayLine( int _len );
	static void FASTCALL freeDelayLine( delayLine * _dl );
	static inline void setDelayLine( delayLine * _dl, float * _values,
								float _scale )
	{
		for( int i = 0; i < _dl->length; ++i )
		{
			_dl->data[i] = _scale * _values[i];
		}
	}

	/* lgDlUpdate(dl, insamp);
	* Places "nut-reflected" sample from upper delay-line into
	* current lower delay-line pointer position (which represents
	* x = 0 position).  The pointer is then incremented (i.e. the
	* wave travels one sample to the left), turning the previous
	* position into an "effective" x = L position for the next
	* iteration. */
	static inline void lgDlUpdate( delayLine * _dl, sample_t _insamp )
	{
		register sample_t * ptr = _dl->pointer;
		*ptr = _insamp;
		++ptr;
		if( ptr > _dl->end )
		{
			ptr = _dl->data;
		}
		_dl->pointer = ptr;
	}

	/* rgDlUpdate(dl, insamp);
	* Decrements current upper delay-line pointer position (i.e.
	* the wave travels one sample to the right), moving it to the
	* "effective" x = 0 position for the next iteration.  The
	* "bridge-reflected" sample from lower delay-line is then placed
	* into this position. */
	static inline void rgDlUpdate( delayLine * _dl, sample_t _insamp )
	{
		register sample_t * ptr = _dl->pointer;    
		--ptr;
		if( ptr < _dl->data )
		{
			ptr = _dl->end;
		}
		*ptr = _insamp;
		_dl->pointer = ptr;
	}

	/* dlAccess(dl, position);
	* Returns sample "position" samples into delay-line's past.
	* Position "0" points to the most recently inserted sample. */
	static inline sample_t dlAccess( delayLine * _dl, int _position )
	{
		sample_t * outpos = _dl->pointer + _position;
		while( outpos < _dl->data )
		{
			outpos += _dl->length;
		}
		while( outpos > _dl->end )
		{
			outpos -= _dl->length;
		}
		return( *outpos );
	}

	/*
	*  Right-going delay line:
	*  -->---->---->--- 
	*  x=0
	*  (pointer)
	*  Left-going delay line:
	*  --<----<----<--- 
	*  x=0
	*  (pointer)
	*/

	/* rgDlAccess(dl, position);
	* Returns spatial sample at position "position", where position zero
	* is equal to the current upper delay-line pointer position (x = 0).
	* In a right-going delay-line, position increases to the right, and
	* delay increases to the right => left = past and right = future. */
	static inline sample_t rgDlAccess( delayLine * _dl, int _position )
	{
		return( dlAccess( _dl, _position ) );
	}

	/* lgDlAccess(dl, position);
	* Returns spatial sample at position "position", where position zero
	* is equal to the current lower delay-line pointer position (x = 0).
	* In a left-going delay-line, position increases to the right, and
	* delay DEcreases to the right => left = future and right = past. */
	static inline sample_t lgDlAccess( delayLine * _dl, int _position )
	{
		return( dlAccess( _dl, _position ) );
	}

	static inline sample_t bridgeReflection( sample_t _insamp )
	{
		static sample_t state = 0.0f; // filter memory
		// Implement a one-pole lowpass with feedback coefficient = 0.5
		return( state = state*0.5f + _insamp*0.5f );
	}

} ;




class pluckedStringSynth : public instrument
{
public:
	pluckedStringSynth( instrumentTrack * _channel_track );
	virtual ~pluckedStringSynth();

	virtual void FASTCALL playNote( notePlayHandle * _n,
						bool _try_parallelizing );
	virtual void FASTCALL deleteNotePluginData( notePlayHandle * _n );


	virtual void FASTCALL saveSettings( QDomDocument & _doc,
							QDomElement & _parent );
	virtual void FASTCALL loadSettings( const QDomElement & _this );

	virtual QString nodeName( void ) const;


private:
	knob * m_pickKnob;
	knob * m_pickupKnob;

} ;


#endif
