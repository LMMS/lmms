/*
 * plucked_string_sytn.h - declaration of class pluckedStringSynth which
 *                         is a synth for plucked string-sounds
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


#ifndef _PLUCKED_STRING_SYNTH_H
#define _PLUCKED_STRING_SYNTH_H

#include "instrument.h"


class knob;
class notePlayHandle;


// the actual synth
class pluckSynth
{
public:
	pluckSynth( float _pitch, float _pick, float _pickup );

	inline ~pluckSynth( void )
	{
		pluckSynth::freeDelayLine( m_upperRail );
		pluckSynth::freeDelayLine( m_lowerRail );
	}

	inline sampleType nextStringSample( void )
	{
		// Output at pickup position
		sampleType outsamp = rgDlAccess( m_upperRail, m_pickupLoc );
		outsamp += lgDlAccess( m_lowerRail, m_pickupLoc );

		// Sample traveling into "bridge"
		sampleType ym0 = lgDlAccess( m_lowerRail, 1 );
		// Sample to "nut"
		sampleType ypM = rgDlAccess( m_upperRail,
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
		sampleType * data;
		int length;
		sampleType * pointer;
		sampleType * end;
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
	static inline void lgDlUpdate( delayLine * _dl, sampleType _insamp )
	{
		register sampleType * ptr = _dl->pointer;
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
	static inline void rgDlUpdate( delayLine * _dl, sampleType _insamp )
	{
		register sampleType * ptr = _dl->pointer;    
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
	static inline sampleType dlAccess( delayLine * _dl, int _position )
	{
		sampleType * outpos = _dl->pointer + _position;
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
	static inline sampleType rgDlAccess( delayLine * _dl, int _position )
	{
		return( dlAccess( _dl, _position ) );
	}

	/* lgDlAccess(dl, position);
	* Returns spatial sample at position "position", where position zero
	* is equal to the current lower delay-line pointer position (x = 0).
	* In a left-going delay-line, position increases to the right, and
	* delay DEcreases to the right => left = future and right = past. */
	static inline sampleType lgDlAccess( delayLine * _dl, int _position )
	{
		return( dlAccess( _dl, _position ) );
	}

	static inline sampleType bridgeReflection( sampleType _insamp )
	{
		static sampleType state = 0.0f; // filter memory
		// Implement a one-pole lowpass with feedback coefficient = 0.5
		return( state = state*0.5f + _insamp*0.5f );
	}

} ;




class pluckedStringSynth : public instrument
{
public:
	pluckedStringSynth(channelTrack * _channel_track );
	virtual ~pluckedStringSynth();

	virtual void FASTCALL playNote( notePlayHandle * _n );
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
