/*
 * lfo.h - declaration of Lfo class, a simple sine lfo
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

#ifndef LFO_H
#define LFO_H

#ifndef M_PI
#define M_PI (3.14159265358979321 )
#endif
#define TWOPI ( 2.0 * M_PI )

class Lfo
{
public:
    Lfo( int samplerate );
    ~Lfo()
    {
    }
    void setFrequency( double frequency );
    void setAmplitude( float amplitude );
    float tick();

private:
    double m_frequency;
    double m_phase;
    double m_increment;
    double m_amplitude;
    double m_twoPiOverSr;
    int m_samplerate;
};

#endif // LFO_H
