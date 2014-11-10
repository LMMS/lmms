/*
 * types.h - typedefs for common types that are used in the whole app
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


#ifndef _TYPES_H
#define _TYPES_H

#include <limits>

#include "lmmsconfig.h"

#ifdef LMMS_HAVE_STDINT_H
#include <stdint.h>
#endif


typedef int32_t tact_t;
typedef int32_t tick_t;
typedef uint8_t volume_t;
typedef int8_t panning_t;


typedef float sample_t;			// standard sample-type
typedef int16_t int_sample_t;		// 16-bit-int-sample


typedef uint32_t sample_rate_t;		// sample-rate
typedef int16_t fpp_t;			// frames per period (0-16384)
typedef int32_t f_cnt_t;			// standard frame-count
typedef uint8_t ch_cnt_t;			// channel-count (0-SURROUND_CHANNELS)
typedef uint16_t bpm_t;			// tempo (MIN_BPM to MAX_BPM)
typedef uint16_t bitrate_t;		// bitrate in kbps
typedef uint16_t fx_ch_t;			// FX-channel (0 to MAX_EFFECT_CHANNEL)

typedef uint32_t jo_id_t;			// (unique) ID of a journalling object

// use for improved branch prediction
#define likely(x)	__builtin_expect((x),1)
#define unlikely(x)	__builtin_expect((x),0)


template<typename T>
struct typeInfo
{
	static inline T min()
	{
		return std::numeric_limits<T>::min();
	}

	static inline T max()
	{
		return std::numeric_limits<T>::max();
	}

	static inline T minEps()
	{
		return 1;
	}

	static inline bool isEqual( T _x, T _y )
	{
		return _x == _y;
	}

	static inline T absVal( T t )
	{
		return t >= 0 ? t : -t;
	}
} ;


template<>
inline float typeInfo<float>::minEps()
{
	return 1.0e-10;
}
	
template<>
inline bool typeInfo<float>::isEqual( float _x, float _y )
{
	if( likely( _x == _y ) )
	{
		return true;
	}
	return absVal( _x - _y ) < minEps();
}



const ch_cnt_t DEFAULT_CHANNELS = 2;

const ch_cnt_t SURROUND_CHANNELS =
#define LMMS_DISABLE_SURROUND
#ifndef LMMS_DISABLE_SURROUND
				4;
#else
				2;
#endif


#ifdef LMMS_BUILD_WIN32
#define LADSPA_PATH_SEPERATOR ';'
#else
#define LADSPA_PATH_SEPERATOR ':'
#endif



typedef sample_t sampleFrame[DEFAULT_CHANNELS];
typedef sample_t surroundSampleFrame[SURROUND_CHANNELS];
#define ALIGN_SIZE 16
#if __GNUC__
typedef sample_t sampleFrameA[DEFAULT_CHANNELS] __attribute__((__aligned__(ALIGN_SIZE)));
#endif


#define STRINGIFY(s) STR(s)
#define STR(PN)	#PN


#endif
