/*
 * BandLimitedWave.h - helper functions for band-limited
 *                    	waveform generation
 *
 * Copyright (c) 2014 Vesa Kivimäki <contact/dot/diizy/at/nbl/dot/fi>
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

#ifndef LMMS_BANDLIMITEDWAVE_H
#define LMMS_BANDLIMITEDWAVE_H

class QDataStream;
class QString;

#include "lmms_export.h"
#include "interpolation.h"
#include "LmmsTypes.h"
#include "lmms_math.h"
#include "Engine.h"
#include "AudioEngine.h"

namespace lmms
{

constexpr int MAXLEN = 11;
constexpr int MIPMAPSIZE = 2 << ( MAXLEN + 1 );
constexpr int MIPMAPSIZE3 = 3 << ( MAXLEN + 1 );
constexpr int MAXTBL = 23;
constexpr int MINTLEN = 2 << 0;
constexpr int MAXTLEN = 3 << MAXLEN;

// table for table sizes
const int TLENS[MAXTBL+1] = { 2 << 0, 3 << 0, 2 << 1, 3 << 1,
					2 << 2, 3 << 2, 2 << 3, 3 << 3,
					2 << 4, 3 << 4, 2 << 5, 3 << 5,
					2 << 6, 3 << 6, 2 << 7, 3 << 7,
					2 << 8, 3 << 8, 2 << 9, 3 << 9,
					2 << 10, 3 << 10, 2 << 11, 3 << 11 };

struct WaveMipMap
{
public:
	inline sample_t sampleAt(int table, int ph)
	{
		if (table % 2 == 0) { return m_data[TLENS[table] + ph]; }
		else
		{
			return m_data3[TLENS[table] + ph];
		}
	}
	inline void setSampleAt(int table, int ph, sample_t sample)
	{
		if (table % 2 == 0) { m_data[TLENS[table] + ph] = sample; }
		else
		{
			m_data3[TLENS[table] + ph] = sample;
		}
	}

private:
	sample_t m_data[MIPMAPSIZE];
	sample_t m_data3[MIPMAPSIZE3];
};

QDataStream& operator<< ( QDataStream &out, WaveMipMap &waveMipMap );


QDataStream& operator>> ( QDataStream &in, WaveMipMap &waveMipMap );



class LMMS_EXPORT BandLimitedWave
{
public:
	enum class Waveform
	{
		BLSaw,
		BLSquare,
		BLTriangle,
		BLMoog,
		Count
	};
	constexpr static auto NumWaveforms = static_cast<std::size_t>(Waveform::Count);

	BandLimitedWave() = default;
	virtual ~BandLimitedWave() = default;

	/*! \brief This method converts frequency to wavelength. The oscillate function takes wavelength as argument so
	 * use this to convert your note frequency to wavelength before using it.
	 */
	static inline float freqToLen( float f )
	{
		return freqToLen( f, Engine::audioEngine()->outputSampleRate() );
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
	static inline sample_t oscillate( float _ph, float _wavelen, Waveform _wave )
	{
		// get the next higher tlen
		int t = 0;
		while( t < MAXTBL && _wavelen >= TLENS[t+1] ) { t++; }

		int tlen = TLENS[t];
		const float ph = fraction( _ph );
		const float lookupf = ph * static_cast<float>( tlen );
		int lookup = static_cast<int>( lookupf );
		const float ip = fraction( lookupf );

		const sample_t s1 = s_waveforms[ static_cast<std::size_t>(_wave) ].sampleAt( t, lookup );
		const sample_t s2 = s_waveforms[ static_cast<std::size_t>(_wave) ].sampleAt( t, ( lookup + 1 ) % tlen );

		const int lm = lookup == 0 ? tlen - 1 : lookup - 1;
		const sample_t s0 = s_waveforms[ static_cast<std::size_t>(_wave) ].sampleAt( t, lm );
		const sample_t s3 = s_waveforms[ static_cast<std::size_t>(_wave) ].sampleAt( t, ( lookup + 2 ) % tlen );
		const sample_t sr = optimal4pInterpolate( s0, s1, s2, s3, ip );

		return sr;

		/*
		lookup = lookup << 1;
		tlen = tlen << 1;
		t += 1;
		const sample_t s3 = s_waveforms[ static_cast<std::size_t>(_wave) ].sampleAt( t, lookup );
		const sample_t s4 = s_waveforms[ static_cast<std::size_t>(_wave) ].sampleAt( t, ( lookup + 1 ) % tlen );
		const sample_t s34 = std::lerp(s3, s4, ip);

		const float ip2 = ( ( tlen - _wavelen ) / tlen - 0.5 ) * 2.0;

		return std::lerp(s12, s34, ip2);
		*/
	};


	static void generateWaves();

	static bool s_wavesGenerated;

	static std::array<WaveMipMap, NumWaveforms> s_waveforms;

	static QString s_wavetableDir;
};

} // namespace lmms

#endif // LMMS_BANDLIMITEDWAVE_H
