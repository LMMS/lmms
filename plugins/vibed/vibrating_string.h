/*
 * vibrating_string.h - model of a vibrating string lifted from pluckedSynth
 *
 * Copyright (c) 2006-2007 Danny McRae <khjklujn/at/yahoo/com>
 * 
 * This file is part of LMMS - http://lmms.io
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
#ifndef _VIBRATING_STRING_H
#define _VIBRATING_STRING_H

#include <stdio.h>
#include <stdlib.h>

#include "lmms_basics.h"

class vibratingString
{

public:
	vibratingString(	float _pitch, 
				float _pick, 
				float _pickup,
				float * impluse,
				int _len,
				sample_rate_t _sample_rate,
				int _oversample,
				float _randomize,
				float _string_loss,
				float _detune,
				bool _state );
	
	inline ~vibratingString()
	{
		delete[] m_outsamp;
		delete[] m_impulse;
		vibratingString::freeDelayLine( m_fromBridge );
		vibratingString::freeDelayLine( m_toBridge );
	}

	inline sample_t nextSample()
	{	
		sample_t ym0;
		sample_t ypM;
		for( int i = 0; i < m_oversample; i++)
		{
			// Output at pickup position
			m_outsamp[i] = fromBridgeAccess( m_fromBridge, 
								m_pickupLoc );
			m_outsamp[i] += toBridgeAccess( m_toBridge, 
								m_pickupLoc );
		
			// Sample traveling into "bridge"
			ym0 = toBridgeAccess( m_toBridge, 1 );
			// Sample to "nut"
			ypM = fromBridgeAccess( m_fromBridge,
						m_fromBridge->length - 2 );

			// String state update

			// Decrement pointer and then update
			fromBridgeUpdate( m_fromBridge, 
						-bridgeReflection( ym0 ) );
			// Update and then increment pointer
			toBridgeUpdate( m_toBridge, -ypM );
		}
		return( m_outsamp[m_choice] );
	}

private:
	struct delayLine
	{
		sample_t * data;
		int length;
		sample_t * pointer;
		sample_t * end;
	} ;

	delayLine * m_fromBridge;
	delayLine * m_toBridge;
	int m_pickupLoc;
	int m_oversample;
	float m_randomize;
	float m_stringLoss;
	
	float * m_impulse;
	int m_choice;
	float m_state;
	
	sample_t * m_outsamp;

	delayLine * initDelayLine( int _len, int _pick );
	static void freeDelayLine( delayLine * _dl );
	void resample( float *_src, f_cnt_t _src_frames, f_cnt_t _dst_frames );
	
	/* setDelayLine initializes the string with an impulse at the pick
	 * position unless the impulse is longer than the string, in which
	 * case the impulse gets truncated. */
	inline void setDelayLine( delayLine * _dl, 
					int _pick,
					const float * _values, 
					int _len,
					float _scale,
					bool _state )
	{
		float r;
		float offset;
		
		if( not _state )
		{
			for( int i = 0; i < _pick; i++ )
			{
				r = static_cast<float>( rand() ) /
						RAND_MAX;
				offset =  ( m_randomize / 2.0f -
						m_randomize ) * r;
				_dl->data[i] = _scale *
						_values[_dl->length - i] +
						offset;
			}
			for( int i = _pick; i < _dl->length; i++ )
			{
				r = static_cast<float>( rand() ) /
						RAND_MAX;
				offset =  ( m_randomize / 2.0f -
						m_randomize ) * r;
				_dl->data[i] = _scale * 
						_values[i - _pick]  + offset ;
			}
		}
		else
		{
			if( _len + _pick > _dl->length )
			{
				for( int i = _pick; i < _dl->length; i++ )
				{
					r = static_cast<float>( rand() ) /
							RAND_MAX;
					offset =  ( m_randomize / 2.0f -
							m_randomize ) * r;
					_dl->data[i] = _scale *
							_values[i-_pick] +
							offset;
				}
			}
			else
			{
				for( int i = 0; i < _len; i++ )
				{
					r = static_cast<float>( rand() ) /
							RAND_MAX;
					offset =  ( m_randomize / 2.0f -
							m_randomize ) * r;
					_dl->data[i+_pick] = _scale *
								_values[i] +
								offset;
				}
			}
		}
	}

	/* toBridgeUpdate(dl, insamp);
	* Places "nut-reflected" sample from upper delay-line into
	* current lower delay-line pointer position (which represents
	* x = 0 position).  The pointer is then incremented (i.e. the
	* wave travels one sample to the left), turning the previous
	* position into an "effective" x = L position for the next
	* iteration. */
	inline void toBridgeUpdate( delayLine * _dl, sample_t _insamp )
	{
		register sample_t * ptr = _dl->pointer;
		*ptr = _insamp * m_stringLoss;
		++ptr;
		if( ptr > _dl->end )
		{
			ptr = _dl->data;
		}
		_dl->pointer = ptr;
	}

	/* fromBridgeUpdate(dl, insamp);
	* Decrements current upper delay-line pointer position (i.e.
	* the wave travels one sample to the right), moving it to the
	* "effective" x = 0 position for the next iteration.  The
	* "bridge-reflected" sample from lower delay-line is then placed
	* into this position. */
	inline void fromBridgeUpdate( delayLine * _dl, 
							sample_t _insamp )
	{
		register sample_t * ptr = _dl->pointer;
		--ptr;
		if( ptr < _dl->data )
		{
			ptr = _dl->end;
		}
		*ptr = _insamp * m_stringLoss;
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

	/* fromBridgeAccess(dl, position);
	* Returns spatial sample at position "position", where position zero
	* is equal to the current upper delay-line pointer position (x = 0).
	* In a right-going delay-line, position increases to the right, and
	* delay increases to the right => left = past and right = future. */
	static inline sample_t fromBridgeAccess( delayLine * _dl, 
								int _position )
	{
		return( dlAccess( _dl, _position ) );
	}

	/* toBridgeAccess(dl, position);
	* Returns spatial sample at position "position", where position zero
	* is equal to the current lower delay-line pointer position (x = 0).
	* In a left-going delay-line, position increases to the right, and
	* delay DEcreases to the right => left = future and right = past. */
	static inline sample_t toBridgeAccess( delayLine * _dl, int _position )
	{
		return( dlAccess( _dl, _position ) );
	}

	inline sample_t bridgeReflection( sample_t _insamp )
	{
		return( m_state = ( m_state + _insamp ) * 0.5 );
	}

} ;

#endif
