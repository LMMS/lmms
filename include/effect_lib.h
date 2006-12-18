/*
 * effect_lib.h - library with template-based inline-effects
 *
 * Copyright (c) 2006 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include "templates.h"
#include "types.h"

namespace effectLib
{
	template<typename SAMPLE = sample_t>
	class monoBase
	{
	public:
		typedef SAMPLE sampleType;
		virtual ~monoBase()
		{
		}
		virtual SAMPLE nextSample( const SAMPLE _in ) const = 0;
	} ;

	template<typename SAMPLE = sample_t>
	class stereoBase
	{
	public:
		virtual ~stereoBase()
		{
		}
		typedef SAMPLE sampleType;
		virtual void nextSample( SAMPLE & _in_left,
						SAMPLE & _in_right ) const = 0;
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


	template<typename SAMPLE>
	inline SAMPLE saturate( const SAMPLE _x )
	{
		return( tMin<SAMPLE>( tMax<SAMPLE>( -1.0f, _x ), 1.0f ) );
	}


	template<typename SAMPLE = sample_t>
	class bassBoost : public monoBase<SAMPLE>
	{
	public:
		bassBoost( const float _selectivity,
				const float _gain,
				const float _ratio,
				const bassBoost<SAMPLE> & _orig =
							bassBoost<SAMPLE>() ) :
			m_selectivity( tMax<SAMPLE>( _selectivity, 10.0f ) ),
			m_gain1( 1.0f / ( m_selectivity + 1.0f ) ),
			m_gain2( _gain ),
			m_ratio( _ratio ),
			m_cap( _orig.m_cap )
		{
		}

		virtual ~bassBoost()
		{
		}

		virtual SAMPLE nextSample( const SAMPLE _in ) const
		{
			m_cap = ( _in + m_cap*m_selectivity ) * m_gain1;
			return( /*saturate<SAMPLE>(*/ ( _in + m_cap*m_ratio ) *
								m_gain2/* )*/ );
		}

		void setSelectivity( const float _selectivity )
		{
			m_selectivity = _selectivity;
			m_gain1 = 1.0f / ( m_selectivity + 1.0f );
		}

		void setGain( const float _gain )
		{
			m_gain2 = _gain;
		}

		void setRatio( const float _ratio )
		{
			m_ratio = _ratio;
		}

	private:
		bassBoost() :
			m_cap( 0.0f )
		{
		}

		float m_selectivity;
		float m_gain1;
		float m_gain2;
		float m_ratio;
		mutable float m_cap;
	} ;


	template<typename SAMPLE = sample_t>
	class stereoEnhancer : public stereoBase<SAMPLE>
	{
	public:
		stereoEnhancer( const float _wide_coeff ) :
			m_wideCoeff( _wide_coeff )
		{
		}

		virtual void nextSample( SAMPLE & _in_left,
						SAMPLE & _in_right ) const
		{
			const float delta = ( _in_left -
				( _in_left+_in_right ) / 2.0f ) * m_wideCoeff;
			_in_left += delta;
			_in_right -= delta;
		}

	private:
		const float m_wideCoeff;
	} ;

} ;


#endif
