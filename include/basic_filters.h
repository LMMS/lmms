/*
 * basic_filters.h - simple but powerful filter-class with most used filters
 *
 * original file by ???
 * modified and enhanced by Tobias Doerffel
 *
 * Copyright (c) 2004-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifndef BASIC_FILTERS_H
#define BASIC_FILTERS_H

#ifndef __USE_XOPEN
#define __USE_XOPEN
#endif

#include <math.h>

#include "lmms_basics.h"
#include "Mixer.h"
#include "templates.h"
#include "lmms_constants.h"
#include "interpolation.h"

//#include <iostream>
//#include <cstdlib>

template<ch_cnt_t CHANNELS/* = DEFAULT_CHANNELS*/>
class basicFilters
{
public:
	enum FilterTypes
	{
		LowPass,
		HiPass,
		BandPass_CSG,
		BandPass_CZPG,
		Notch,
		AllPass,
		Moog,
		DoubleLowPass,
		Lowpass_RC12,
		Bandpass_RC12,
		Highpass_RC12,
		Lowpass_RC24,
		Bandpass_RC24,
		Highpass_RC24,
		Formantfilter,
		NumFilters
	} ;

	static inline float minFreq()
	{
		return( 3.0f );
	}

	static inline float minQ()
	{
		return( 0.01f );
	}

	inline void setFilterType( const int _idx )
	{
		m_doubleFilter = _idx == DoubleLowPass;
		if( !m_doubleFilter )
		{
			m_type = static_cast<FilterTypes>( _idx );
			return;
		}

		// Double lowpass mode, backwards-compat for the goofy
		// Add-NumFilters to signify doubleFilter stuff
		m_type = static_cast<FilterTypes>( LowPass );
		if( m_subFilter == NULL )
		{
			m_subFilter = new basicFilters<CHANNELS>(
						static_cast<sample_rate_t>(
							m_sampleRate ) );
		}
		m_subFilter->m_type = m_type;
	}

	inline basicFilters( const sample_rate_t _sample_rate ) :
		m_b0a0( 0.0f ),
		m_b1a0( 0.0f ),
		m_b2a0( 0.0f ),
		m_a1a0( 0.0f ),
		m_a2a0( 0.0f ),
		m_rca( 0.0f ),
		m_rcb( 1.0f ),
		m_rcc( 0.0f ),
		m_doubleFilter( false ),
		m_sampleRate( (float) _sample_rate ),
		m_subFilter( NULL )
	{
		clearHistory();
	}

	inline ~basicFilters()
	{
		delete m_subFilter;
	}

	inline void clearHistory()
	{
		// reset in/out history
		for( ch_cnt_t _chnl = 0; _chnl < CHANNELS; ++_chnl )
		{
			// reset in/out history for simple filters
			m_ou1[_chnl] = m_ou2[_chnl] = m_in1[_chnl] =
					m_in2[_chnl] = 0.0f;

			// reset in/out history for moog-filter
			m_y1[_chnl] = m_y2[_chnl] = m_y3[_chnl] = m_y4[_chnl] =
					m_oldx[_chnl] = m_oldy1[_chnl] =
					m_oldy2[_chnl] = m_oldy3[_chnl] = 0.0f;

			// reset in/out history for RC-filters
			m_rclp0[_chnl] = m_rcbp0[_chnl] = m_rchp0[_chnl] = m_rclast0[_chnl] = 0.0f;
			m_rclp1[_chnl] = m_rcbp1[_chnl] = m_rchp1[_chnl] = m_rclast1[_chnl] = 0.0f;

			for(int i=0; i<6; i++)
			   m_vflp[i][_chnl] = m_vfbp[i][_chnl] = m_vfhp[i][_chnl] = m_vflast[i][_chnl] = 0.0f;
		}
	}

