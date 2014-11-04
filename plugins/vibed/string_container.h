/* string_container.h - contains a collection of strings
 *
 * Copyright (c) 2006 Danny McRae <khjklujn/at/yahoo/com>
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

#ifndef _STRING_CONTAINER_H
#define _STRING_CONTAINER_H

#include <QtCore/QVector>

#include "vibrating_string.h"



class stringContainer
{
public:
	stringContainer(const float _pitch, 
			const sample_rate_t _sample_rate,
			const int _buffer_length,
			const int _strings = 9 );
	
	void addString(	int _harm,
			const float _pick,
			const float _pickup,
			const float * _impluse,
			const float _randomize,
			const float _string_loss,
			const float _detune,
			const int _oversample,
			const bool _state,
			const int _id );
	
	bool exists( int _id ) const
	{
		return m_exists[_id];
	}
	
	~stringContainer()
	{
		int strings = m_strings.count();
		for( int i = 0; i < strings; i++ )
		{
			delete m_strings[i];
		}
	}
	
	float getStringSample( int _string )
	{
		return m_strings[_string]->nextSample();
	}
	
private:
	QVector<vibratingString *> m_strings;
	const float m_pitch;
	const sample_rate_t m_sampleRate;
	const int m_bufferLength;
	QVector<bool> m_exists;
} ;

#endif
