/*
 * BandLimitedWave.h - helper functions for band-limited
 *                    	waveform generation
 *
 * Copyright (c) 2014 Vesa Kivimäki <contact/dot/diizy/at/nbl/dot/fi>
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

#ifndef BANDLIMITEDWAVE_H
#define BANDLIMITEDWAVE_H

#include "interpolation.h"
#include "lmms_basics.h"
#include "lmms_math.h"
#include "engine.h"
#include "Mixer.h"

#define MAXLEN 12
#define MIPMAPSIZE 1 << ( MAXLEN + 1 )


typedef struct
{
public:
	inline sample_t sampleAt( int _table, int _ph )
	{
		return m_data[ ( 1 << _table ) + _ph ];
	}
	inline void setSampleAt( int _table, int _ph, sample_t _sample )
	{
		m_data[ ( 1 << _table ) + _ph ] = _sample;
	}
private:
	sample_t m_data [ MIPMAPSIZE ];
} WaveMipMap;


class BandLimitedWave
{
public:
	enum Waveforms
	{
		BLSaw,
		BLSquare,
		BLTriangle,
		BLMoog,
		NumBLWaveforms
	};

	BandLimitedWave() {};
	virtual ~BandLimitedWave() {};

	/*! \brief This method converts frequency to wavelength. The oscillate function takes wavelength as argument so
	 * use this to convert your note frequency to wavelength before using it.
	 */
	static inline float freqToLen( float _f )
	{
		return freqToLen( _f, engine::mixer()->processingSampleRate() );
	}

	/*! \brief This method converts frequency to wavelength, but you can use any custom sample rate with it.
	 */
	static inline float freqToLen( float _f, sample_rate_t _sr )
	{
		return static_cast<float>( _sr ) / _f;
	}

	/*! \brief This method provides interpolated samples of bandlimited waveforms.
	 *  \param _ph The phase of the sample.
	 *  \param _wavelen The wavelength (length of one cycle, ie. the inverse of frequency) of the wanted oscillation, measured in sample frames
	 *  \param _wave The wanted waveform. Options currently are saw, triangle, square and moog saw.
	 */
	static inline sample_t oscillate( float _ph, float _wavelen, Waveforms _wave )
	{
		// high wavelen/ low freq
		if( _wavelen >= 1 << MAXLEN )
		{
			const int t = MAXLEN;
			const int tlen = 1 << t;
			const float ph = fraction( _ph );
			const float lookupf = ph * static_cast<float>( tlen );
			const int lookup = static_cast<int>( lookupf );
			const sample_t s1 = s_waveforms[ _wave ].sampleAt( t, lookup );
			const sample_t s2 = s_waveforms[ _wave ].sampleAt( t, ( lookup + 1 ) % tlen );
			return linearInterpolate( s1, s2, fraction( lookupf ) );
		}
		// low wavelen/ high freq
		if( _wavelen <= 2.0f )
		{
			const int t = 1;
			const int tlen = 2;
			const float ph = fraction( _ph );
			const float lookupf = ph * static_cast<float>( tlen );
			const int lookup = static_cast<int>( lookupf );
			const sample_t s1 = s_waveforms[ _wave ].sampleAt( t, lookup );
			const sample_t s2 = s_waveforms[ _wave ].sampleAt( t, ( lookup + 1 ) % tlen );
			return linearInterpolate( s1, s2, fraction( lookupf ) );
		}
		
		// get the next higher tlen
		int t = 2;
		while( ( 1 << t ) < _wavelen ) { t++; }
		
		const int tlen = 1 << t;
		const float ph = fraction( _ph );
		const float lookupf = ph * static_cast<float>( tlen );
		const int lookup = static_cast<int>( lookupf );
		const sample_t s1 = s_waveforms[ _wave ].sampleAt( t, lookup );
		const sample_t s2 = s_waveforms[ _wave ].sampleAt( t, ( lookup + 1 ) % tlen );
		return linearInterpolate( s1, s2, fraction( lookupf ) );
		
		
		/*const int tlen1 = 1 << t;
		const int tlen2 = 1 << ( t - 1 );
		
		const float ph = fraction( _ph );
		const float lookupf = ph * static_cast<float>( tlen1 );
		const int lookup1 = static_cast<int>( lookupf );
		const int lookup2 = static_cast<int>( ph * static_cast<float>( tlen2 ) );

		const sample_t s1 = linearInterpolate( s_waveforms[ _wave ].sampleAt( t, lookup1 ),
												s_waveforms[ _wave ].sampleAt( t, ( lookup1 + 1 ) % tlen1 ),
												fraction( lookupf ) );
		const sample_t s2 = s_waveforms[ _wave ].sampleAt( t - 1, lookup2 );

		const float ip = static_cast<float>( tlen1 - _wavelen ) / static_cast<float>( tlen2 );
		
		return linearInterpolate( s1, s2, ip );*/
	};

	/*! \brief The same as oscillate but uses cosinus interpolation instead of linear.
	 */
	static inline sample_t oscillateCos( float _ph, float _wavelen, Waveforms _wave )
	{
		int t = MAXLEN;
		while( ( 1 << t ) > _wavelen ) { t--; }
		t = qMax( 1, t );

		const int tlen = 1 << t;
		const float ph = fraction( _ph );
		const int lookup = static_cast<int>( ph * tlen );
		const sample_t s1 = s_waveforms[ _wave ].sampleAt( t, lookup );
		const sample_t s2 = s_waveforms[ _wave ].sampleAt( t, ( lookup + 1 ) % tlen );

		return cosinusInterpolate( s1, s2, ph );
	};

	/*! \brief The same as oscillate but without any interpolation.
	 */
	static inline sample_t oscillateNoip( float _ph, float _wavelen, Waveforms _wave )
	{
		int t = MAXLEN;
		while( ( 1 << t ) > _wavelen ) { t--; }
		t = qMax( 1, t );

		const int tlen = 1 << t;
		const float ph = fraction( _ph );
		const int lookup = static_cast<int>( ph * tlen );
		return s_waveforms[ _wave ].sampleAt( t, lookup );
	};


	static void generateWaves();

	static WaveMipMap s_waveforms [NumBLWaveforms];
};


#endif
