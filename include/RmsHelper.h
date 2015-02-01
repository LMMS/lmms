/*
 * RmsHelper.h - helper class for calculating RMS
 *
 * Copyright (c) 2014 Vesa Kivimäki <contact/dot/diizy/at/nbl/dot/fi>
 * Copyright (c) 2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef RMS_HELPER_H
#define RMS_HELPER_H

#include "lmms_math.h"

class RmsHelper
{
public:
	RmsHelper( int size ) :
		m_buffer( NULL )
	{
		setSize( size );
	}
	virtual ~RmsHelper() 
	{
		if( m_buffer ) delete[] m_buffer;
	}

	inline void setSize( int size )
	{
		if( m_buffer )
		{
			if( m_size < size )
			{
				delete m_buffer;
				m_buffer = new float[ size ];
				m_size = size;
				reset();
			}
			else
			{
				m_size = size;
				reset();
			}
		}
		else
		{
			m_buffer = new float[ size ];
			m_size = size;
			reset();
		}
	}

	inline void reset()
	{
		m_sizef = 1.0f / (float) m_size;
		m_pos = 0;
		m_sum = 0.0f;
		memset( m_buffer, 0, m_size * sizeof( float ) );
	}

	inline float update( const float in )
	{
		m_sum -= m_buffer[ m_pos ];
		m_sum += m_buffer[ m_pos ] = in * in;
		++m_pos %= m_size;
		return sqrtf( m_sum * m_sizef );
	}

private:
	float * m_buffer;
	float m_sum;
	unsigned int m_pos;
	unsigned int m_size;
	float m_sizef;
};


#endif
