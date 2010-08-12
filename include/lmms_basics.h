/*
 * lmms_basics.h - common basics for the whole App
 *
 * Copyright (c) 2004-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifndef _LMMS_BASICS_H
#define _LMMS_BASICS_H

#include "lmmsconfig.h"

#ifdef LMMS_HAVE_STDINT_H
#include <stdint.h>
#endif

typedef unsigned char	Uint8;
typedef signed char	Sint8;
typedef unsigned short	Uint16;
typedef signed short	Sint16;
typedef unsigned int	Uint32;
typedef signed int	Sint32;


typedef Uint32 minute_t;
typedef Sint8 second_t;
typedef Sint32 tact_t;
typedef Sint32 tick_t;
typedef Uint8 volume_t;
typedef Sint8 panning_t;


typedef float sample_t;			// standard sample-type
typedef Sint16 int_sample_t;		// 16-bit-int-sample


typedef Uint32 sample_rate_t;		// sample-rate
typedef Sint16 fpp_t;			// frames per period (0-16384)
typedef Sint32 f_cnt_t;			// standard frame-count
typedef Uint8 ch_cnt_t;			// channel-count (0-SURROUND_CHANNELS)
typedef Uint16 bpm_t;			// tempo (MIN_BPM to MAX_BPM)
typedef Uint16 bitrate_t;		// bitrate in kbps
typedef Sint8 fx_ch_t;			// FX-channel (0 to MAX_EFFECT_CHANNEL)

typedef Uint32 jo_id_t;			// (unique) ID of a journalling object

// use for improved branch prediction
#define likely(x)	__builtin_expect((x),1)
#define unlikely(x)	__builtin_expect((x),0)

#ifdef __cplusplus

#include <limits>

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

#endif


#ifdef LMMS_BUILD_WIN32
#define LADSPA_PATH_SEPERATOR ';'
#else
#define LADSPA_PATH_SEPERATOR ':'
#endif


#define DEFAULT_CHANNELS 2
#define LMMS_DISABLE_SURROUND
#ifdef LMMS_DISABLE_SURROUND
#define SURROUND_CHANNELS 2
#else
#define SURROUND_CHANNELS 4
#endif



typedef sample_t sampleFrame[DEFAULT_CHANNELS];
typedef sample_t surroundSampleFrame[SURROUND_CHANNELS];

#define ALIGN_SIZE 16

#if __GNUC__

typedef sample_t sampleFrameA[DEFAULT_CHANNELS] __attribute__((__aligned__(ALIGN_SIZE)));
typedef int_sample_t intSampleFrameA[DEFAULT_CHANNELS] __attribute__((__aligned__(ALIGN_SIZE)));
#define RP __restrict__

#else

#define RP

#endif


#ifdef __cplusplus
const int BYTES_PER_SAMPLE = sizeof( sample_t );
const int BYTES_PER_INT_SAMPLE = sizeof( int_sample_t );
const int BYTES_PER_FRAME = sizeof( sampleFrame );
const int BYTES_PER_SURROUND_FRAME = sizeof( surroundSampleFrame );

const float OUTPUT_SAMPLE_MULTIPLIER = 32767.0f;
#else
#define BYTES_PER_SAMPLE sizeof( sample_t )
#define BYTES_PER_INT_SAMPLE sizeof( int_sample_t )
#define BYTES_PER_FRAME sizeof( sampleFrame )
#define BYTES_PER_SURROUND_FRAME sizeof( surroundSampleFrame )
#define OUTPUT_SAMPLE_MULTIPLIER 32767.0f
#endif


#define STRINGIFY(s) STR(s)
#define STR(PN)	#PN


#endif
