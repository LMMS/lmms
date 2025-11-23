/*
 * quadraturelfo.h - definition of QuadratureLfo class.
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

#ifndef LMMS_QUADRATURE_LFO_H
#define LMMS_QUADRATURE_LFO_H

#include <numbers>
#include <cmath>

namespace lmms
{


class QuadratureLfo
{
public:
	QuadratureLfo( int sampleRate ) :
		m_frequency(0),
		m_phase(0),
		m_offset(std::numbers::pi * 0.5)
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
		m_twoPiOverSr = 2 * std::numbers::pi_v<float> / samplerate;
		m_increment = m_frequency * m_twoPiOverSr;
	}


	inline void setOffset( double offsetVal )
	{
		m_offset = offsetVal;
	}


	void tick( float *l, float *r )
	{
		*l = std::sin(m_phase);
		*r = std::sin(m_phase + m_offset);
		m_phase += m_increment;
		m_phase = std::fmod(m_phase, 2 * std::numbers::pi);
	}

private:
	double m_frequency;
	double m_phase;
	double m_increment;
	double m_twoPiOverSr;
	double m_offset;
	int m_samplerate;

};


} // namespace lmms

#endif // LMMS_QUADRATURE_LFO_H
