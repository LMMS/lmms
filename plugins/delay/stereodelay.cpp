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


StereoDelay::StereoDelay(int maxLength)
{
    m_buffer = 0;
    m_buffer = ( float* )malloc(maxLength*2*sizeof( float ) );
    m_maxLength = maxLength;
    m_length = m_maxLength;
    m_index = 0;
    m_feedback = 0.0f;
    setLength( 0 );
}




StereoDelay::~StereoDelay()
{
    if( m_buffer )
    {
        free( m_buffer );
    }
}




void StereoDelay::setLength( int length )
{
    if( length <= m_maxLength && length >= 0 )
    {
        if( length < m_length )
        {
            for( int i = length * 2; i < m_length *2; i++)
            {
                m_buffer[i] = 0.0f;
            }
        }
        m_length = length;
    }
}




void StereoDelay::setFeedback( float feedback )
{
    m_feedback = feedback;
}



float m_oldLeft;
float m_oldRight;
void StereoDelay::tick( float* left, float* right )
{
    m_oldLeft = m_buffer[m_index];
    m_oldRight = m_buffer[m_index+1];
    m_buffer[m_index] = *left + ( m_oldLeft * m_feedback );
    m_buffer[m_index+1] = *right + ( m_oldRight * m_feedback );
    *left = m_oldLeft;
    *right = m_oldRight;
    m_index++; m_index++;
    if( m_index > m_length )
    {
        m_index = 0;
    }
}






