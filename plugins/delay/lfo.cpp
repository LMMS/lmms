/*
 * lfo.cpp - defination of Lfo class.
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

#include "lfo.h"
#include <math.h>




Lfo::Lfo( int samplerate )
{
    m_samplerate = samplerate;
    m_twoPiOverSr = TWOPI / samplerate;
}



void Lfo::setFrequency( double frequency )
{
    if( frequency < 0 || frequency > ( m_samplerate / 2.0 ) || frequency == m_frequency )
    {
        return;
    }
    m_frequency = frequency;
    m_increment = m_frequency * m_twoPiOverSr;
}




void Lfo::setAmplitude( float amplitude )
{
    if( amplitude < 0.0 || amplitude > 1.0 )
    {
        return;
    }
    m_amplitude = amplitude;
}




float Lfo::tick()
{
    float output = ( float )sin( m_phase );
    m_phase += m_increment;
    if( m_phase >= TWOPI )
    {
        m_phase -= TWOPI;
    }
    if( m_amplitude > 0.0001 )
    {
        return ( ( output + 1.0 ) / 2.0 ) * m_amplitude;
    } else
    {
        return 1;
    }
}
