/*
 * stereodelay.cpp - defination of StereoDelay class.
 *
 * Copyright (c) 2014 David French <dave/dot/french3/at/googlemail/dot/com>
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

#include "stereodelay.h"
#include <cstdlib>
#include "lmms_basics.h"
#include "interpolation.h"
#include "lmms_math.h"


StereoDelay::StereoDelay( int maxTime, int sampleRate )
{
	m_buffer = 0;
	m_maxTime = maxTime;
	m_maxLength = maxTime * sampleRate;
	m_length = m_maxLength;

	m_index = 0;
	m_feedback = 0.0f;
	setSampleRate( sampleRate );
}




StereoDelay::~StereoDelay()
{
	if( m_buffer )
	{
		delete m_buffer;
	}
}




void StereoDelay::tick( sampleFrame frame )
{
	m_buffer[m_index][0] = frame[0];
	m_buffer[m_index][1] = frame[1];

	int readIndex = m_index - ( int )m_length - 1;
	if( readIndex < 0 )
	{
		readIndex += m_maxLength;
	}
	float fract = 1.0f -  fraction( m_length );
	frame[0] = linearInterpolate( m_buffer[readIndex][0] ,
			m_buffer[( readIndex+1) % m_maxLength][0], fract );
	frame[1] = linearInterpolate( m_buffer[readIndex][1] ,
			m_buffer[( readIndex+1) % m_maxLength][1], fract );

	m_buffer[m_index][0] += frame[0] * m_feedback;
	m_buffer[m_index][1] += frame[1] * m_feedback;

	m_index = ( m_index + 1) % m_maxLength;
}




void StereoDelay::setSampleRate( int sampleRate )
{
	if( m_buffer )
	{
		delete m_buffer;
	}

	int bufferSize = ( int )( sampleRate * m_maxTime );
	m_buffer = new sampleFrame[bufferSize];
	for( int i = 0 ; i < bufferSize ; i++)
	{
		m_buffer[i][0] = 0.0;
		m_buffer[i][1] = 0.0;
	}
}






