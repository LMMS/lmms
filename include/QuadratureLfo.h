/*
 * quadraturelfo.h - defination of QuadratureLfo class.
 *
 * Copyright (c) 2014 David French <dave/dot/french3/at/googlemail/dot/com>
 *
 * This file is part of LMMS - https://lmms.io
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

#ifndef QUADRATURELFO_H
#define QUADRATURELFO_H

#include "lmms_math.h"

class QuadratureLfo
{
public:
	QuadratureLfo( int sampleRate ) :
		m_frequency(0),
		m_phase(0),
		m_offset(D_PI / 2)
	{
		setSampleRate(sampleRate);
	}

	~QuadratureLfo() = default;

	inline void setFrequency( double frequency )
	{
		if( frequency < 0 || frequency > m_samplerate / 2.0 || frequency == m_frequency )
		{
			return;
		}
		m_frequency = frequency;
		m_increment = m_frequency * m_twoPiOverSr;
	}


	inline void restart()
	{
		m_phase = 0;
	}


	inline void setSampleRate ( int samplerate )
	{
		m_samplerate = samplerate;
		m_twoPiOverSr = F_2PI / samplerate;
		m_increment = m_frequency * m_twoPiOverSr;
	}


	inline void setOffset( double offsetVal )
	{
		m_offset = offsetVal;
	}


	void tick( float *l, float *r )
	{
		*l = sinf( m_phase );
		*r = sinf( m_phase + m_offset );
		m_phase += m_increment;

		while (m_phase >= D_2PI)
		{
			m_phase -= D_2PI;
		}
	}

private:
	double m_frequency;
	double m_phase;
	double m_increment;
	double m_twoPiOverSr;
	double m_offset;
	int m_samplerate;

};

#endif // QUADRATURELFO_H
