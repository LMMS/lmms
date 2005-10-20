/*
 * basic_filters.h - simple but powerful filter-class with most used filters
 *
 * original file by ??? 
 * modified and enhanced by Tobias Doerffel, 2004
 *
 * Linux MultiMedia Studio
 * Copyright (c) 2004-2005 Tobias Doerffel <tobydox@users.sourceforge.net>
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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
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

const int MOOG_VOLTAGE = 40000;

template<Uint8 CHANNELS = DEFAULT_CHANNELS>
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
		SIMPLE_FLT_CNT,
		DOUBLE_LOWPASS	= 16+LOWPASS,
		DOUBLE_MOOG	= 16+MOOG
	} ;

	static inline filterTypes getFilterType( const int _idx )
	{
		if( _idx < SIMPLE_FLT_CNT )
		{
			return( static_cast<filterTypes>( _idx ) );
		}
		return( static_cast<filterTypes>( DOUBLE_LOWPASS + _idx -
							SIMPLE_FLT_CNT ) );
	}

	inline basicFilters( const float _sampleRate ) :
		m_b0a0( 0.0f ),
		m_b1a0( 0.0f ),
		m_b2a0( 0.0f ),
		m_a1a0( 0.0f ),
		m_a2a0( 0.0f ),
		m_sampleRate( _sampleRate ),
		m_subFilter( NULL )
	{
		// reset in/out history
		for( Uint8 _chnl = 0; _chnl < CHANNELS; ++_chnl )
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

	inline ~basicFilters()
	{
		delete m_subFilter;
	}

	inline sampleType update( sampleType _in0, Uint8 _chnl )
	{
		sampleType out;
		switch( m_type )
		{
			case MOOG:
			case DOUBLE_MOOG:
			{
				sampleType x = _in0 - m_r*m_y4[_chnl];

				// four cascaded onepole filters
				// (bilinear transform)
				m_y1[_chnl] = x*m_p + m_oldx[_chnl]*m_p -
								m_k*m_y1[_chnl];
				m_y2[_chnl] = m_y1[_chnl]*m_p+m_oldy1[_chnl]*
							m_p - m_k*m_y2[_chnl];
				m_y3[_chnl] = m_y2[_chnl]*m_p+m_oldy2[_chnl]*
							m_p - m_k*m_y3[_chnl];
				m_y4[_chnl] = m_y3[_chnl]*m_p+m_oldy3[_chnl]*
							m_p - m_k*m_y4[_chnl];

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
		if( m_subFilter != NULL )
		{
			return( m_subFilter->update( out, _chnl ) );
		}
		// Clipper band limited sigmoid
		return( out );
	}

   
	inline void calcFilterCoeffs( const filterTypes _type, float _freq,
					const float _q
				/*, const bool _q_is_bandwidth = FALSE*/ )
	{
		// temp coef vars
		m_type = _type;
		_freq = tMax( _freq, 0.01f );// limit freq for not getting
					      // bad noise out of the filter...

		switch( m_type )
		{
			case DOUBLE_MOOG:
			{
				if( m_subFilter == NULL )
				{
					m_subFilter =
						new basicFilters<CHANNELS>(
								m_sampleRate );
				}
				m_subFilter->calcFilterCoeffs( MOOG, _freq,
									_q );
			}

			case MOOG:
			{
				// [ 0 - 1 ]
				const float f = 2 * _freq / m_sampleRate;
				// (Empirical tunning)
				m_k = 3.6f*f - 1.6f*f*f - 1;
				m_p = (m_k+1)*0.5f;
				m_r = _q*powf( M_E, ( ( 1-m_p ) * 1.386249f ) );
				break;
			}

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
				m_p = MOOG_VOLTAGE * ( 1 - expf( -2.0 * M_PI *
								kfcr * kf ) );
				m_r = 4 * _q * kacr;
				break;
			}*/

			default:
			{
				// other filters
				const float omega	= 2.0f * M_PI * _freq /
								m_sampleRate;
				const float tsin	= sinf( omega );
				const float tcos	= cosf( omega );
				//float alpha;
  
				//if (q_is_bandwidth)
				//alpha = tsin*sinhf(logf(2.0f)/2.0f*q*omega/
				//					tsin);
				//else
				const float alpha = tsin / ( 2.0f * _q );

				const float a0 = 1.0f / ( 1.0f+alpha );
   
				if( m_type == LOWPASS ||
						m_type == DOUBLE_LOWPASS )
				{
					m_b0a0 = ((1.0f-tcos)/2.0f)*a0;
					m_b1a0 = (1.0f-tcos)*a0;
					m_b2a0 = m_b0a0;//((1.0f-tcos)/2.0f)*a0;
					m_a1a0 = (-2.0f*tcos)*a0;
					if( m_type == DOUBLE_LOWPASS )
					{
						if( m_subFilter == NULL )
						{
							m_subFilter =
				new basicFilters<CHANNELS>( m_sampleRate );
						}
						m_subFilter->calcFilterCoeffs(
									LOWPASS,
									_freq,
									_q );
					}
				}
				else if( m_type == HIPASS )
				{
					m_b0a0 = ((1.0f+tcos)/2.0f)*a0;
					m_b1a0 = (-1.0f-tcos)*a0;
					m_b2a0 = m_b0a0;//((1.0f+tcos)/2.0f)*a0;
					m_a1a0 = (-2.0f*tcos)*a0;
				}
				else if( m_type == BANDPASS_CSG )
				{
					m_b0a0 = (tsin/2.0f)*a0;
					m_b1a0 = 0.0f;
					m_b2a0 = (-tsin/2.0f)*a0;
					m_a1a0 = (-2.0f*tcos)*a0;
				}
				else if( m_type == BANDPASS_CZPG )
				{
					m_b0a0 = alpha*a0;
					m_b1a0 = 0.0f;
					m_b2a0 = (-alpha)*a0;
					m_a1a0 = (-2.0f*tcos)*a0;
				}
				else if( m_type == NOTCH )
				{
					m_b0a0 = a0;
					m_b1a0 = (-2.0f*tcos)*a0;
					m_b2a0 = a0;
					m_a1a0 = m_b1a0;//(-2.0f*tcos)*a0;
				}
				else if( m_type == ALLPASS )
				{
					m_b0a0 = (1.0f-alpha)*a0;
					m_b1a0 = (-2.0f*tcos)*a0;
					m_b2a0 = 1.0;//(1.0f+alpha)*a0;
					m_a1a0 = m_b1a0;//(-2.0f*tcos)*a0;
					//m_a2a0 = m_b0a0;//(1.0f-alpha)*a0;
				}
				m_a2a0 = (1.0f-alpha)*a0;
				break;
			}
		}
	}


private:
	// filter coeffs
	float m_b0a0, m_b1a0, m_b2a0, m_a1a0, m_a2a0;

	// coeffs for moog-filter
	float m_r, m_p, m_k;

	typedef sampleType frame[CHANNELS];

	// in/out history
	frame m_ou1, m_ou2, m_in1, m_in2;

	// in/out history for moog-filter
	frame m_y1, m_y2, m_y3, m_y4, m_oldx, m_oldy1, m_oldy2, m_oldy3;

	filterTypes m_type;

	float m_sampleRate;
	basicFilters<CHANNELS> * m_subFilter;
} ;


#endif
