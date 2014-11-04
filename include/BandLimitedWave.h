/*
 * BandLimitedWave.h - helper functions for band-limited
 *                    	waveform generation
 *
 * Copyright (c) 2014 Vesa Kivimäki <contact/dot/diizy/at/nbl/dot/fi>
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

#ifndef BANDLIMITEDWAVE_H
#define BANDLIMITEDWAVE_H

#include <QString>
#include <QDataStream>
#include <QFile>

#include "config_mgr.h"
#include "export.h"
#include "interpolation.h"
#include "lmms_basics.h"
#include "lmms_math.h"
#include "engine.h"
#include "Mixer.h"

#define MAXLEN 11
#define MIPMAPSIZE 2 << ( MAXLEN + 1 )
#define MIPMAPSIZE3 3 << ( MAXLEN + 1 )
#define MAXTBL 23
#define MINTLEN 2 << 0
#define MAXTLEN 3 << MAXLEN

// table for table sizes
const int TLENS[MAXTBL+1] = { 2 << 0, 3 << 0, 2 << 1, 3 << 1,
					2 << 2, 3 << 2, 2 << 3, 3 << 3,
					2 << 4, 3 << 4, 2 << 5, 3 << 5,
					2 << 6, 3 << 6, 2 << 7, 3 << 7,
					2 << 8, 3 << 8, 2 << 9, 3 << 9,
					2 << 10, 3 << 10, 2 << 11, 3 << 11 };

typedef struct
{
public:
	inline sample_t sampleAt( int table, int ph )
	{
		if( table % 2 == 0 )
		{	return m_data[ TLENS[ table ] + ph ]; }
		else
		{	return m_data3[ TLENS[ table ] + ph ]; }
	}
	inline void setSampleAt( int table, int ph, sample_t sample )
	{
		if( table % 2 == 0 )
		{	m_data[ TLENS[ table ] + ph ] = sample; }
		else
		{ 	m_data3[ TLENS[ table ] + ph ] = sample; }
	}
private:
	sample_t m_data [ MIPMAPSIZE ];
	sample_t m_data3 [ MIPMAPSIZE3 ];

} WaveMipMap;


QDataStream& operator<< ( QDataStream &out, WaveMipMap &waveMipMap );


QDataStream& operator>> ( QDataStream &in, WaveMipMap &waveMipMap );



class EXPORT BandLimitedWave
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
	static inline float freqToLen( float f )
	{
		return freqToLen( f, engine::mixer()->processingSampleRate() );
	}

	/*! \brief This method converts frequency to wavelength, but you can use any custom sample rate with it.
	 */
	static inline float freqToLen( float f, sample_rate_t sr )
	{
		return static_cast<float>( sr ) / f;
	}

	/*! \brief This method converts phase delta to wavelength. It assumes a phase scale of 0 to 1. */
	static inline float pdToLen( float pd )
	{
		return 1.0f / pd;
	}

	/*! \brief This method provides interpolated samples of bandlimited waveforms.
	 *  \param _ph The phase of the sample.
	 *  \param _wavelen The wavelength (length of one cycle, ie. the inverse of frequency) of the wanted oscillation, measured in sample frames
	 *  \param _wave The wanted waveform. Options currently are saw, triangle, square and moog saw.
	 */
	static inline sample_t oscillate( float _ph, float _wavelen, Waveforms _wave )
	{
		// high wavelen/ low freq
		if( _wavelen > TLENS[ MAXTBL ] )
		{
			const int t = MAXTBL;
			const int tlen = TLENS[t];
			const float ph = fraction( _ph );
			const float lookupf = ph * static_cast<float>( tlen );
			const int lookup = static_cast<int>( lookupf );
			const float ip = fraction( lookupf );

			const sample_t s1 = s_waveforms[ _wave ].sampleAt( t, lookup );
			const sample_t s2 = s_waveforms[ _wave ].sampleAt( t, ( lookup + 1 ) % tlen );
			const int lm = lookup == 0 ? tlen - 1 : lookup - 1;
			const sample_t s0 = s_waveforms[ _wave ].sampleAt( t, lm );
			const sample_t s3 = s_waveforms[ _wave ].sampleAt( t, ( lookup + 2 ) % tlen );
			const sample_t sr = optimal4pInterpolate( s0, s1, s2, s3, ip );

			return sr;
		}
		// low wavelen/ high freq
		if( _wavelen < 3.0f )
		{
			const int t = 0;
			const int tlen = TLENS[t];
			const float ph = fraction( _ph );
			const float lookupf = ph * static_cast<float>( tlen );
			const int lookup = static_cast<int>( lookupf );
			const float ip = fraction( lookupf );

			const sample_t s1 = s_waveforms[ _wave ].sampleAt( t, lookup );
			const sample_t s2 = s_waveforms[ _wave ].sampleAt( t, ( lookup + 1 ) % tlen );
			const int lm = lookup == 0 ? tlen - 1 : lookup - 1;
			const sample_t s0 = s_waveforms[ _wave ].sampleAt( t, lm );
			const sample_t s3 = s_waveforms[ _wave ].sampleAt( t, ( lookup + 2 ) % tlen );
			const sample_t sr = optimal4pInterpolate( s0, s1, s2, s3, ip );

			return sr;
		}

		// get the next higher tlen
		int t = MAXTBL - 1;
		while( _wavelen < TLENS[t] ) { t--; }

		int tlen = TLENS[t];
		const float ph = fraction( _ph );
		const float lookupf = ph * static_cast<float>( tlen );
		int lookup = static_cast<int>( lookupf );
		const float ip = fraction( lookupf );

		const sample_t s1 = s_waveforms[ _wave ].sampleAt( t, lookup );
		const sample_t s2 = s_waveforms[ _wave ].sampleAt( t, ( lookup + 1 ) % tlen );

		const int lm = lookup == 0 ? tlen - 1 : lookup - 1;
		const sample_t s0 = s_waveforms[ _wave ].sampleAt( t, lm );
		const sample_t s3 = s_waveforms[ _wave ].sampleAt( t, ( lookup + 2 ) % tlen );
		const sample_t sr = optimal4pInterpolate( s0, s1, s2, s3, ip );

		return sr;

/*		lookup = lookup << 1;
		tlen = tlen << 1;
		t += 1;
		const sample_t s3 = s_waveforms[ _wave ].sampleAt( t, lookup );
		const sample_t s4 = s_waveforms[ _wave ].sampleAt( t, ( lookup + 1 ) % tlen );
		const sample_t s34 = linearInterpolate( s3, s4, ip );

		const float ip2 = ( ( tlen - _wavelen ) / tlen - 0.5 ) * 2.0;

		return linearInterpolate( s12, s34, ip2 );
	*/
	};


	static void generateWaves();

	static bool s_wavesGenerated;

	static WaveMipMap s_waveforms [NumBLWaveforms];

	static QString s_wavetableDir;
};


#endif