	inline sample_t update( sample_t _in0, ch_cnt_t _chnl )
	{
		sample_t out;
		switch( m_type )
		{
			case Moog:
			{
				sample_t x = _in0 - m_r*m_y4[_chnl];

				// four cascaded onepole filters
				// (bilinear transform)
				m_y1[_chnl] = tLimit(
						( x + m_oldx[_chnl] ) * m_p
							- m_k * m_y1[_chnl],
								-10.0f, 10.0f );
				m_y2[_chnl] = tLimit(
					( m_y1[_chnl] + m_oldy1[_chnl] ) * m_p
							- m_k * m_y2[_chnl],
								-10.0f, 10.0f );
				m_y3[_chnl] = tLimit(
					( m_y2[_chnl] + m_oldy2[_chnl] ) * m_p
							- m_k * m_y3[_chnl],
								-10.0f, 10.0f );
				m_y4[_chnl] = tLimit(
					( m_y3[_chnl] + m_oldy3[_chnl] ) * m_p
							- m_k * m_y4[_chnl],
								-10.0f, 10.0f );

				m_oldx[_chnl] = x;
				m_oldy1[_chnl] = m_y1[_chnl];
				m_oldy2[_chnl] = m_y2[_chnl];
				m_oldy3[_chnl] = m_y3[_chnl];
				out = m_y4[_chnl] - m_y4[_chnl] * m_y4[_chnl] *
						m_y4[_chnl] * ( 1.0f / 6.0f );
				break;
			}


			// 4-times oversampled simulation of an active RC-Bandpass,-Lowpass,-Highpass-
			// Filter-Network as it was used in nearly all modern analog synthesizers. This
			// can be driven up to self-oscillation (BTW: do not remove the limits!!!).
			// (C) 1998 ... 2009 S.Fendt. Released under the GPL v2.0  or any later version.

			case Lowpass_RC12:
			{
				sample_t lp, bp, hp, in;
				for( int n = 4; n != 0; --n )
				{
					in = _in0 + m_rcbp0[_chnl] * m_rcq;
					in = qBound( -1.0f, in, 1.0f );

					lp = in * m_rcb + m_rclp0[_chnl] * m_rca;
					lp = qBound( -1.0f, lp, 1.0f );

					hp = m_rcc * ( m_rchp0[_chnl] + in - m_rclast0[_chnl] );
					hp = qBound( -1.0f, hp, 1.0f );

					bp = hp * m_rcb + m_rcbp0[_chnl] * m_rca;
					bp = qBound( -1.0f, bp, 1.0f );

					m_rclast0[_chnl] = in;
					m_rclp0[_chnl] = lp;
					m_rchp0[_chnl] = hp;
					m_rcbp0[_chnl] = bp;
				}
				return lp;
				break;
			}
			case Highpass_RC12:
			case Bandpass_RC12:
			{
				sample_t hp, bp, in;
				for( int n = 4; n != 0; --n )
				{
					in = _in0 + m_rcbp0[_chnl] * m_rcq;
					in = qBound( -1.0f, in, 1.0f );

					hp = m_rcc * ( m_rchp0[_chnl] + in - m_rclast0[_chnl] );
					hp = qBound( -1.0f, hp, 1.0f );

					bp = hp * m_rcb + m_rcbp0[_chnl] * m_rca;
					bp = qBound( -1.0f, bp, 1.0f );

					m_rclast0[_chnl] = in;
					m_rchp0[_chnl] = hp;
					m_rcbp0[_chnl] = bp;
				}
				return m_type == Highpass_RC12 ? hp : bp;
				break;
			}

			case Lowpass_RC24:
			{
				sample_t lp, bp, hp, in;
				for( int n = 4; n != 0; --n )
				{
					// first stage is as for the 12dB case...
					in = _in0 + m_rcbp0[_chnl] * m_rcq;
					in = qBound( -1.0f, in, 1.0f );

					lp = in * m_rcb + m_rclp0[_chnl] * m_rca;
					lp = qBound( -1.0f, lp, 1.0f );

					hp = m_rcc * ( m_rchp0[_chnl] + in - m_rclast0[_chnl] );
					hp = qBound( -1.0f, hp, 1.0f );

					bp = hp * m_rcb + m_rcbp0[_chnl] * m_rca;
					bp = qBound( -1.0f, bp, 1.0f );

					m_rclast0[_chnl] = in;
					m_rclp0[_chnl] = lp;
					m_rcbp0[_chnl] = bp;
					m_rchp0[_chnl] = hp;

					// second stage gets the output of the first stage as input...
					in = lp + m_rcbp1[_chnl] * m_rcq;
					in = qBound( -1.0f, in, 1.0f );

					lp = in * m_rcb + m_rclp1[_chnl] * m_rca;
					lp = qBound( -1.0f, lp, 1.0f );

					hp = m_rcc * ( m_rchp1[_chnl] + in - m_rclast1[_chnl] );
					hp = qBound( -1.0f, hp, 1.0f );

					bp = hp * m_rcb + m_rcbp1[_chnl] * m_rca;
					bp = qBound( -1.0f, bp, 1.0f );

					m_rclast1[_chnl] = in;
					m_rclp1[_chnl] = lp;
					m_rcbp1[_chnl] = bp;
					m_rchp1[_chnl] = hp;
				}
				return lp;
				break;
			}
			case Highpass_RC24:
			case Bandpass_RC24:
			{
				sample_t hp, bp, in;
				for( int n = 4; n != 0; --n )
				{
					// first stage is as for the 12dB case...
					in = _in0 + m_rcbp0[_chnl] * m_rcq;
					in = qBound( -1.0f, in, 1.0f );

					hp = m_rcc * ( m_rchp0[_chnl] + in - m_rclast0[_chnl] );
					hp = qBound( -1.0f, hp, 1.0f );

					bp = hp * m_rcb + m_rcbp0[_chnl] * m_rca;
					bp = qBound( -1.0f, bp, 1.0f );

					m_rclast0[_chnl] = in;
					m_rchp0[_chnl] = hp;
					m_rcbp0[_chnl] = bp;

					// second stage gets the output of the first stage as input...
					in = m_type == Highpass_RC24
						? hp + m_rcbp1[_chnl] * m_rcq
						: bp + m_rcbp1[_chnl] * m_rcq;

					in = qBound( -1.0f, in, 1.0f );

					hp = m_rcc * ( m_rchp1[_chnl] + in - m_rclast1[_chnl] );
					hp = qBound( -1.0f, hp, 1.0f );

					bp = hp * m_rcb + m_rcbp1[_chnl] * m_rca;
					bp = qBound( -1.0f, bp, 1.0f );

					m_rclast1[_chnl] = in;
					m_rchp1[_chnl] = hp;
					m_rcbp1[_chnl] = bp;
				}
				return m_type == Highpass_RC24 ? hp : bp;
				break;
			}

			case Formantfilter:
			{
				sample_t lp, hp, bp, in;

				out = 0;
				for(int o=0; o<4; o++)
				{
					// first formant
					in = _in0 + m_vfbp[0][_chnl] * m_vfq;
					in = qBound( -1.0f, in, 1.0f );

					lp = in * m_vfb[0] + m_vflp[0][_chnl] * m_vfa[0];
					lp = qBound( -1.0f, lp, 1.0f );

					hp = m_vfc[0] * ( m_vfhp[0][_chnl] + in - m_vflast[0][_chnl] );
					hp = qBound( -1.0f, hp, 1.0f );

					bp = hp * m_vfb[0] + m_vfbp[0][_chnl] * m_vfa[0];
					bp = qBound( -1.0f, bp, 1.0f );

					m_vflast[0][_chnl] = in;
					m_vflp[0][_chnl] = lp;
					m_vfhp[0][_chnl] = hp;
					m_vfbp[0][_chnl] = bp;

					in = bp + m_vfbp[2][_chnl] * m_vfq;
					in = qBound( -1.0f, in, 1.0f );

					lp = in * m_vfb[0] + m_vflp[2][_chnl] * m_vfa[0];
					lp = qBound( -1.0f, lp, 1.0f );

					hp = m_vfc[0] * ( m_vfhp[2][_chnl] + in - m_vflast[2][_chnl] );
					hp = qBound( -1.0f, hp, 1.0f );

					bp = hp * m_vfb[0] + m_vfbp[2][_chnl] * m_vfa[0];
					bp = qBound( -1.0f, bp, 1.0f );

					m_vflast[2][_chnl] = in;
					m_vflp[2][_chnl] = lp;
					m_vfhp[2][_chnl] = hp;
					m_vfbp[2][_chnl] = bp;

					in = bp + m_vfbp[4][_chnl] * m_vfq;
					in = qBound( -1.0f, in, 1.0f );

					lp = in * m_vfb[0] + m_vflp[4][_chnl] * m_vfa[0];
					lp = qBound( -1.0f, lp, 1.0f );

					hp = m_vfc[0] * ( m_vfhp[4][_chnl] + in - m_vflast[4][_chnl] );
					hp = qBound( -1.0f, hp, 1.0f );

					bp = hp * m_vfb[0] + m_vfbp[4][_chnl] * m_vfa[0];
					bp = qBound( -1.0f, bp, 1.0f );

					m_vflast[4][_chnl] = in;
					m_vflp[4][_chnl] = lp;
					m_vfhp[4][_chnl] = hp;
					m_vfbp[4][_chnl] = bp;

					out += bp;

					// second formant
					in = _in0 + m_vfbp[0][_chnl] * m_vfq;
					in = qBound( -1.0f, in, 1.0f );

					lp = in * m_vfb[1] + m_vflp[1][_chnl] * m_vfa[1];
					lp = qBound( -1.0f, lp, 1.0f );

					hp = m_vfc[1] * ( m_vfhp[1][_chnl] + in - m_vflast[1][_chnl] );
					hp = qBound( -1.0f, hp, 1.0f );

					bp = hp * m_vfb[1] + m_vfbp[1][_chnl] * m_vfa[1];
					bp = qBound( -1.0f, bp, 1.0f );

					m_vflast[1][_chnl] = in;
					m_vflp[1][_chnl] = lp;
					m_vfhp[1][_chnl] = hp;
					m_vfbp[1][_chnl] = bp;

					in = bp + m_vfbp[3][_chnl] * m_vfq;
					in = qBound( -1.0f, in, 1.0f );

					lp = in * m_vfb[1] + m_vflp[3][_chnl] * m_vfa[1];
					lp = qBound( -1.0f, lp, 1.0f );

					hp = m_vfc[1] * ( m_vfhp[3][_chnl] + in - m_vflast[3][_chnl] );
					hp = qBound( -1.0f, hp, 1.0f );

					bp = hp * m_vfb[1] + m_vfbp[3][_chnl] * m_vfa[1];
					bp = qBound( -1.0f, bp, 1.0f );

					m_vflast[3][_chnl] = in;
					m_vflp[3][_chnl] = lp;
					m_vfhp[3][_chnl] = hp;
					m_vfbp[3][_chnl] = bp;

					in = bp + m_vfbp[5][_chnl] * m_vfq;
					in = qBound( -1.0f, in, 1.0f );

					lp = in * m_vfb[1] + m_vflp[5][_chnl] * m_vfa[1];
					lp = qBound( -1.0f, lp, 1.0f );

					hp = m_vfc[1] * ( m_vfhp[5][_chnl] + in - m_vflast[5][_chnl] );
					hp = qBound( -1.0f, hp, 1.0f );

					bp = hp * m_vfb[1] + m_vfbp[5][_chnl] * m_vfa[1];
					bp = qBound( -1.0f, bp, 1.0f );

					m_vflast[5][_chnl] = in;
					m_vflp[5][_chnl] = lp;
					m_vfhp[5][_chnl] = hp;
					m_vfbp[5][_chnl] = bp;

					out += bp;
				}

				return( out/2.0f );
				break;
			}

			default:
				// filter
				out = m_b0a0*_in0 +
						m_b1a0*m_in1[_chnl] +
						m_b2a0*m_in2[_chnl] -
						m_a1a0*m_ou1[_chnl] -
						m_a2a0*m_ou2[_chnl];

				// push in/out buffers
				m_in2[_chnl] = m_in1[_chnl];
				m_in1[_chnl] = _in0;
				m_ou2[_chnl] = m_ou1[_chnl];

				m_ou1[_chnl] = out;
				break;
		}

		if( m_doubleFilter )
		{
			return m_subFilter->update( out, _chnl );
		}

		// Clipper band limited sigmoid
		return out;
	}


