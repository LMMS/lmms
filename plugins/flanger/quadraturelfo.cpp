/*
 * quadraturelfo.cpp - defination of QuadratureLfo class.
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

#include "quadraturelfo.h"

QuadratureLfo::QuadratureLfo( int sampleRate )
{
    setSampleRate(sampleRate);
}

void QuadratureLfo::tick( float *s, float *c )
{
    *s = sinf( m_phase );
    *c = cosf( m_phase );
    m_phase += m_increment;

// removed to return -1 +1
//    if( m_amplitude < 0.0001 )
//    {
//        *s = 1;
//        *c = 1;
//    } else
//    {
//        *s = ( ( *s * m_amplitude + 1.0 ) * 0.5 );
//        *c = ( ( *c * m_amplitude + 1.0 ) * 0.5 );
//    }


}
