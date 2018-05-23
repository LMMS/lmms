/*
 * lmms_basics.h - typedefs for common types that are used in the whole app
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


#ifndef TYPES_H
#define TYPES_H

#include <limits>

#include "lmmsconfig.h"

#ifdef LMMS_HAVE_STDINT_H
#include <cstdint>
#endif

// for std::size_t.
#include <cstddef>

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
#define likely(x)	Q_LIKELY(x)
#define unlikely(x)	Q_UNLIKELY(x)

// windows headers define "min" and "max" macros, breaking the methods below
#undef min
#undef max

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

	static inline bool isEqual( T x, T y )
	{
		return x == y;
	}

	static inline T absVal( T t )
	{
		return t >= 0 ? t : -t;
	}
} ;


template<>
inline float typeInfo<float>::minEps()
{
	return 1.0e-10f;
}

template<>
inline bool typeInfo<float>::isEqual( float x, float y )
{
	if( x == y )
	{
		return true;
	}
	return absVal( x - y ) < minEps();
}



const ch_cnt_t DEFAULT_CHANNELS = 2;

const ch_cnt_t SURROUND_CHANNELS =
#define LMMS_DISABLE_SURROUND
#ifndef LMMS_DISABLE_SURROUND
				4;
#else
				2;
#endif

// "In audio 1 is always the left" - umcaruje, 2017
const ch_cnt_t RIGHT_CHANNEL_INDEX = 1;
const ch_cnt_t LEFT_CHANNEL_INDEX  = 0;

#ifdef LMMS_BUILD_WIN32
#define LADSPA_PATH_SEPERATOR ';'
#else
#define LADSPA_PATH_SEPERATOR ':'
#endif

/**
 *	@brief Non implementation dependent std::array.
 *
 *  std::array does not guaranty that its alignment
 *	would be that same as its elements. In other words,
 *  the compiler is allowed to add padding to the end
 *  of the class. That would be a problem if we want to
 *  use std::vector<sampleFrame>::data()->data()[X>CHANNEL].
 */
template<class T, size_t Size>
struct alignas (alignof(T)) ElementAlignedArray
{
	// implicit constructor and
	// operator =.

	T *data()
	{
		return m_data;
	}

	const T *data() const
	{
		return m_data;
	}

	T &operator[] (size_t index)
	{
		return m_data[index];
	}

	const T &operator[] (size_t index) const
	{
		return m_data[index];
	}

	T m_data[Size];
};

typedef ElementAlignedArray<sample_t, DEFAULT_CHANNELS> sampleFrame;
typedef ElementAlignedArray<sample_t, SURROUND_CHANNELS> surroundSampleFrame;

#define ALIGN_SIZE (16)

#define STRINGIFY(s) STR(s)
#define STR(PN)	#PN


#endif