	inline void calcFilterCoeffs( float _freq, float _q
				/*, const bool _q_is_bandwidth = false*/ )
	{
		// temp coef vars
		_q = qMax( _q, minQ() );

		if( m_type == Lowpass_RC12  ||
			m_type == Bandpass_RC12 ||
			m_type == Highpass_RC12 ||
			m_type == Lowpass_RC24 ||
			m_type == Bandpass_RC24 ||
			m_type == Highpass_RC24 )
		{
			_freq = qBound( 50.0f, _freq, 20000.0f );

			m_rca = 1.0f - (1.0f/(m_sampleRate*4)) / ( (1.0f/(_freq*2.0f*M_PI)) + (1.0f/(m_sampleRate*4)) );
			m_rcb = 1.0f - m_rca;
			m_rcc = (1.0f/(_freq*2.0f*M_PI)) / ( (1.0f/(_freq*2.0f*M_PI)) + (1.0f/(m_sampleRate*4)) );

			// Stretch Q/resonance, as self-oscillation reliably starts at a q of ~2.5 - ~2.6
			m_rcq = _q * 0.25f;
			return;
		}

		if( m_type == Formantfilter )
		{
			_freq = qBound( minFreq(), _freq, 20000.0f ); // limit freq and q for not getting bad noise out of the filter...

			// formats for a, e, i, o, u, a
			static const float _f[5][2] = { { 1000, 1400 }, { 500, 2300 },
							{ 320, 3200 },
							{ 500, 1000 },
							{ 320, 800 } };
			static const float freqRatio = 4.0f / 14000.0f;

			// Stretch Q/resonance
			m_vfq = _q * 0.25f;

			// frequency in lmms ranges from 1Hz to 14000Hz
			const float vowelf = _freq * freqRatio;
			const int vowel = static_cast<int>( vowelf );
			const float fract = vowelf - vowel;

			// interpolate between formant frequencies
			const float f0 = linearInterpolate( _f[vowel+0][0], _f[vowel+1][0], fract );
			const float f1 = linearInterpolate( _f[vowel+0][1], _f[vowel+1][1], fract );

			m_vfa[0] = 1.0f - (1.0f/(m_sampleRate*4)) /
						( (1.0f/(f0*2.0f*M_PI)) +
						(1.0f/(m_sampleRate*4)) );
			m_vfb[0] = 1.0f - m_vfa[0];
			m_vfc[0] = (1.0f/(f0*2.0f*M_PI)) /
						( (1.0f/(f0*2.0f*M_PI)) +
						(1.0f/(m_sampleRate*4)) );

			m_vfa[1] = 1.0f - (1.0f/(m_sampleRate*4)) /
						( (1.0f/(f1*2.0f*M_PI)) +
						(1.0f/(m_sampleRate*4)) );
			m_vfb[1] = 1.0f - m_vfa[1];
			m_vfc[1] = (1.0f/(f1*2.0f*M_PI)) /
					( (1.0f/(f1*2.0f*M_PI)) +
						(1.0f/(m_sampleRate*4)) );
			return;
		}

		if( m_type == Moog )
		{
			_freq = qBound( minFreq(), _freq, 20000.0f ); 

			// [ 0 - 0.5 ]
			const float f = _freq / m_sampleRate;
			// (Empirical tunning)
			m_p = ( 3.6f - 3.2f * f ) * f;
			m_k = 2.0f * m_p - 1;
			m_r = _q * powf( M_E, ( 1 - m_p ) * 1.386249f );

			if( m_doubleFilter )
			{
				m_subFilter->m_r = m_r;
				m_subFilter->m_p = m_p;
				m_subFilter->m_k = m_k;
			}
			return;
		}

		// other filters
		_freq = qBound( minFreq(), _freq, 20000.0f ); 
		const float omega = F_2PI * _freq / m_sampleRate;
		const float tsin = sinf( omega );
		const float tcos = cosf( omega );
		//float alpha;

		//if (q_is_bandwidth)
		//alpha = tsin*sinhf(logf(2.0f)/2.0f*q*omega/
		//					tsin);
		//else
		const float alpha = 0.5f * tsin / _q;

		const float a0 = 1.0f / ( 1.0f + alpha );

		m_a1a0 = -2.0f * tcos * a0;
		m_a2a0 = ( 1.0f - alpha ) * a0;

		switch( m_type )
		{
			case LowPass:
				m_b1a0 = ( 1.0f - tcos ) * a0;
				m_b0a0 = m_b1a0 * 0.5f;
				m_b2a0 = m_b0a0;//((1.0f-tcos)/2.0f)*a0;
				break;
			case HiPass:
				m_b1a0 = ( -1.0f - tcos ) * a0;
				m_b0a0 = m_b1a0 * -0.5f;
				m_b2a0 = m_b0a0;//((1.0f+tcos)/2.0f)*a0;
				break;
			case BandPass_CSG:
				m_b1a0 = 0.0f;
				m_b0a0 = tsin * 0.5f * a0;
				m_b2a0 = -m_b0a0;
				break;
			case BandPass_CZPG:
				m_b1a0 = 0.0f;
				m_b0a0 = alpha * a0;
				m_b2a0 = -m_b0a0;
				break;
			case Notch:
				m_b1a0 = m_a1a0;
				m_b0a0 = a0;
				m_b2a0 = a0;
				break;
			case AllPass:
				m_b1a0 = m_a1a0;
				m_b0a0 = m_a2a0;
				m_b2a0 = 1.0f;//(1.0f+alpha)*a0;
				break;
			default:
				break;
		}

		if( m_doubleFilter )
		{
			m_subFilter->m_b0a0 = m_b0a0;
			m_subFilter->m_b1a0 = m_b1a0;
			m_subFilter->m_b2a0 = m_b2a0;
			m_subFilter->m_a1a0 = m_a1a0;
			m_subFilter->m_a2a0 = m_a2a0;
		}
	}


private:
	// filter coeffs
	float m_b0a0, m_b1a0, m_b2a0, m_a1a0, m_a2a0;

	// coeffs for moog-filter
	float m_r, m_p, m_k;

	// coeffs for RC-type-filters
	float m_rca, m_rcb, m_rcc, m_rcq;

	// coeffs for formant-filters
	float m_vfa[4], m_vfb[4], m_vfc[4], m_vfq;

	typedef sample_t frame[CHANNELS];

	// in/out history
	frame m_ou1, m_ou2, m_in1, m_in2;

	// in/out history for moog-filter
	frame m_y1, m_y2, m_y3, m_y4, m_oldx, m_oldy1, m_oldy2, m_oldy3;

	// in/out history for RC-type-filters
	frame m_rcbp0, m_rclp0, m_rchp0, m_rclast0;
	frame m_rcbp1, m_rclp1, m_rchp1, m_rclast1;

	// in/out history for Formant-filters
	frame m_vfbp[6], m_vflp[6], m_vfhp[6], m_vflast[6];

	FilterTypes m_type;
	bool m_doubleFilter;

	float m_sampleRate;
	basicFilters<CHANNELS> * m_subFilter;

} ;


#endif
