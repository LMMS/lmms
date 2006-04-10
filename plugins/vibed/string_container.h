/* string_container.h - contains a collection of strings
 *
 * Copyright (c) 2006 Danny McRae <khjklujn/at/yahoo/com>
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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */
#ifndef _TWO_STRINGS_H
#define _TWO_STRINGS_H


#include "qt3support.h"

#ifndef QT3

#include <QtCore/QVector>

#else

#include <qvaluevector.h>

#endif


#include "config.h"
#include "types.h"
#include "vibrating_string.h"



class stringContainer
{
public:
	stringContainer(const float _pitch, 
			const sample_rate_t _sample_rate,
			const Uint32 _buffer_length,
			const Uint8 _strings = 9 );
	
	void addString(	Uint8 _harm,
			const float _pick,
			const float _pickup,
			float * _impluse,
			const float _randomize,
			const float _string_loss,
			const float _detune,
			const Uint8 _oversample,
			const bool _state,
			const Uint8 _id );
	
	inline bool exists( Uint8 _id )
	{
		return( m_exists[_id] );
	}
	
	inline ~stringContainer()
	{
		Uint32 strings = m_strings.count();
		for( Uint32 i = 0; i < strings; i++ )
		{
			delete m_strings[i];
		}
	}
	
	inline float getStringSample( Uint8 _string )
	{
		return( m_strings[_string]->nextSample() );
	}
	
private:
	vvector<vibratingString *> m_strings;
	const float m_pitch;
	const sample_rate_t m_sampleRate;
	const Uint32 m_bufferLength;
	vvector<bool> m_exists;
} ;

#endif
