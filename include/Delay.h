/*
 * Delay.h - Delay effect objects to use as building blocks in DSP
 *
 * Copyright (c) 2014 Vesa Kivimäki <contact/dot/diizy/at/nbl/dot/fi>
 * Copyright (c) 2006-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef LMMS_DELAY_H
#define LMMS_DELAY_H

#include <cmath>

#include "LmmsTypes.h"

namespace lmms
{

// brief usage 

// Classes:

// CombFeedback: a feedback comb filter - basically a simple delay line, makes a comb shape in the freq response
// CombFeedfwd: a feed-forward comb filter - an "inverted" comb filter, can be combined with CombFeedback to create a net allpass if negative gain is used
// CombFeedbackDualtap: same as CombFeedback but takes two delay values
// AllpassDelay: an allpass delay - combines feedback and feed-forward - has flat frequency response

// all classes are templated with channel count, any arbitrary channel count can be used for each fx

// Methods (for all classes):

// setDelay sets delay amount in frames. It's up to you to make this samplerate-agnostic. 
// Fractions are allowed - linear interpolation is used to deal with them
// CombFeedbackDualTap is a special case: it requires 2 delay times

// setMaxDelay (re)sets the maximum allowed delay, in frames
// NOTE: for performance reasons, there's no bounds checking at setDelay, so make sure you set maxDelay >= delay!

// clearHistory clears the delay buffer

// setGain sets the feedback/feed-forward gain, in linear amplitude, negative values are allowed
// 1.0 is full feedback/feed-forward, -1.0 is full negative feedback/feed-forward

// update runs the fx for one frame - takes as arguments input and number of channel to run, returns output

template<ch_cnt_t CHANNELS>
class CombFeedback
{
public:
	using frame = std::array<double, CHANNELS>;

	CombFeedback( int maxDelay ) :
		m_size( maxDelay ),
		m_position( 0 ),
		m_feedBack( 0.0 ),
		m_delay( 0 ),
		m_fraction( 0.0 )
	{
		m_buffer = new frame[maxDelay];
		memset( m_buffer, 0, sizeof( frame ) * maxDelay );
	}
	virtual ~CombFeedback()
	{
		delete[] m_buffer;
	}
	
	inline void setMaxDelay( int maxDelay )
	{
		if( maxDelay > m_size )
		{
			delete[] m_buffer;
			m_buffer = new frame[maxDelay];
			memset( m_buffer, 0, sizeof( frame ) * maxDelay );
		}
		m_size = maxDelay;
		m_position %= m_size;
	}
	
	inline void clearHistory()
	{
		memset( m_buffer, 0, sizeof( frame ) * m_size );
	}
	
	inline void setDelay( double delay )
	{
		m_delay = static_cast<int>( ceil( delay ) );
		m_fraction = 1.0 - ( delay - floor( delay ) );
	}
	
	inline void setGain( double gain )
	{
		m_gain = gain;
	}
	
	inline double update( double in, ch_cnt_t ch )
	{
		int readPos = m_position - m_delay;
		if( readPos < 0 ) { readPos += m_size; }
		
		const double y = std::lerp(m_buffer[readPos][ch], m_buffer[(readPos + 1) % m_size][ch], m_fraction);
		
		++m_position %= m_size;
		
		m_buffer[m_position][ch] = in + m_gain * y;
		return y;
	}

private:
	frame * m_buffer;
	int m_size;
	int m_position;
	double m_gain;
	int m_delay;
	double m_fraction;
};


template<ch_cnt_t CHANNELS>
class CombFeedfwd
{
	using frame = std::array<double, CHANNELS>;

	CombFeedfwd( int maxDelay ) :
		m_size( maxDelay ),
		m_position( 0 ),
		m_feedBack( 0.0 ),
		m_delay( 0 ),
		m_fraction( 0.0 )
	{
		m_buffer = new frame[maxDelay];
		memset( m_buffer, 0, sizeof( frame ) * maxDelay );
	}
	virtual ~CombFeedfwd()
	{
		delete[] m_buffer;
	}
	
	inline void setMaxDelay( int maxDelay )
	{
		if( maxDelay > m_size )
		{
			delete[] m_buffer;
			m_buffer = new frame[maxDelay];
			memset( m_buffer, 0, sizeof( frame ) * maxDelay );
		}
		m_size = maxDelay;
		m_position %= m_size;
	}
	
	inline void clearHistory()
	{
		memset( m_buffer, 0, sizeof( frame ) * m_size );
	}
	
	inline void setDelay( double delay )
	{
		m_delay = static_cast<int>( ceil( delay ) );
		m_fraction = 1.0 - ( delay - floor( delay ) );
	}
	
	inline void setGain( double gain )
	{
		m_gain = gain;
	}
	
	inline double update( double in, ch_cnt_t ch )
	{
		int readPos = m_position - m_delay;
		if( readPos < 0 ) { readPos += m_size; }
		
		const double y = std::lerp(m_buffer[readPos][ch], m_buffer[(readPos + 1) % m_size][ch], m_fraction) + in * m_gain;
		
		++m_position %= m_size;
		
		m_buffer[m_position][ch] = in;
		return y;
	}

private:
	frame * m_buffer;
	int m_size;
	int m_position;
	double m_gain;
	int m_delay;
	double m_fraction;
};


template<ch_cnt_t CHANNELS>
class CombFeedbackDualtap
{
	using frame = std::array<double, CHANNELS>;

	CombFeedbackDualtap( int maxDelay ) :
		m_size( maxDelay ),
		m_position( 0 ),
		m_feedBack( 0.0 ),
		m_delay( 0 ),
		m_fraction( 0.0 )
	{
		m_buffer = new frame[maxDelay];
		memset( m_buffer, 0, sizeof( frame ) * maxDelay );
	}
	virtual ~CombFeedbackDualtap()
	{
		delete[] m_buffer;
	}
	
	inline void setMaxDelay( int maxDelay )
	{
		if( maxDelay > m_size )
		{
			delete[] m_buffer;
			m_buffer = new frame[maxDelay];
			memset( m_buffer, 0, sizeof( frame ) * maxDelay );
		}
		m_size = maxDelay;
		m_position %= m_size;
	}
	
	inline void clearHistory()
	{
		memset( m_buffer, 0, sizeof( frame ) * m_size );
	}
	
	inline void setDelays( double delay1, double delay2 )
	{
		m_delay1 = static_cast<int>( ceil( delay1 ) );
		m_fraction1 = 1.0 - ( delay1 - floor( delay1 ) );
		
		m_delay2 = static_cast<int>( ceil( delay2 ) );
		m_fraction2 = 1.0 - ( delay2 - floor( delay2 ) );
	}
	
	inline void setGain( double gain )
	{
		m_gain = gain;
	}
	
	inline double update( double in, ch_cnt_t ch )
	{
		int readPos1 = m_position - m_delay1;
		if( readPos1 < 0 ) { readPos1 += m_size; }
		
		int readPos2 = m_position - m_delay2;
		if( readPos2 < 0 ) { readPos2 += m_size; }
		
		const double y = std::lerp(m_buffer[readPos1][ch], m_buffer[(readPos1 + 1) % m_size][ch], m_fraction1)
			+ std::lerp(m_buffer[readPos2][ch], m_buffer[(readPos2 + 1) % m_size][ch], m_fraction2);
		
		++m_position %= m_size;
		
		m_buffer[m_position][ch] = in + m_gain * y;
		return y;
	}

private:
	frame * m_buffer;
	int m_size;
	int m_position;
	double m_gain;
	int m_delay1;
	int m_delay2;
	double m_fraction1;
	double m_fraction2;
};


template<ch_cnt_t CHANNELS>
class AllpassDelay
{
public:
	using frame = std::array<double, CHANNELS>;

	AllpassDelay( int maxDelay ) :
		m_size( maxDelay ),
		m_position( 0 ),
		m_feedBack( 0.0 ),
		m_delay( 0 ),
		m_fraction( 0.0 )
	{
		m_buffer = new frame[maxDelay];
		memset( m_buffer, 0, sizeof( frame ) * maxDelay );
	}
	virtual ~AllpassDelay()
	{
		delete[] m_buffer;
	}
	
	inline void setMaxDelay( int maxDelay )
	{
		if( maxDelay > m_size )
		{
			delete[] m_buffer;
			m_buffer = new frame[maxDelay];
			memset( m_buffer, 0, sizeof( frame ) * maxDelay );
		}
		m_size = maxDelay;
		m_position %= m_size;
	}
	
	inline void clearHistory()
	{
		memset( m_buffer, 0, sizeof( frame ) * m_size );
	}
	
	inline void setDelay( double delay )
	{
		m_delay = static_cast<int>( ceil( delay ) );
		m_fraction = 1.0 - ( delay - floor( delay ) );
	}
	
	inline void setGain( double gain )
	{
		m_gain = gain;
	}
	
	inline double update( double in, ch_cnt_t ch )
	{
		int readPos = m_position - m_delay;
		if( readPos < 0 ) { readPos += m_size; }
		
		const double y = std::lerp(m_buffer[readPos][ch], m_buffer[(readPos + 1) % m_size][ch], m_fraction) + in * -m_gain;
		const double x = in + m_gain * y;
		
		++m_position %= m_size;
		
		m_buffer[m_position][ch] = x;
		return y;
	}

private:
	frame * m_buffer;
	int m_size;
	int m_position;
	double m_gain;
	int m_delay;
	double m_fraction;	
};

// convenience typedefs for stereo effects
using StereoCombFeedback = CombFeedback<2>;
using StereoCombFeedfwd = CombFeedfwd<2>;
using StereoCombFeedbackDualtap = CombFeedbackDualtap<2>;
using StereoAllpassDelay = AllpassDelay<2>;

} // namespace lmms

#endif // LMMS_DELAY_H
