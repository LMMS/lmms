/*
 * eqfilter.cpp - defination of EqFilterclass.
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

#ifndef EQFILTER_H
#define EQFILTER_H

#include "BasicFilters.h"
#include "lmms_math.h"

namespace lmms
{


///
/// \brief The EqFilter class.
/// A wrapper for the StereoBiQuad class, giving it freq, res, and gain controls.
/// Used on a per channel per frame basis with recalculation of coefficents
/// upon parameter changes. The intention is to use this as a bass class, children override
/// the calcCoefficents() function, providing the coefficents a1, a2, b0, b1, b2.
///
class EqFilter
{
public:
	EqFilter() :
		m_sampleRate(0),
		m_freq(0),
		m_res(0),
		m_gain(0),
		m_bw(0)
	{

	}




	virtual inline void setSampleRate( int sampleRate )
	{
		if( sampleRate != m_sampleRate )
		{
			m_sampleRate = sampleRate;
			calcCoefficents();
		}
	}




	virtual inline void setFrequency( float freq ){
		if ( freq != m_freq )
		{
			m_freq = freq;
			calcCoefficents();
		}
	}




	virtual inline void setQ( float res )
	{
		if ( res != m_res )
		{
			m_res = res;
			calcCoefficents();
		}
	}




	virtual inline void setGain( float gain )
	{
		if ( gain != m_gain )
		{
			m_gain = gain;
			calcCoefficents();
		}
	}



	virtual inline void setParameters( float sampleRate, float freq, float res, float gain )
	{
		bool hasChanged = ( sampleRate != m_sampleRate ||
		                    freq != m_freq ||
		                    res != m_res ||
		                    gain != m_gain );
		if ( hasChanged )
		{
			m_sampleRate = sampleRate;
			m_freq = freq;
			m_res = res;
			m_gain = gain;
			calcCoefficents();
		}
	}


	///
	/// \brief update
	/// filters using two BiQuads, then crossfades,
	///  depending on on percentage of period processes
	/// \param in
	/// \param ch
	/// \param frameProgress percentage of frame processed
	/// \return
	///
	inline float update( float in, ch_cnt_t ch, float frameProgress)
	{
		float initailF =  m_biQuadFrameInitial.update( in, ch );
		float targetF = m_biQuadFrameTarget.update( in, ch );

		if(frameProgress > 0.99999 )
		{
			m_biQuadFrameInitial= m_biQuadFrameTarget;
		}

		return (1.0f-frameProgress) * initailF + frameProgress * targetF;

	}


protected:
	///
	/// \brief calcCoefficents
	///  Override this in child classes to provide the coefficents, based on
	///  Freq, Res and Gain
	virtual void calcCoefficents(){
		setCoeffs( 0, 0, 0, 0, 0 );

	}

	inline void setCoeffs( float a1, float a2, float b0, float b1, float b2 )
	{
		m_biQuadFrameTarget.setCoeffs( a1, a2, b0, b1, b2 );
	}






	float m_sampleRate;
	float m_freq;
	float m_res;
	float m_gain;
	float m_bw;
	StereoBiQuad m_biQuadFrameInitial;
	StereoBiQuad m_biQuadFrameTarget;
};




///
/// \brief The EqHp12Filter class
/// A 2 pole High Pass Filter
/// Coefficent calculations from http://www.musicdsp.org/files/Audio-EQ-Cookbook.txt
class EqHp12Filter : public EqFilter
{
public :
	void calcCoefficents() override
	{

		// calc intermediate
		float w0 = numbers::tau_v<float> * m_freq / m_sampleRate;
		float c = std::cos(w0);
		float s = std::sin(w0);
		float alpha = s / ( 2 * m_res );

		//calc coefficents
		float b0 = (1 + c) * 0.5;
		float b1 = (-(1 + c));
		float b2 = (1 + c) * 0.5;
		float a0 = 1 + alpha;
		float a1 = (-2 * c);
		float a2 = 1 - alpha;

		//normalise
		b0 /= a0;
		b1 /= a0;
		b2 /= a0;
		a1 /= a0;
		a2 /= a0;

		a0 = 1;

		setCoeffs( a1, a2, b0, b1, b2 );


	}
};




///
/// \brief The EqLp12Filter class.
/// A 2 pole low pass filter
/// Coefficent calculations from http://www.musicdsp.org/files/Audio-EQ-Cookbook.txt
///
class EqLp12Filter : public EqFilter
{
public :
	void calcCoefficents() override
	{

		// calc intermediate
		float w0 = numbers::tau_v<float> * m_freq / m_sampleRate;
		float c = std::cos(w0);
		float s = std::sin(w0);
		float alpha = s / ( 2 * m_res );

		//calc coefficents
		float b0 = (1 - c) * 0.5;
		float b1 = 1 - c;
		float b2 = (1 - c) * 0.5;
		float a0 = 1 + alpha;
		float a1 = -2 * c;
		float a2 = 1 - alpha;

		//normalise
		b0 /= a0;
		b1 /= a0;
		b2 /= a0;
		a1 /= a0;
		a2 /= a0;

		a0 = 1;

		setCoeffs( a1, a2, b0, b1, b2 );
	}
};



///
/// \brief The EqPeakFilter class
/// A Peak Filter
/// Coefficent calculations from http://www.musicdsp.org/files/Audio-EQ-Cookbook.txt
///
class EqPeakFilter : public EqFilter
{
public:


	void calcCoefficents() override
	{
		// calc intermediate
		float w0 = numbers::tau_v<float> * m_freq / m_sampleRate;
		float c = std::cos(w0);
		float s = std::sin(w0);
		float A = fastPow10f(m_gain * 0.025);
		float alpha = s * std::sinh(std::log(2.f) / 2 * m_bw * w0 / std::sin(w0));

		//calc coefficents
		float b0 = 1 + alpha * A;
		float b1 = -2 * c;
		float b2 = 1 - alpha * A;
		float a0 = 1 + alpha / A;
		float a1 = -2 * c;
		float a2 = 1 - alpha / A;

		//normalise
		b0 /= a0;
		b1 /= a0;
		b2 /= a0;
		a1 /= a0;
		a2 /= a0;
		a0 = 1;

		setCoeffs( a1, a2, b0, b1, b2 );
	}

	inline void setParameters( float sampleRate, float freq, float bw, float gain ) override
	{
		bool hasChanged = false;
		if( sampleRate != m_sampleRate )
		{
			m_sampleRate = sampleRate;
			hasChanged = true;
		}
		if ( freq != m_freq )
		{
			m_freq = freq;
			hasChanged = true;
		}
		if ( bw != m_bw )
		{
			m_bw = bw;
			hasChanged = true;
		}
		if ( gain != m_gain )
		{
			m_gain = gain;
			hasChanged = true;
		}

		if ( hasChanged ) { calcCoefficents(); }
	}
};




class EqLowShelfFilter : public EqFilter
{
public :
	void calcCoefficents() override
	{

		// calc intermediate
		float w0 = numbers::tau_v<float> * m_freq / m_sampleRate;
		float c = std::cos(w0);
		float s = std::sin(w0);
		float A = fastPow10f(m_gain * 0.025);
		// float alpha = s / (2 * m_res);
		float beta = std::sqrt(A) / m_res;

		//calc coefficents
		float b0 = A * ((A + 1) - (A - 1) * c + beta * s);
		float b1 = 2 * A * ((A - 1) - (A + 1) * c);
		float b2 = A * ((A + 1) - (A - 1) * c - beta * s);
		float a0 = (A + 1) + (A - 1) * c + beta * s;
		float a1 = -2 * ((A - 1) + (A + 1) * c);
		float a2 = (A + 1) + (A - 1) * c - beta * s;

		//normalise
		b0 /= a0;
		b1 /= a0;
		b2 /= a0;
		a1 /= a0;
		a2 /= a0;

		a0 = 1;

		setCoeffs( a1, a2, b0, b1, b2 );


	}
};

class EqHighShelfFilter : public EqFilter
{
public :
	void calcCoefficents() override
	{

		// calc intermediate
		float w0 = numbers::tau_v<float> * m_freq / m_sampleRate;
		float c = std::cos(w0);
		float s = std::sin(w0);
		float A = fastPow10f(m_gain * 0.025);
		float beta = std::sqrt(A) / m_res;

		//calc coefficents
		float b0 = A * ((A + 1) + (A - 1) * c + beta * s);
		float b1 = -2 * A * ((A - 1) + (A + 1) * c);
		float b2 = A * ((A + 1) + (A - 1) * c - beta * s);
		float a0 = (A + 1) - (A - 1) * c + beta * s;
		float a1 = 2 * ((A - 1) - (A + 1) * c);
		float a2 = (A + 1) - (A - 1) * c - beta * s;

		//normalise
		b0 /= a0;
		b1 /= a0;
		b2 /= a0;
		a1 /= a0;
		a2 /= a0;
		a0 = 1;

		setCoeffs( a1, a2, b0, b1, b2 );
	}
};




class EqLinkwitzRiley : public StereoLinkwitzRiley
{
public:
	EqLinkwitzRiley() :
		StereoLinkwitzRiley( 44100),
		m_freq(0 ),
		m_sr( 1 )
	{
	}

	virtual inline void setSR( int sampleRate )
	{
		if( sampleRate != m_sr )
		{
			m_sr = sampleRate;
			setSampleRate( sampleRate );
			setLowpass(m_freq);
		}
	}




	virtual inline void setFrequency( float freq ){
		if ( freq != m_freq )
		{
			m_freq = freq;
			setLowpass(m_freq);
		}
	}




	virtual void processBuffer( SampleFrame* buf, const fpp_t frames )
	{
		for ( fpp_t f = 0 ; f < frames ; ++f)
		{
			buf[f][0] = update( buf[f][0] , 0);
			buf[f][1] = update( buf[f][1] , 1);
		}
	}
protected:

	float m_freq;
	int m_sr;


};


} // namespace lmms


#endif // EQFILTER_H
