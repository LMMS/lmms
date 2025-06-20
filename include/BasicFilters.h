/*
 * BasicFilters.h - simple but powerful filter-class with most used filters
 *
 * original file by ???
 * modified and enhanced by Tobias Doerffel
 * 
 * Lowpass_SV code originally from Nekobee, Copyright (C) 2004 Sean Bolton and others
 * adapted & modified for use in LMMS
 *
 * Copyright (c) 2004-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef LMMS_BASIC_FILTERS_H
#define LMMS_BASIC_FILTERS_H

#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>

#include "lmms_constants.h"
#include "LmmsTypes.h"


namespace lmms
{

template<ch_cnt_t CHANNELS=DEFAULT_CHANNELS> class BasicFilters;

template<ch_cnt_t CHANNELS>
class LinkwitzRiley
{
public:
	LinkwitzRiley( float sampleRate )
	{
		m_sampleRate = sampleRate;
		clearHistory();
	}
	virtual ~LinkwitzRiley() = default;

	inline void clearHistory()
	{
		for( int i = 0; i < CHANNELS; ++i )
		{
			m_z1[i] = m_z2[i] = m_z3[i] = m_z4[i] = 0.0f;
		}
	}

	inline void setSampleRate( float sampleRate )
	{
		m_sampleRate = sampleRate;
	}

	inline void setCoeffs( float freq )
	{
		using namespace std::numbers;
		// wc
		const double wc = 2 * pi * freq;
		const double wc2 = wc * wc;
		const double wc3 = wc2 * wc;
		m_wc4 = wc2 * wc2;

		// k
		const double k = wc / std::tan(pi * freq / m_sampleRate);
		const double k2 = k * k;
		const double k3 = k2 * k;
		m_k4 = k2 * k2;

		// a
		const double sq_tmp1 = sqrt2 * wc3 * k;
		const double sq_tmp2 = sqrt2 * wc * k3;

		m_a = 1.0 / ( 4.0 * wc2 * k2 + 2.0 * sq_tmp1 + m_k4 + 2.0 * sq_tmp2 + m_wc4 );

		// b
		m_b1 = ( 4.0 * ( m_wc4 + sq_tmp1 - m_k4 - sq_tmp2 ) ) * m_a;
		m_b2 = ( 6.0 * m_wc4 - 8.0 * wc2 * k2 + 6.0 * m_k4 ) * m_a;
		m_b3 = ( 4.0 * ( m_wc4 - sq_tmp1 + sq_tmp2 - m_k4 ) ) * m_a;
		m_b4 = ( m_k4 - 2.0 * sq_tmp1 + m_wc4 - 2.0 * sq_tmp2 + 4.0 * wc2 * k2 ) * m_a;
	}

	inline void setLowpass( float freq )
	{
		setCoeffs( freq );
		m_a0 = m_wc4 * m_a;
		m_a1 = 4.0 * m_a0;
		m_a2 = 6.0 * m_a0;
	}
	
	inline void setHighpass( float freq )
	{
		setCoeffs( freq );
		m_a0 = m_k4 * m_a;
		m_a1 = -4.0 * m_a0;
		m_a2 = 6.0 * m_a0;
	}

	inline float update( float in, ch_cnt_t ch )
	{
		const double x = in - ( m_z1[ch] * m_b1 ) - ( m_z2[ch] * m_b2 ) -
			( m_z3[ch] * m_b3 ) - ( m_z4[ch] * m_b4 );
		const double y = ( m_a0 * x ) + ( m_z1[ch] * m_a1 ) + ( m_z2[ch] * m_a2 ) +
			( m_z3[ch] * m_a1 ) + ( m_z4[ch] * m_a0 );
		m_z4[ch] = m_z3[ch];
		m_z3[ch] = m_z2[ch];
		m_z2[ch] = m_z1[ch];
		m_z1[ch] = x;
		
		return y;
	}

private:
	float m_sampleRate;
	double m_wc4;
	double m_k4;
	double m_a, m_a0, m_a1, m_a2;
	double m_b1, m_b2, m_b3, m_b4;
	
	using frame = std::array<double, CHANNELS>;
	frame m_z1, m_z2, m_z3, m_z4;
};
using StereoLinkwitzRiley = LinkwitzRiley<2>;

template<ch_cnt_t CHANNELS>
class BiQuad
{
public:
	BiQuad() :
		m_a1(0.),
		m_a2(0.),
		m_b0(0.),
		m_b1(0.),
		m_b2(0.)
	{
		clearHistory();
	}
	virtual ~BiQuad() = default;
	
	inline void setCoeffs( float a1, float a2, float b0, float b1, float b2 )
	{
		m_a1 = a1;
		m_a2 = a2;
		m_b0 = b0;
		m_b1 = b1;
		m_b2 = b2;
	}
	inline void clearHistory()
	{
		for( int i = 0; i < CHANNELS; ++i )
		{
			m_z1[i] = 0.0f;
			m_z2[i] = 0.0f;
		}
	}
	inline float update( float in, ch_cnt_t ch )
	{
		// biquad filter in transposed form
		const float out = m_z1[ch] + m_b0 * in;
		m_z1[ch] = m_b1 * in + m_z2[ch] - m_a1 * out;
		m_z2[ch] = m_b2 * in - m_a2 * out;
		return out;
	}
private:
	float m_a1, m_a2, m_b0, m_b1, m_b2;
	float m_z1 [CHANNELS], m_z2 [CHANNELS];
	
	friend class BasicFilters<CHANNELS>; // needed for subfilter stuff in BasicFilters
};
using StereoBiQuad = BiQuad<2>;

template<ch_cnt_t CHANNELS>
class OnePole
{
public:
	OnePole()
	{
		m_a0 = 1.0; 
		m_b1 = 0.0;
		for( int i = 0; i < CHANNELS; ++i )
		{
			m_z1[i] = 0.0;
		}
	}
	virtual ~OnePole() = default;
	
	inline void setCoeffs( float a0, float b1 )
	{
		m_a0 = a0;
		m_b1 = b1;
	}
	
	inline float update( float s, ch_cnt_t ch )
	{
		if (std::abs(s) < F_EPSILON && std::abs(m_z1[ch]) < F_EPSILON) { return 0.0f; }
		return m_z1[ch] = s * m_a0 + m_z1[ch] * m_b1;
	}
	
private:
	float m_a0, m_b1; 
	float m_z1 [CHANNELS];
};
using StereoOnePole = OnePole<2>;

template<ch_cnt_t CHANNELS>
class BasicFilters
{
public:
	enum class FilterType
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
		DoubleMoog,
		Lowpass_SV,
		Bandpass_SV,
		Highpass_SV,
		Notch_SV,
		FastFormant,
		Tripole
	};

	static inline float minFreq()
	{
		return( 5.0f );
	}

	static inline float minQ()
	{
		return( 0.01f );
	}

	inline void setFilterType( const FilterType _idx )
	{
		m_doubleFilter = _idx == FilterType::DoubleLowPass || _idx == FilterType::DoubleMoog;
		if( !m_doubleFilter )
		{
			m_type = _idx;
			return;
		}

		// Double lowpass mode, backwards-compat for the goofy
		// Add-NumFilters to signify doubleFilter stuff
		m_type = _idx == FilterType::DoubleLowPass 
			? FilterType::LowPass
			: FilterType::Moog;
		if( m_subFilter == nullptr )
		{
			m_subFilter = new BasicFilters<CHANNELS>(
						static_cast<sample_rate_t>(
							m_sampleRate ) );
		}
		m_subFilter->m_type = m_type;
	}

	inline BasicFilters( const sample_rate_t _sample_rate ) :
		m_doubleFilter( false ),
		m_sampleRate( (float) _sample_rate ),
		m_sampleRatio( 1.0f / m_sampleRate ),
		m_subFilter( nullptr )
	{
		clearHistory();
	}

	inline ~BasicFilters()
	{
		delete m_subFilter;
	}

	inline void clearHistory()
	{
		// reset in/out history for biquads
		m_biQuad.clearHistory();

		// reset in/out history
		for( ch_cnt_t _chnl = 0; _chnl < CHANNELS; ++_chnl )
		{
			// reset in/out history for moog-filter
			m_y1[_chnl] = m_y2[_chnl] = m_y3[_chnl] = m_y4[_chnl] =
					m_oldx[_chnl] = m_oldy1[_chnl] =
					m_oldy2[_chnl] = m_oldy3[_chnl] = 0.0f;
			
			// tripole
			m_last[_chnl] = 0.0f;

			// reset in/out history for RC-filters
			m_rclp0[_chnl] = m_rcbp0[_chnl] = m_rchp0[_chnl] = m_rclast0[_chnl] = 0.0f;
			m_rclp1[_chnl] = m_rcbp1[_chnl] = m_rchp1[_chnl] = m_rclast1[_chnl] = 0.0f;

			for(int i=0; i<6; i++)
			   m_vfbp[i][_chnl] = m_vfhp[i][_chnl] = m_vflast[i][_chnl] = 0.0f;
			   
			// reset in/out history for SV-filters
			m_delay1[_chnl] = 0.0f;
			m_delay2[_chnl] = 0.0f;
			m_delay3[_chnl] = 0.0f;
			m_delay4[_chnl] = 0.0f;
		}
	}

	inline void setSampleRate(const sample_rate_t sampleRate)
	{
		m_sampleRate = sampleRate;
		m_sampleRatio = 1.f / m_sampleRate;
		if (m_subFilter != nullptr)
		{
			m_subFilter->setSampleRate(m_sampleRate);
		}
	}

	inline sample_t update( sample_t _in0, ch_cnt_t _chnl )
	{
		sample_t out = 0.0f;
		switch( m_type )
		{
			case FilterType::Moog:
			{
				sample_t x = _in0 - m_r*m_y4[_chnl];

				// four cascaded onepole filters
				// (bilinear transform)
				m_y1[_chnl] = std::clamp((x + m_oldx[_chnl]) * m_p
							- m_k * m_y1[_chnl], -10.0f,
								10.0f);
				m_y2[_chnl] = std::clamp((m_y1[_chnl] + m_oldy1[_chnl]) * m_p
							- m_k * m_y2[_chnl], -10.0f,
								10.0f);
				m_y3[_chnl] = std::clamp((m_y2[_chnl] + m_oldy2[_chnl]) * m_p
							- m_k * m_y3[_chnl], -10.0f,
								10.0f );
				m_y4[_chnl] = std::clamp((m_y3[_chnl] + m_oldy3[_chnl]) * m_p
							- m_k * m_y4[_chnl], -10.0f,
								10.0f);

				m_oldx[_chnl] = x;
				m_oldy1[_chnl] = m_y1[_chnl];
				m_oldy2[_chnl] = m_y2[_chnl];
				m_oldy3[_chnl] = m_y3[_chnl];
				out = m_y4[_chnl] - m_y4[_chnl] * m_y4[_chnl] *
						m_y4[_chnl] * ( 1.0f / 6.0f );
				break;
			}
			
			// 3x onepole filters with 4x oversampling and interpolation of oversampled signal:
			// input signal is linear-interpolated after oversampling, output signal is averaged from oversampled outputs
			case FilterType::Tripole:
			{
				float ip = 0.0f;
				for( int i = 0; i < 4; ++i )
				{
					ip += 0.25f;
					sample_t x = std::lerp(m_last[_chnl], _in0, ip) - m_r * m_y3[_chnl];
					
					m_y1[_chnl] = std::clamp((x + m_oldx[_chnl]) * m_p
							- m_k * m_y1[_chnl], -10.0f,
								10.0f);
					m_y2[_chnl] = std::clamp((m_y1[_chnl] + m_oldy1[_chnl]) * m_p
								- m_k * m_y2[_chnl], -10.0f,
									10.0f);
					m_y3[_chnl] = std::clamp((m_y2[_chnl] + m_oldy2[_chnl]) * m_p
								- m_k * m_y3[_chnl], -10.0f,
									10.0f);
					m_oldx[_chnl] = x;
					m_oldy1[_chnl] = m_y1[_chnl];
					m_oldy2[_chnl] = m_y2[_chnl];
					
					out += ( m_y3[_chnl] - m_y3[_chnl] * m_y3[_chnl] * m_y3[_chnl] * ( 1.0f / 6.0f ) );
				}
				out *= 0.25f;
				m_last[_chnl] = _in0;
				return out;
			}
			
			// 4-pole state-variant lowpass filter, adapted from Nekobee source code
			// and extended to other SV filter types
			// /* Hal Chamberlin's state variable filter */
			
			case FilterType::Lowpass_SV:
			case FilterType::Bandpass_SV:
			{
				float highpass;
				
				for( int i = 0; i < 2; ++i ) // 2x oversample
				{
					m_delay2[_chnl] = m_delay2[_chnl] + m_svf1 * m_delay1[_chnl];				/* delay2/4 = lowpass output */
					highpass = _in0 - m_delay2[_chnl] - m_svq * m_delay1[_chnl];
					m_delay1[_chnl] = m_svf1 * highpass + m_delay1[_chnl];           			/* delay1/3 = bandpass output */

					m_delay4[_chnl] = m_delay4[_chnl] + m_svf2 * m_delay3[_chnl];
					highpass = m_delay2[_chnl] - m_delay4[_chnl] - m_svq * m_delay3[_chnl];
					m_delay3[_chnl] = m_svf2 * highpass + m_delay3[_chnl];
				}

				/* mix filter output into output buffer */
				return m_type == FilterType::Lowpass_SV 
					? m_delay4[_chnl]
					: m_delay3[_chnl];
			}
			
			case FilterType::Highpass_SV:
			{
				float hp;
				for( int i = 0; i < 2; ++i ) // 2x oversample
				{				
					m_delay2[_chnl] = m_delay2[_chnl] + m_svf1 * m_delay1[_chnl];
					hp = _in0 - m_delay2[_chnl] - m_svq * m_delay1[_chnl];
					m_delay1[_chnl] = m_svf1 * hp + m_delay1[_chnl];
				}
				
				return hp;
			}
			
			case FilterType::Notch_SV:
			{
				float hp1;
				for( int i = 0; i < 2; ++i ) // 2x oversample
				{
					m_delay2[_chnl] = m_delay2[_chnl] + m_svf1 * m_delay1[_chnl];				/* delay2/4 = lowpass output */
					hp1 = _in0 - m_delay2[_chnl] - m_svq * m_delay1[_chnl];
					m_delay1[_chnl] = m_svf1 * hp1 + m_delay1[_chnl];           			/* delay1/3 = bandpass output */

					m_delay4[_chnl] = m_delay4[_chnl] + m_svf2 * m_delay3[_chnl];
					float hp2 = m_delay2[_chnl] - m_delay4[_chnl] - m_svq * m_delay3[_chnl];
					m_delay3[_chnl] = m_svf2 * hp2 + m_delay3[_chnl];
				}

				/* mix filter output into output buffer */
				return m_delay4[_chnl] + hp1;
			}


			// 4-times oversampled simulation of an active RC-Bandpass,-Lowpass,-Highpass-
			// Filter-Network as it was used in nearly all modern analog synthesizers. This
			// can be driven up to self-oscillation (BTW: do not remove the limits!!!).
			// (C) 1998 ... 2009 S.Fendt. Released under the GPL v2.0  or any later version.

			case FilterType::Lowpass_RC12:
			{
				sample_t lp = 0.0f;
				for( int n = 4; n != 0; --n )
				{
					sample_t in = _in0 + m_rcbp0[_chnl] * m_rcq;
					in = std::clamp(in, -1.0f, 1.0f);

					lp = in * m_rcb + m_rclp0[_chnl] * m_rca;
					lp = std::clamp(lp, -1.0f, 1.0f);

					sample_t hp = m_rcc * (m_rchp0[_chnl] + in - m_rclast0[_chnl]);
					hp = std::clamp(hp, -1.0f, 1.0f);

					sample_t bp = hp * m_rcb + m_rcbp0[_chnl] * m_rca;
					bp = std::clamp(bp, -1.0f, 1.0f);

					m_rclast0[_chnl] = in;
					m_rclp0[_chnl] = lp;
					m_rchp0[_chnl] = hp;
					m_rcbp0[_chnl] = bp;
				}
				return lp;
			}
			case FilterType::Highpass_RC12:
			case FilterType::Bandpass_RC12:
			{
				sample_t hp, bp;
				for( int n = 4; n != 0; --n )
				{
					sample_t in = _in0 + m_rcbp0[_chnl] * m_rcq;
					in = std::clamp(in, -1.0f, 1.0f);

					hp = m_rcc * ( m_rchp0[_chnl] + in - m_rclast0[_chnl] );
					hp = std::clamp(hp, -1.0f, 1.0f);

					bp = hp * m_rcb + m_rcbp0[_chnl] * m_rca;
					bp = std::clamp(bp, -1.0f, 1.0f);

					m_rclast0[_chnl] = in;
					m_rchp0[_chnl] = hp;
					m_rcbp0[_chnl] = bp;
				}
				return m_type == FilterType::Highpass_RC12 ? hp : bp;
			}

			case FilterType::Lowpass_RC24:
			{
				sample_t lp;
				for( int n = 4; n != 0; --n )
				{
					// first stage is as for the 12dB case...
					sample_t in = _in0 + m_rcbp0[_chnl] * m_rcq;
					in = std::clamp(in, -1.0f, 1.0f);

					lp = in * m_rcb + m_rclp0[_chnl] * m_rca;
					lp = std::clamp(lp, -1.0f, 1.0f);

					sample_t hp = m_rcc * ( m_rchp0[_chnl] + in - m_rclast0[_chnl] );
					hp = std::clamp(hp, -1.0f, 1.0f);

					sample_t bp = hp * m_rcb + m_rcbp0[_chnl] * m_rca;
					bp = std::clamp(bp, -1.0f, 1.0f);

					m_rclast0[_chnl] = in;
					m_rclp0[_chnl] = lp;
					m_rcbp0[_chnl] = bp;
					m_rchp0[_chnl] = hp;

					// second stage gets the output of the first stage as input...
					in = lp + m_rcbp1[_chnl] * m_rcq;
					in = std::clamp(in, -1.0f, 1.0f );

					lp = in * m_rcb + m_rclp1[_chnl] * m_rca;
					lp = std::clamp(lp, -1.0f, 1.0f);

					hp = m_rcc * ( m_rchp1[_chnl] + in - m_rclast1[_chnl] );
					hp = std::clamp(hp, -1.0f, 1.0f);

					bp = hp * m_rcb + m_rcbp1[_chnl] * m_rca;
					bp = std::clamp(bp, -1.0f, 1.0f);

					m_rclast1[_chnl] = in;
					m_rclp1[_chnl] = lp;
					m_rcbp1[_chnl] = bp;
					m_rchp1[_chnl] = hp;
				}
				return lp;
			}
			case FilterType::Highpass_RC24:
			case FilterType::Bandpass_RC24:
			{
				sample_t hp, bp;
				for( int n = 4; n != 0; --n )
				{
					// first stage is as for the 12dB case...
					sample_t in = _in0 + m_rcbp0[_chnl] * m_rcq;
					in = std::clamp(in, -1.0f, 1.0f);

					hp = m_rcc * ( m_rchp0[_chnl] + in - m_rclast0[_chnl] );
					hp = std::clamp(hp, -1.0f, 1.0f);

					bp = hp * m_rcb + m_rcbp0[_chnl] * m_rca;
					bp = std::clamp(bp, -1.0f, 1.0f);

					m_rclast0[_chnl] = in;
					m_rchp0[_chnl] = hp;
					m_rcbp0[_chnl] = bp;

					// second stage gets the output of the first stage as input...
					in = m_type == FilterType::Highpass_RC24
						? hp + m_rcbp1[_chnl] * m_rcq
						: bp + m_rcbp1[_chnl] * m_rcq;

					in = std::clamp(in, -1.0f, 1.0f);

					hp = m_rcc * ( m_rchp1[_chnl] + in - m_rclast1[_chnl] );
					hp = std::clamp(hp, -1.0f, 1.0f);

					bp = hp * m_rcb + m_rcbp1[_chnl] * m_rca;
					bp = std::clamp(bp, -1.0f, 1.0f);

					m_rclast1[_chnl] = in;
					m_rchp1[_chnl] = hp;
					m_rcbp1[_chnl] = bp;
				}
				return m_type == FilterType::Highpass_RC24 ? hp : bp;
			}

			case FilterType::Formantfilter:
			case FilterType::FastFormant:
			{
				if (std::abs(_in0) < F_EPSILON && std::abs(m_vflast[0][_chnl]) < F_EPSILON) { return 0.0f; } // performance hack - skip processing when the numbers get too small

				const int os = m_type == FilterType::FastFormant ? 1 : 4; // no oversampling for fast formant
				for( int o = 0; o < os; ++o )
				{
					// first formant
					sample_t in = _in0 + m_vfbp[0][_chnl] * m_vfq;
					in = std::clamp(in, -1.0f, 1.0f);

					sample_t hp = m_vfc[0] * ( m_vfhp[0][_chnl] + in - m_vflast[0][_chnl] );
					hp = std::clamp(hp, -1.0f, 1.0f);

					sample_t bp = hp * m_vfb[0] + m_vfbp[0][_chnl] * m_vfa[0];
					bp = std::clamp(bp, -1.0f, 1.0f);

					m_vflast[0][_chnl] = in;
					m_vfhp[0][_chnl] = hp;
					m_vfbp[0][_chnl] = bp;

					in = bp + m_vfbp[2][_chnl] * m_vfq;
					in = std::clamp(in, -1.0f, 1.0f);

					hp = m_vfc[0] * ( m_vfhp[2][_chnl] + in - m_vflast[2][_chnl] );
					hp = std::clamp(hp, -1.0f, 1.0f);

					bp = hp * m_vfb[0] + m_vfbp[2][_chnl] * m_vfa[0];
					bp = std::clamp(bp, -1.0f, 1.0f);

					m_vflast[2][_chnl] = in;
					m_vfhp[2][_chnl] = hp;
					m_vfbp[2][_chnl] = bp;

					in = bp + m_vfbp[4][_chnl] * m_vfq;
					in = std::clamp(in, -1.0f, 1.0f);

					hp = m_vfc[0] * ( m_vfhp[4][_chnl] + in - m_vflast[4][_chnl] );
					hp = std::clamp(hp, -1.0f, 1.0f);

					bp = hp * m_vfb[0] + m_vfbp[4][_chnl] * m_vfa[0];
					bp = std::clamp(bp, -1.0f, 1.0f);

					m_vflast[4][_chnl] = in;
					m_vfhp[4][_chnl] = hp;
					m_vfbp[4][_chnl] = bp;

					out += bp;

					// second formant
					in = _in0 + m_vfbp[0][_chnl] * m_vfq;
					in = std::clamp(in, -1.0f, 1.0f);

					hp = m_vfc[1] * ( m_vfhp[1][_chnl] + in - m_vflast[1][_chnl] );
					hp = std::clamp(hp, -1.0f, 1.0f);

					bp = hp * m_vfb[1] + m_vfbp[1][_chnl] * m_vfa[1];
					bp = std::clamp(bp, -1.0f, 1.0f);

					m_vflast[1][_chnl] = in;
					m_vfhp[1][_chnl] = hp;
					m_vfbp[1][_chnl] = bp;

					in = bp + m_vfbp[3][_chnl] * m_vfq;
					in = std::clamp(in, -1.0f, 1.0f);

					hp = m_vfc[1] * ( m_vfhp[3][_chnl] + in - m_vflast[3][_chnl] );
					hp = std::clamp(hp, -1.0f, 1.0f);

					bp = hp * m_vfb[1] + m_vfbp[3][_chnl] * m_vfa[1];
					bp = std::clamp(bp, -1.0f, 1.0f);

					m_vflast[3][_chnl] = in;
					m_vfhp[3][_chnl] = hp;
					m_vfbp[3][_chnl] = bp;

					in = bp + m_vfbp[5][_chnl] * m_vfq;
					in = std::clamp(in, -1.0f, 1.0f);

					hp = m_vfc[1] * ( m_vfhp[5][_chnl] + in - m_vflast[5][_chnl] );
					hp = std::clamp(hp, -1.0f, 1.0f);

					bp = hp * m_vfb[1] + m_vfbp[5][_chnl] * m_vfa[1];
					bp = std::clamp(bp, -1.0f, 1.0f);

					m_vflast[5][_chnl] = in;
					m_vfhp[5][_chnl] = hp;
					m_vfbp[5][_chnl] = bp;

					out += bp;
				}
            	return m_type == FilterType::FastFormant ? out * 2.0f : out * 0.5f;
			}

			default:
				out = m_biQuad.update( _in0, _chnl );
				break;
		}

		if( m_doubleFilter )
		{
			return m_subFilter->update( out, _chnl );
		}

		// Clipper band limited sigmoid
		return out;
	}


	inline void calcFilterCoeffs( float _freq, float _q )
	{
		using namespace std::numbers;
		// temp coef vars
		_q = std::max(_q, minQ());

		if( m_type == FilterType::Lowpass_RC12  ||
			m_type == FilterType::Bandpass_RC12 ||
			m_type == FilterType::Highpass_RC12 ||
			m_type == FilterType::Lowpass_RC24 ||
			m_type == FilterType::Bandpass_RC24 ||
			m_type == FilterType::Highpass_RC24 )
		{
			_freq = std::clamp(_freq, 50.0f, 20000.0f);
			const float sr = m_sampleRatio * 0.25f;
			const float f = 1.0f / (_freq * 2 * pi_v<float>);
			
			m_rca = 1.0f - sr / ( f + sr );
			m_rcb = 1.0f - m_rca;
			m_rcc = f / ( f + sr );

			// Stretch Q/resonance, as self-oscillation reliably starts at a q of ~2.5 - ~2.6
			m_rcq = _q * 0.25f;
			return;
		}

		if( m_type == FilterType::Formantfilter ||
			m_type == FilterType::FastFormant )
		{
			_freq = std::clamp(_freq, minFreq(), 20000.0f); // limit freq and q for not getting bad noise out of the filter...

			// formats for a, e, i, o, u, a
			static const float _f[6][2] = { { 1000, 1400 }, { 500, 2300 },
							{ 320, 3200 },
							{ 500, 1000 },
							{ 320, 800 },
							{ 1000, 1400 } };
			static const float freqRatio = 4.0f / 14000.0f;

			// Stretch Q/resonance
			m_vfq = _q * 0.25f;

			// frequency in lmms ranges from 1Hz to 14000Hz
			const float vowelf = _freq * freqRatio;
			const int vowel = static_cast<int>( vowelf );
			const float fract = vowelf - vowel;

			// interpolate between formant frequencies
			const float f0 = 1.f / (std::lerp(_f[vowel+0][0], _f[vowel+1][0], fract) * 2 * pi_v<float>);
			const float f1 = 1.f / (std::lerp(_f[vowel+0][1], _f[vowel+1][1], fract) * 2 * pi_v<float>);

			// samplerate coeff: depends on oversampling
			const float sr = m_type == FilterType::FastFormant ? m_sampleRatio : m_sampleRatio * 0.25f;

			m_vfa[0] = 1.0f - sr / ( f0 + sr );
			m_vfb[0] = 1.0f - m_vfa[0];
			m_vfc[0] = f0 /	( f0 + sr );
			m_vfa[1] = 1.0f - sr / ( f1 + sr );
			m_vfb[1] = 1.0f - m_vfa[1];
			m_vfc[1] = f1 /	( f1 + sr );
			return;
		}

		if( m_type == FilterType::Moog ||
			m_type == FilterType::DoubleMoog )
		{
			// [ 0 - 0.5 ]
			const float f = std::clamp(_freq, minFreq(), 20000.0f) * m_sampleRatio;
			// (Empirical tunning)
			m_p = ( 3.6f - 3.2f * f ) * f;
			m_k = 2.0f * m_p - 1;
			m_r = _q * std::exp((1 - m_p) * 1.386249f);

			if( m_doubleFilter )
			{
				m_subFilter->m_r = m_r;
				m_subFilter->m_p = m_p;
				m_subFilter->m_k = m_k;
			}
			return;
		}
		
		if( m_type == FilterType::Tripole )
		{
			const float f = std::clamp(_freq, 20.0f, 20000.0f) * m_sampleRatio * 0.25f;
			
			m_p = ( 3.6f - 3.2f * f ) * f;
			m_k = 2.0f * m_p - 1.0f;
			m_r = _q * 0.1f * std::exp((1 - m_p) * 1.386249f);
			
			return;
		}

		if( m_type == FilterType::Lowpass_SV || 
			m_type == FilterType::Bandpass_SV ||
			m_type == FilterType::Highpass_SV ||
			m_type == FilterType::Notch_SV )
		{
			const float f = std::sin(std::max(minFreq(), _freq) * m_sampleRatio * pi_v<float>);
			m_svf1 = std::min(f, 0.825f);
			m_svf2 = std::min(f * 2.0f, 0.825f);
			m_svq = std::max(0.0001f, 2.0f - (_q * 0.1995f));
			return;
		}

		// other filters
		_freq = std::clamp(_freq, minFreq(), 20000.0f);
		const float omega = 2 * pi_v<float> * _freq * m_sampleRatio;
		const float tsin = std::sin(omega) * 0.5f;
		const float tcos = std::cos(omega);

		const float alpha = tsin / _q;

		const float a0 = 1.0f / ( 1.0f + alpha );

		const float a1 = -2.0f * tcos * a0;
		const float a2 = ( 1.0f - alpha ) * a0;

		switch( m_type )
		{
			case FilterType::LowPass:
			{
				const float b1 = ( 1.0f - tcos ) * a0;
				const float b0 = b1 * 0.5f;
				m_biQuad.setCoeffs( a1, a2, b0, b1, b0 );
				break;
			}
			case FilterType::HiPass:
			{
				const float b1 = ( -1.0f - tcos ) * a0;
				const float b0 = b1 * -0.5f;
				m_biQuad.setCoeffs( a1, a2, b0, b1, b0 );
				break;
			}
			case FilterType::BandPass_CSG:
			{
				const float b0 = tsin * a0;
				m_biQuad.setCoeffs( a1, a2, b0, 0.0f, -b0 );
				break;
			}
			case FilterType::BandPass_CZPG:
			{
				const float b0 = alpha * a0;
				m_biQuad.setCoeffs( a1, a2, b0, 0.0f, -b0 );
				break;
			}
			case FilterType::Notch:
			{
				m_biQuad.setCoeffs( a1, a2, a0, a1, a0 );
				break;
			}
			case FilterType::AllPass:
			{
				m_biQuad.setCoeffs( a1, a2, a2, a1, 1.0f );
				break;
			}
			default:
				break;
		}

		if( m_doubleFilter )
		{
			m_subFilter->m_biQuad.setCoeffs( m_biQuad.m_a1, m_biQuad.m_a2, m_biQuad.m_b0, m_biQuad.m_b1, m_biQuad.m_b2 );
		}
	}


