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

#ifndef LMMS_BASICS_H
#define LMMS_BASICS_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>

#include "lmmsconfig.h"

namespace lmms
{

using bar_t = int32_t;
using tick_t = int32_t;
using volume_t = uint8_t;
using panning_t = int8_t;

using sample_t = float;		  // standard sample-type
using int_sample_t = int16_t; // 16-bit-int-sample

using sample_rate_t = uint32_t; // sample-rate
using fpp_t = int16_t;			// frames per period (0-16384)
using f_cnt_t = int32_t;		// standard frame-count
using ch_cnt_t = uint8_t;		// channel-count (0-SURROUND_CHANNELS)
using bpm_t = uint16_t;			// tempo (MIN_BPM to MAX_BPM)
using bitrate_t = uint16_t;		// bitrate in kbps
using mix_ch_t = uint16_t;		// Mixer-channel (0 to MAX_CHANNEL)

using jo_id_t = uint32_t; // (unique) ID of a journalling object

// windows headers define "min" and "max" macros, breaking the methods bwloe
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



constexpr ch_cnt_t DEFAULT_CHANNELS = 2;

constexpr ch_cnt_t SURROUND_CHANNELS =
#define LMMS_DISABLE_SURROUND
#ifndef LMMS_DISABLE_SURROUND
				4;
#else
				2;
#endif

constexpr char LADSPA_PATH_SEPERATOR =
#ifdef LMMS_BUILD_WIN32
';';
#else
':';
#endif



using         sampleFrame = std::array<sample_t,  DEFAULT_CHANNELS>;
using surroundSampleFrame = std::array<sample_t, SURROUND_CHANNELS>;
constexpr std::size_t LMMS_ALIGN_SIZE = 16;


#define LMMS_STRINGIFY(s) LMMS_STR(s)
#define LMMS_STR(PN)	#PN

// Abstract away GUI CTRL key (linux/windows) vs ⌘ (apple)
constexpr const char* UI_CTRL_KEY =
#ifdef LMMS_BUILD_APPLE
"⌘";
#else
"Ctrl";
#endif


//! Stand-in for C++20's std::span
template<typename T>
struct Span
{
	T* ptr;
	std::size_t size;

	constexpr auto operator[](std::size_t idx) const -> const T& { return ptr[idx]; }
	constexpr auto operator[](std::size_t idx) -> T& { return ptr[idx]; }

	constexpr auto begin() const -> const T* { return ptr; }
	constexpr auto begin() -> T* { return ptr; }
	constexpr auto end() const -> const T* { return ptr + size; }
	constexpr auto end() -> T* { return ptr + size; }
};


//! Conventions for passing audio data
enum class AudioDataLayout
{
	/*
	 * Given:
	 *   - N == Frame count
	 *   - C == Number of channels
	 *   - i == Sample index, where 0 <= i < N
	 *   - `samples` has the type sample_t*
	 *   - `samples` size == N * C
	 */

	/**
	 * Layout where the samples for each channel are interleaved.
	 * i.e. "LRLRLRLR"
	 *
	 * Or:
	 * - Channel #0 samples: samples[C*i]
	 * - Channel #1 samples: samples[C*i + 1]
	 * - Channel #2 samples: samples[C*i + 2]
	 * - Channel #3 samples: samples[C*i + 3]
	 */
	Interleaved,

	/**
	 * Layout where all samples for a particular channel are grouped together.
	 * i.e. "LLLLRRRR"
	 *
	 * Or:
	 * - Channel #0 samples: samples[i]
	 * - Channel #1 samples: samples[1*N + i]
	 * - Channel #2 samples: samples[2*N + i]
	 * - Channel #3 samples: samples[3*N + i]
	 */
	Split
};


/**
 * A simple (samples, size) pair for storing audio data of a particular layout.
 *
 * Does not contain channel grouping information like `ChannelGroups`, but all data is contiguous in memory.
 *
 * NOTE: More information is still needed to correctly interpret this audio data:
 * - For Split layout, the frame count is needed
 * - For Interleaved layout, the channel count is needed
 */
template<AudioDataLayout layout, typename T, std::enable_if_t<std::is_floating_point_v<T>, bool> = true>
using AudioData = Span<T>;

template<typename T>
using SplitAudioData = AudioData<AudioDataLayout::Split, T>;

template<typename T>
using InterleavedAudioData = AudioData<AudioDataLayout::Interleaved, T>;

using CoreAudioData = Span<const sampleFrame* const>;
using CoreAudioDataMut = Span<sampleFrame* const>;


// Stand-in for C++23's std::unreachable
// Taken from https://en.cppreference.com/w/cpp/utility/unreachable
[[noreturn]] inline void unreachable()
{
#if defined(_MSC_VER) && !defined(__clang__) // MSVC
	__assume(false);
#else // GCC, Clang
	__builtin_unreachable();
#endif
}


} // namespace lmms

#endif // LMMS_BASICS_H
