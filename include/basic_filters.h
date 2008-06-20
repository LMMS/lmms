/*
 * basic_filters.h - simple but powerful filter-class with most used filters
 *
 * original file by ??? 
 * modified and enhanced by Tobias Doerffel
 *
 * Copyright (c) 2004-2007 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * 
 * This file is part of Linux MultiMedia Studio - http://lmms.sourceforge.net
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


#ifndef _BASIC_FILTERS_H
#define _BASIC_FILTERS_H

#ifndef __USE_XOPEN
#define __USE_XOPEN
#endif

#include <math.h>

#include "types.h"
#include "mixer.h"
#include "templates.h"
#include "lmms_constants.h"

//const int MOOG_VOLTAGE = 40000;

template<ch_cnt_t CHANNELS/* = DEFAULT_CHANNELS*/>
class basicFilters
{
public:
	enum filterTypes
	{
		LOWPASS,
		HIPASS,
		BANDPASS_CSG,
		BANDPASS_CZPG,
		NOTCH,
		ALLPASS,
		MOOG,
		SIMPLE_FLT_CNT
	} ;

	static inline float minQ( void )
	{
		return( 0.01f );
	}

	inline void setFilterType( const int _idx )
	{
		m_double_filter = _idx >= SIMPLE_FLT_CNT;
		if( !m_double_filter )
		{
			m_type = static_cast<filterTypes>( _idx );
			return;
		}
		m_type = static_cast<filterTypes>( LOWPASS + _idx -
							SIMPLE_FLT_CNT );
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
		m_double_filter( FALSE ),
		m_sampleRate( _sample_rate ),
		m_subFilter( NULL )
	{
		clearHistory();
	}

	inline ~basicFilters()
	{
		delete m_subFilter;
	}

	inline void clearHistory( void )
	{
		// reset in/out history
		for( ch_cnt_t _chnl = 0; _chnl < CHANNELS; ++_chnl )
		{
			// reset in/out history for simple filters
			m_ou1[_chnl] = m_ou2[_chnl] = m_in1[_chnl] =
					m_in2[_chnl] = 0.0f;
			// reset in/out historey for moog-filter
			m_y1[_chnl] = m_y2[_chnl] = m_y3[_chnl] = m_y4[_chnl] =
					m_oldx[_chnl] = m_oldy1[_chnl] =
					m_oldy2[_chnl] = m_oldy3[_chnl] = 0.0f;
		}
	}

	inline sample_t update( sample_t _in0, ch_cnt_t _chnl )
	{
		sample_t out;
		switch( m_type )
		{
			case MOOG:
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
/*			case MOOG2:
			case DOUBLE_MOOG2:
			{
				const float x1 = ( _in0 - m_r *
						m_oldx[_chnl] ) / MOOG_VOLTAGE;
				const float tanh1 = tanhf( x1 );
				const float x2 = m_oldy1[_chnl] / MOOG_VOLTAGE;
				const float tanh2 = tanhf( x2 );
				m_y1[_chnl] = m_oldy1[_chnl] + m_p *
							( tanh1 - tanh2 );
				m_oldy1[_chnl] = m_y1[_chnl];
				m_y2[_chnl] = m_oldy2[_chnl] + m_p *
						( tanhf( m_y1[_chnl] /
							MOOG_VOLTAGE ) -
						tanhf( m_oldy2[_chnl] /
							MOOG_VOLTAGE ) );
				m_oldy2[_chnl] = m_y2[_chnl];
				m_y3[_chnl] = m_oldy3[_chnl] + m_p *
						( tanhf( m_y2[_chnl] /
							MOOG_VOLTAGE ) -
						tanhf( m_oldy3[_chnl] /
							MOOG_VOLTAGE ) );
				m_oldy3[_chnl] = m_y3[_chnl];
				m_y4[_chnl] = m_ou1[_chnl] + m_p *
						( tanhf( m_y3[_chnl] /
							MOOG_VOLTAGE ) -
						tanhf( m_ou1[_chnl] /
							MOOG_VOLTAGE ) );
				m_ou1[_chnl] = m_y4[_chnl];

				m_oldx[_chnl] = ( m_y4[_chnl] +
							m_ou2[_chnl] ) * 0.5f;
				m_ou2[_chnl] = m_y4[_chnl];

				// the same code again...
				m_y1[_chnl] = m_oldy1[_chnl] + m_p *
							( tanh1 - tanh2 );
				m_oldy1[_chnl] = m_y1[_chnl];
				m_y2[_chnl] = m_oldy2[_chnl] + m_p *
						( tanhf( m_y1[_chnl] /
							MOOG_VOLTAGE ) -
						tanhf( m_oldy2[_chnl] /
							MOOG_VOLTAGE ) );
				m_oldy2[_chnl] = m_y2[_chnl];
				m_y3[_chnl] = m_oldy3[_chnl] + m_p *
						( tanhf( m_y2[_chnl] /
							MOOG_VOLTAGE ) -
						tanhf( m_oldy3[_chnl] /
							MOOG_VOLTAGE ) );
				m_oldy3[_chnl] = m_y3[_chnl];
				m_y4[_chnl] = m_ou1[_chnl] + m_p *
						( tanhf( m_y3[_chnl] /
							MOOG_VOLTAGE ) -
						tanhf( m_ou1[_chnl] /
							MOOG_VOLTAGE ) );
				m_ou1[_chnl] = m_y4[_chnl];

				m_oldx[_chnl] = ( m_y4[_chnl] +
							m_ou2[_chnl] ) * 0.5f;
				m_ou2[_chnl] = m_y4[_chnl];

				out = m_oldx[_chnl];
				break;
			}*/

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

		if( m_double_filter )
		{
			return( m_subFilter->update( out, _chnl ) );
		}

		// Clipper band limited sigmoid
		return( out );
	}


	inline void calcFilterCoeffs( float _freq, float _q
				/*, const bool _q_is_bandwidth = FALSE*/ )
	{
		// temp coef vars
		_freq = tMax( _freq, 0.01f );// limit freq and q for not getting
					      // bad noise out of the filter...
		_q = tMax( _q, minQ() );

		if( m_type == MOOG )
		{
			// [ 0 - 0.5 ]
			const float f = _freq / m_sampleRate;
			// (Empirical tunning)
			m_p = ( 3.6f - 3.2f * f ) * f;
			m_k = 2.0f * m_p - 1;
			m_r = _q * powf( M_E, ( 1 - m_p ) * 1.386249f );

			if( m_double_filter )
			{
				m_subFilter->m_r = m_r;
				m_subFilter->m_p = m_p;
				m_subFilter->m_k = m_k;
			}
			return;

/*			case DOUBLE_MOOG2:
			{
				if( m_subFilter == NULL )
				{
					m_subFilter =
						new basicFilters<CHANNELS>(
							m_sampleRate );
				}
				m_subFilter->calcFilterCoeffs( MOOG2, _freq,
									_q );
			}

			case MOOG2:
			{
				const float kfc = 2 * _freq / m_sampleRate;
				const float kf = _freq / m_sampleRate;
				const float kfcr = 1.8730 * ( kfc*kfc*kfc ) +
							0.4955 * ( kfc*kfc ) +
							0.6490 * kfc + 0.9988;
				const float kacr = -3.9364 * ( kfc*kfc ) +
							1.8409 * kfc + 0.9968;
				m_p = MOOG_VOLTAGE * ( 1 - expf( F_2PI *
								kfcr * kf ) );
				m_r = 4 * _q * kacr;
				break;
			}*/
		}

		// other filters
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
			case LOWPASS:
				m_b1a0 = ( 1.0f - tcos ) * a0;
				m_b0a0 = m_b1a0 * 0.5f;
				m_b2a0 = m_b0a0;//((1.0f-tcos)/2.0f)*a0;
				break;
			case HIPASS:
				m_b1a0 = ( -1.0f - tcos ) * a0;
				m_b0a0 = m_b1a0 * -0.5f;
				m_b2a0 = m_b0a0;//((1.0f+tcos)/2.0f)*a0;
				break;
			case BANDPASS_CSG:
				m_b1a0 = 0.0f;
				m_b0a0 = tsin * 0.5f * a0;
				m_b2a0 = -m_b0a0;
				break;
			case BANDPASS_CZPG:
				m_b1a0 = 0.0f;
				m_b0a0 = alpha * a0;
				m_b2a0 = -m_b0a0;
				break;
			case NOTCH:
				m_b1a0 = m_a1a0;
				m_b0a0 = a0;
				m_b2a0 = a0;
				break;
			case ALLPASS:
				m_b1a0 = m_a1a0;
				m_b0a0 = m_a2a0;
				m_b2a0 = 1.0f;//(1.0f+alpha)*a0;
				break;
			default:
				break;
		}

		if( m_double_filter )
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

	typedef sample_t frame[CHANNELS];

	// in/out history
	frame m_ou1, m_ou2, m_in1, m_in2;

	// in/out history for moog-filter
	frame m_y1, m_y2, m_y3, m_y4, m_oldx, m_oldy1, m_oldy2, m_oldy3;

	filterTypes m_type;
	bool m_double_filter;

	float m_sampleRate;
	basicFilters<CHANNELS> * m_subFilter;

} ;


#endif