private:
	// biquad filter
	BiQuad<CHANNELS> m_biQuad;

	// coeffs for moog-filter
	float m_r, m_p, m_k;

	// coeffs for RC-type-filters
	float m_rca, m_rcb, m_rcc, m_rcq;

	// coeffs for formant-filters
	float m_vfa[4], m_vfb[4], m_vfc[4], m_vfq;

	// coeffs for Lowpass_SV (state-variant lowpass)
	float m_svf1, m_svf2, m_svq;

	using frame = std::array<sample_t, CHANNELS>;

	// in/out history for moog-filter
	frame m_y1, m_y2, m_y3, m_y4, m_oldx, m_oldy1, m_oldy2, m_oldy3;
	// additional one for Tripole filter
	frame m_last;

	// in/out history for RC-type-filters
	frame m_rcbp0, m_rclp0, m_rchp0, m_rclast0;
	frame m_rcbp1, m_rclp1, m_rchp1, m_rclast1;

	// in/out history for Formant-filters
	frame m_vfbp[6], m_vfhp[6], m_vflast[6];

	// in/out history for Lowpass_SV (state-variant lowpass)
	frame m_delay1, m_delay2, m_delay3, m_delay4;

	FilterType m_type;
	bool m_doubleFilter;

	float m_sampleRate;
	float m_sampleRatio;
	BasicFilters<CHANNELS> * m_subFilter;

} ;


} // namespace lmms

#endif // LMMS_BASIC_FILTERS_H
