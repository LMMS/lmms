/*
 * ValueBuffer.h - a container class for passing buffers of model values around
 *
 * Copyright (c) 2014 Vesa Kivimäki <contact/dot/diizy/at/nbl/dot/fi>
 * Copyright (c) 2008-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef VALUE_BUFFER_H
#define VALUE_BUFFER_H

#include "interpolation.h"
#include <string.h>

class ValueBuffer
{
public:
	ValueBuffer()
	{
		m_values = NULL;
		m_length = 0;
	}
	
	ValueBuffer( int length )
	{
		m_values = new float[length];
		m_length = length;
	}

	ValueBuffer( float * values, int length )
	{
		m_values = new float[length];
		m_length = length;		
		memcpy( m_values, values, sizeof(float) * length );
	}

	ValueBuffer( float value, int length )
	{
		m_values = new float[length];
		m_length = length;
		for( int i = 0; i < length; i++ )
		{
			m_values[i] = value;
		}
	}

	virtual ~ValueBuffer() 
	{
		delete[] m_values;
	}

	void clear()
	{
		delete[] m_values;
		m_values = NULL;
		m_length = 0;
	}
	
	void fill( float value )
	{
		for( int i = 0; i < m_length; i++ )
		{
			m_values[i] = value;
		}
	}

	float value( int offset ) const
	{
		return m_values[ offset % m_length ];
	}
	
	void setValue( int offset, float value )
	{
		m_values[ offset % m_length ] = value;
	}

	float * values() const
	{
		return m_values;
	}
	
	void setValues( float * values )
	{
		m_values = values;
	}
	
	int length() const
	{
		return m_length;
	}
	
	void setLength( const int length )
	{
		m_length = length;
	}

	void interpolate( float start, float end )
	{
		float f = 0.0f;
		const float fstep = 1.0f / static_cast<float>( m_length );
		for( int i = 0; i < m_length; i++ )
		{
			f += fstep;
			m_values[i] = linearInterpolate( start, end, f );
		}
	}

	static ValueBuffer interpolatedBuffer( float start, float end, int length )
	{
		ValueBuffer vb = ValueBuffer( length );
		vb.interpolate( start, end );
		return vb;
	}

private:
	float * m_values;
	int m_length;
};

#endif
