/*
 * effect_lib.h - library with template-based inline-effects
 *
 * Copyright (c) 2006-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifndef _EFFECT_LIB_H
#define _EFFECT_LIB_H

#include <math.h>

#include "templates.h"
#include "lmms_constants.h"
#include "types.h"


namespace effectLib
{

	template<typename SAMPLE> class monoBypass;
	template<typename SAMPLE> class stereoBypass;


	template<typename SAMPLE = sample_t>
	class monoBase
	{
	public:
		typedef SAMPLE sampleType;
		typedef monoBypass<SAMPLE> bypassType;

		virtual ~monoBase()
		{
		}
		virtual SAMPLE nextSample( const SAMPLE _in ) const = 0;
		virtual void process( SAMPLE * * _buf,
						const f_cnt_t _frames ) const
		{
			for( f_cnt_t f = 0; f < _frames; ++f )
			{
				_buf[f][0] = nextSample( _buf[f][0] );
			}
		}
	} ;

	template<typename SAMPLE = sample_t>
	class stereoBase
	{
	public:
		typedef SAMPLE sampleType;
		typedef stereoBypass<SAMPLE> bypassType;

		virtual ~stereoBase()
		{
		}
		virtual void nextSample( SAMPLE & _in_left,
						SAMPLE & _in_right ) const = 0;
		virtual void process( SAMPLE * * _buf,
						const f_cnt_t _frames ) const
		{
			for( f_cnt_t f = 0; f < _frames; ++f )
			{
				nextSample( _buf[f][0], _buf[f][1] );
			}
		}
	} ;


	template<class FXL, class FXR = FXL>
	class monoToStereoAdaptor : public stereoBase<typename FXL::sampleType>
	{
	public:
		typedef typename FXL::sampleType sampleType;

		monoToStereoAdaptor( const FXL & _mono_fx ) :
			m_leftFX( _mono_fx ),
			m_rightFX( _mono_fx )
		{
		}

		monoToStereoAdaptor( const FXL & _left_fx,
						const FXR & _right_fx ) :
			m_leftFX( _left_fx ),
			m_rightFX( _right_fx )
		{
		}

		virtual void nextSample( sampleType & _in_left,
						sampleType & _in_right ) const
		{
			_in_left = m_leftFX.nextSample( _in_left );
			_in_right = m_rightFX.nextSample( _in_right );
		}

		FXL & leftFX( void )
		{
			return( m_leftFX );
		}

		FXR & rightFX( void )
		{
			return( m_rightFX );
		}

	private:
		FXL m_leftFX;
		FXR m_rightFX;
	} ;


	template<class FX>
	class stereoToMonoAdaptor : public monoBase<typename FX::sampleType>
	{
	public:
		typedef typename FX::sampleType sampleType;

		stereoToMonoAdaptor( const FX & _stereo_fx ) :
			m_FX( _stereo_fx )
		{
		}

		virtual sampleType nextSample( const sampleType _in ) const
		{
			sampleType s[2] = { _in, _in };
			m_FX.nextSample( s[0], s[1] );
			return( ( s[0] + s[1] ) / 2.0f );
		}

	private:
		FX m_FX;
	} ;


	template<typename SAMPLE = sample_t>
	class monoBypass : public monoBase<SAMPLE>
	{
	public:
		virtual SAMPLE nextSample( const SAMPLE _in ) const
		{
			return( _in );
		}
	} ;


	template<typename SAMPLE = sample_t>
	class stereoBypass : public stereoBase<SAMPLE>
	{
	public:
		virtual void nextSample( SAMPLE &, SAMPLE & ) const
		{
		}
	} ;


	/* convenient class to build up static FX-chains, for example

	using namespace effectLib;
	chain<monoToStereoAdaptor<bassBoost<> >,
		chain<stereoEnhancer<>,
			monoToStereoAdaptor<foldbackDistortion<> > > >
				fxchain( bassBoost<>( 60.0, 1.0, 4.0f ),
		chain<stereoEnhancer<>,
			monoToStereoAdaptor<foldbackDistortion<> > >(
				stereoEnhancer<>( 1.0 ),
					foldbackDistortion<>( 1.0f, 1.0f ) ) );

	// now you can do simple calls such as which will process a bass-boost-,
	// stereo-enhancer- and foldback-distortion-effect on your buffer
        fx_chain.process( (sample_t * *) buf, frames );
*/

	template<class FX0, class FX1 = typename FX0::bypassType>
	class chain : public FX0::bypassType
	{
	public:
		typedef typename FX0::sampleType sampleType;
		chain( const FX0 & _fx0, const FX1 & _fx1 = FX1() ) :
			m_FX0( _fx0 ),
			m_FX1( _fx1 )
		{
		}

		virtual void process( sampleType * * _buf,
						const f_cnt_t _frames ) const
		{
			m_FX0.process( _buf, _frames );
			m_FX1.process( _buf, _frames );
		}

	private:
		FX0 m_FX0;
		FX1 m_FX1;
	} ;



	template<typename SAMPLE>
	inline SAMPLE saturate( const SAMPLE _x )
	{
		return( tMin<SAMPLE>( tMax<SAMPLE>( -1.0f, _x ), 1.0f ) );
	}


	template<typename SAMPLE = sample_t>
	class fastBassBoost : public monoBase<SAMPLE>
	{
	public:
		fastBassBoost( const SAMPLE _frequency,
				const SAMPLE _gain,
				const SAMPLE _ratio,
				const fastBassBoost<SAMPLE> & _orig =
						fastBassBoost<SAMPLE>() ) :
			m_frequency( tMax<SAMPLE>( _frequency, 10.0 ) ),
			m_gain1( 1.0 / ( m_frequency + 1.0 ) ),
			m_gain2( _gain ),
			m_ratio( _ratio ),
			m_cap( _orig.m_cap )
		{
		}

		virtual ~fastBassBoost()
		{
		}

		virtual SAMPLE nextSample( const SAMPLE _in ) const
		{
			// TODO: somehow remove these horrible aliases...
			m_cap = ( _in + m_cap*m_frequency ) * m_gain1;
			return( /*saturate<SAMPLE>(*/ ( _in + m_cap*m_ratio ) *
								m_gain2/* )*/ );
		}

		void setFrequency( const SAMPLE _frequency )
		{
			m_frequency = _frequency;
			m_gain1 = 1.0 / ( m_frequency + 1.0 );
		}

		void setGain( const SAMPLE _gain )
		{
			m_gain2 = _gain;
		}

		void setRatio( const SAMPLE _ratio )
		{
			m_ratio = _ratio;
		}

	private:
		fastBassBoost() :
			m_cap( 0.0 )
		{
		}

		SAMPLE m_frequency;
		SAMPLE m_gain1;
		SAMPLE m_gain2;
		SAMPLE m_ratio;
		mutable SAMPLE m_cap;
	} ;


	// for some reason this effect doesn't work... (in=out)
	template<typename SAMPLE = sample_t>
	class bassBoost : public monoBase<SAMPLE>
	{
	public:
		bassBoost( const float _frequency,
				const float _gain,
				const float _ratio,
				const float _sample_rate ) :
			m_gain( _gain ),
			m_beta( -1.0f ),
			m_shape( 1.0f ),
			m_sampleRate( _sample_rate )
		{
			xn1=xn2=yn1=yn2=0.0f;
			setFrequency( _frequency );
			setRatio( _ratio );
		}

		virtual ~bassBoost()
		{
		}

		virtual SAMPLE nextSample( const SAMPLE _in ) const
		{
			const float out = ( m_b0*_in + m_b1*xn1 + m_b2*xn2 -
						m_a1*yn1 - m_a2*yn2 ) / m_a0;
			xn2 = xn1;
			xn1 = _in;
			yn2 = yn1;
			yn1 = out;
			return( out*m_gain );
		}

		void setFrequency( const float _frequency )
		{
			const float omega = 2*F_PI*_frequency/m_sampleRate;
			m_sin = sinf( omega );
			m_cos = cosf( omega );
			if( m_beta > 0 )
			{
				updateFilter();
			}
		}

		void setGain( const float _gain )
		{
			m_gain = _gain;
		}

		void setRatio( const float _ratio )
		{
			m_a = expf( logf( 1.0f ) * _ratio / 40 );
			updateFilter();
		}

	private:
		void updateFilter( void )
		{
			m_beta = sqrtf( ( m_a*m_a + 1 ) / m_shape -
							powf( m_a - 1, 2 ) );
			m_b0 = m_a*((m_a+1)-(m_a-1)*m_cos + m_beta*m_sin);
			m_b1 = 2*m_a*((m_a-1) - (m_a+1) * m_cos);
			m_b2 = m_a*((m_a+1) - (m_a-1)*m_cos - m_beta*m_sin);
			m_a0 = ((m_a+1) + (m_a-1)*m_cos + m_beta*m_sin);
			m_a1 = -2*((m_a-1) + (m_a+1) * m_cos);
			m_a2 = (m_a+1) + (m_a-1)*m_cos-m_beta*m_sin;
		}
		float m_gain;
		float m_a;
		float m_sin;
		float m_cos;
		float m_beta;
		const float m_shape;
		const float m_sampleRate;
		float m_b0, m_b1, m_b2, m_a0, m_a1, m_a2;
		mutable float xn1, xn2, yn1, yn2;
	} ;


	template<typename SAMPLE = sample_t>
	class foldbackDistortion : public monoBase<SAMPLE>
	{
	public:
		foldbackDistortion( const float _threshold,
							const float _gain ) :
			m_threshold( _threshold ),
			m_gain( _gain )
		{
		}

		virtual SAMPLE nextSample( const SAMPLE _in ) const
		{
			if( _in >= m_threshold || _in < -m_threshold )
			{
				return( ( fabsf( fabsf(
					fmodf( _in - m_threshold,
							m_threshold*4 ) ) -
							m_threshold*2 ) -
							m_threshold ) *
								m_gain );
			}
			return( _in * m_gain );
		}

		void setThreshold( const float _threshold )
		{
			m_threshold = _threshold;
		}

		void setGain( const float _gain )
		{
			m_gain = _gain;
		}


	private:
		float m_threshold;
		float m_gain;

	} ;


	template<typename SAMPLE = sample_t>
	class distortion : public monoBase<SAMPLE>
	{
	public:
		distortion( const float _threshold, const float _gain ) :
			m_threshold( _threshold ),
			m_gain( _gain )
		{
		}

		virtual SAMPLE nextSample( const SAMPLE _in ) const
		{
			return( m_gain * ( _in * ( fabsf( _in )+m_threshold ) /
					( _in*_in +( m_threshold-1 )*
							fabsf( _in ) + 1 ) ) );
		}

		void setThreshold( const float _threshold )
		{
			m_threshold = _threshold;
		}

		void setGain( const float _gain )
		{
			m_gain = _gain;
		}


	private:
		float m_threshold;
		float m_gain;

	} ;


	template<typename SAMPLE = sample_t>
	class stereoEnhancer : public stereoBase<SAMPLE>
	{
	public:
		stereoEnhancer( const float _wide_coeff ) :
			m_wideCoeff( _wide_coeff )
		{
		}
		
		// Lou's Hack
		void setWideCoeff( const float _wideCoeff )
		{
			m_wideCoeff = _wideCoeff;
		}
		
		float getWideCoeff()
		{
			return m_wideCoeff;
		}
		// -----------

		virtual void nextSample( SAMPLE & _in_left,
						SAMPLE & _in_right ) const
		{
			/*
			const float delta = ( _in_left -
				( _in_left+_in_right ) / 2.0f ) * m_wideCoeff;
			_in_left += delta;
			_in_right -= delta;
			*/

			
			// Lou's Hack
			// I really can't tell you why this math works, but it sounds good
			float toRad = 3.141592 / 180;
			_in_left += _in_right * sinf( m_wideCoeff * .5 * toRad);
			_in_right -= _in_left * sinf( m_wideCoeff * .5 * toRad);
			
		}

	private:
		// Lou's Hack
		float m_wideCoeff;
		//-----------
	} ;

} ;


#endif
