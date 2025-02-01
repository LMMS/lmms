/*
 * lfo.h - declaration of Lfo class, a simple sine lfo
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

#ifndef LFO_H
#define LFO_H

#include "lmms_constants.h"


namespace lmms
{


class Lfo
{
public:
	Lfo( int samplerate );
	~Lfo() = default;




	inline void setFrequency( double frequency )
	{
		if( frequency < 0 || frequency > ( m_samplerate / 2.0 ) || frequency == m_frequency )
		{
			return;
		}
		m_frequency = frequency;
		m_increment = m_frequency * m_twoPiOverSr;

		if (m_phase >= numbers::tau_v<float>)	{	m_phase -= numbers::tau_v<float>;	}
	}




	inline void setSampleRate ( int samplerate )
	{
		m_samplerate = samplerate;
		m_twoPiOverSr = numbers::tau_v<float> / samplerate;
		m_increment = m_frequency * m_twoPiOverSr;
	}




	float tick();

private:
	double m_frequency;
	double m_phase;
	double m_increment;
	double m_twoPiOverSr;
	int m_samplerate;
};


} // namespace lmms

#endif // LFO_H
