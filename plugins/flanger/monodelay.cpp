/*
 * monodelay.cpp - defination of MonoDelay class.
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

#include "monodelay.h"
#include "interpolation.h"
#include "lmms_math.h"

MonoDelay::MonoDelay( int maxTime , int sampleRate )
{
    m_buffer = 0;
    m_maxTime = maxTime;
    m_maxLength = maxTime * sampleRate;
    m_length = m_maxLength;

    m_index = 0;
    m_feedback = 0.0f;
    setSampleRate( sampleRate );
}




MonoDelay::~MonoDelay()
{
    if( m_buffer )
    {
        delete m_buffer;
    }
}




void MonoDelay::tick( sample_t* sample )
{
    m_buffer[m_index] = *sample;
    int readIndex = m_index - ( int )m_length;
    if(readIndex < 0)
    {
        readIndex += m_maxLength;
    }
    float fract = fraction( m_length );
    if(readIndex != m_maxLength-1 )
    {
    *sample = linearInterpolate(m_buffer[readIndex] ,
                               m_buffer[readIndex+1], fract );
    } else
    {
        *sample = linearInterpolate(m_buffer[readIndex] ,
                                   m_buffer[0], fract );
    }
    m_buffer[m_index] += *sample * m_feedback;
    m_index = ( m_index +1 ) % m_maxLength;
}




void MonoDelay::setSampleRate( int sampleRate )
{
    if( m_buffer )
    {
        delete m_buffer;
    }


    m_buffer = new sample_t[( int )( sampleRate * m_maxTime )];
}
