/*
 * DspEffectLibrary.h - library with template-based inline-effects
 *
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

#ifndef LMMS_DSPEFFECTLIBRARY_H
#define LMMS_DSPEFFECTLIBRARY_H

#include <numbers>

#include "lmms_math.h"
#include "LmmsTypes.h"
#include "SampleFrame.h"

namespace lmms::DspEffectLibrary
{

	template<typename T>
	class MonoBase
	{
	public:
		using bypassType = class MonoBypass;

		static void process( sample_t * * _buf, const f_cnt_t _frames )
		{
			for( f_cnt_t f = 0; f < _frames; ++f )
			{
				_buf[f][0] = T::nextSample( _buf[f][0] );
			}
		}
	} ;

	template<typename T>
	class StereoBase
	{
	public:
		using bypassType = class StereoBypass;

		static void process( sample_t * * _buf, const f_cnt_t _frames )
		{
			for( f_cnt_t f = 0; f < _frames; ++f )
			{
				T::nextSample( _buf[f][0], _buf[f][1] );
			}
		}
	} ;


	template<class FXL, class FXR = FXL>
	class MonoToStereoAdaptor : public StereoBase<MonoToStereoAdaptor<FXL, FXR> >
	{
	public:
		MonoToStereoAdaptor( const FXL& monoFX ) :
			m_leftFX( monoFX ),
			m_rightFX( monoFX )
		{
		}

		MonoToStereoAdaptor( const FXL& leftFX, const FXR& rightFX ) :
			m_leftFX( leftFX ),
			m_rightFX( rightFX )
		{
		}

		void setGain(float gain)
		{
			leftFX().setGain(gain);
			rightFX().setGain(gain);
		}

		void nextSample(SampleFrame & in)
		{
			nextSample(in.left(), in.right());
		}

		void nextSample( sample_t& inLeft, sample_t& inRight )
		{
			inLeft = m_leftFX.nextSample( inLeft );
			inRight = m_rightFX.nextSample( inRight );
		}

		FXL& leftFX()
		{
			return( m_leftFX );
		}

		FXR& rightFX()
		{
			return( m_rightFX );
		}

	private:
		FXL m_leftFX;
		FXR m_rightFX;
	} ;


	template<class FX>
	class StereoToMonoAdaptor : public MonoBase<StereoToMonoAdaptor<FX> >
	{
	public:
		StereoToMonoAdaptor( const FX& fx ) :
			m_FX( fx )
		{
		}

		sample_t nextSample( sample_t in )
		{
			sample_t s[2] = { in, in };
			m_FX.nextSample( s[0], s[1] );

			return ( s[0] + s[1] ) / 2.0f;
		}

	private:
		FX m_FX;

	} ;

	class MonoBypass : public MonoBase<MonoBypass>
	{
	public:
		sample_t nextSample( sample_t in )
		{
			return in;
		}
	} ;


	class StereoBypass : public StereoBase<StereoBypass>
	{
	public:
		void nextSample( sample_t&, sample_t& )
		{
		}
	} ;

	/* convenient class to build up static FX chains, for example

	using namespace DspEffectLib;
	chain<MonoToStereoAdaptor<bassBoost<> >,
		chain<StereoEnhancer<>,
			MonoToStereoAdaptor<FoldbackDistortion<> > > >
				fxchain( bassBoost<>( 60.0, 1.0, 4.0f ),
		chain<StereoEnhancer<>,
			MonoToStereoAdaptor<FoldbackDistortion<> > >(
				StereoEnhancer<>( 1.0 ),
					FoldbackDistortion<>( 1.0f, 1.0f ) ) );

	// now you can do simple calls such as which will process a bass-boost-,
	// stereo enhancer- and foldback distortion effect on your buffer
        fx_chain.process( (sample_t * *) buf, frames );
*/

	template<class FX0, class FX1 = typename FX0::bypassType>
	class Chain : public FX0::bypassType
	{
	public:
		using sample_t = typename FX0::sample_t;
		Chain( const FX0& fx0, const FX1& fx1 = FX1() ) :
			m_FX0( fx0 ),
			m_FX1( fx1 )
		{
		}

		void process( sample_t** buf, const f_cnt_t frames )
		{
			m_FX0.process( buf, frames );
			m_FX1.process( buf, frames );
		}

	private:
		FX0 m_FX0;
		FX1 m_FX1;

	} ;



	template<typename sample_t>
	inline sample_t saturate( sample_t x )
	{
		return std::min<sample_t>(std::max<sample_t>(-1.0f, x), 1.0f);
	}


	class FastBassBoost : public MonoBase<FastBassBoost>
	{
	public:
		FastBassBoost( const sample_t _frequency,
				const sample_t _gain,
				const sample_t _ratio,
				const FastBassBoost & _orig = FastBassBoost() ) :
			m_frequency(std::max<sample_t>(_frequency, 10.0)),
			m_gain1( 1.0 / ( m_frequency + 1.0 ) ),
			m_gain2( _gain ),
			m_ratio( _ratio ),
			m_cap( _orig.m_cap )
		{
		}

		inline sample_t nextSample( sample_t _in )
		{
			// TODO: somehow remove these horrible aliases...
			m_cap = ( _in + m_cap*m_frequency ) * m_gain1;
			return( ( _in + m_cap*m_ratio ) * m_gain2 );
		}

		void setFrequency( const sample_t _frequency )
		{
			m_frequency = _frequency;
			m_gain1 = 1.0 / ( m_frequency + 1.0 );
		}

		void setGain( const sample_t _gain )
		{
			m_gain2 = _gain;
		}

		void setRatio( const sample_t _ratio )
		{
			m_ratio = _ratio;
		}

	private:
		FastBassBoost() :
			m_cap( 0.0 )
		{
		}

		sample_t m_frequency;
		sample_t m_gain1;
		sample_t m_gain2;
		sample_t m_ratio;
		sample_t m_cap;
	} ;


	template<class T>
	class DistortionBase : public MonoBase<T>
	{
	public:
		DistortionBase( float threshold, float gain ) :
			m_threshold( threshold ),
			m_gain( gain )
		{
		}

		void setThreshold( float threshold )
		{
			m_threshold = threshold;
		}

		void setGain( float gain )
		{
			m_gain = gain;
		}

	protected:
		float m_threshold;
		float m_gain;
	};


	class FoldbackDistortion : public DistortionBase<FoldbackDistortion>
	{
	public:
		using DistortionBase<FoldbackDistortion>::DistortionBase;

		sample_t nextSample( sample_t in )
		{
			if( in >= m_threshold || in < -m_threshold )
			{
				return (std::abs(std::abs(std::fmod(in - m_threshold, m_threshold * 4)) - m_threshold * 2) - m_threshold) * m_gain;
			}
			return in * m_gain;
		}
	} ;


	class Distortion : public DistortionBase<Distortion>
	{
	public:
		using DistortionBase<Distortion>::DistortionBase;

		sample_t nextSample( sample_t in )
		{
			return m_gain * (in * (std::abs(in) + m_threshold) / (in * in + (m_threshold - 1) * std::abs(in) + 1));
		}
	} ;


	class StereoEnhancer : public StereoBase<StereoEnhancer>
	{
	public:
		StereoEnhancer( float wideCoeff ) :
			m_wideCoeff( wideCoeff )
		{
		}

		void setWideCoeff( float wideCoeff )
		{
			m_wideCoeff = wideCoeff;
		}

		float wideCoeff()
		{
			return m_wideCoeff;
		}

		void nextSample( sample_t& inLeft, sample_t& inRight )
		{
			constexpr float toRad = std::numbers::pi_v<float> / 180.f;
			const sample_t tmp = inLeft;
			inLeft += inRight * std::sin(m_wideCoeff * toRad * .5f);
			inRight -= tmp * std::sin(m_wideCoeff * toRad * .5f);
		}

	private:
		float m_wideCoeff;

	} ;

} // namespace lmms::DspEffectLibrary

#endif // LMMS_DSPEFFECTLIBRARY_H
