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

#include <cstddef>
#include <cstdint>
#include <type_traits>

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
using fpp_t = size_t;			// frames per period (0-16384)
using f_cnt_t = size_t;			// standard frame-count
using ch_cnt_t = uint8_t;		// channel-count (0-DEFAULT_CHANNELS)
using bpm_t = uint16_t;			// tempo (MIN_BPM to MAX_BPM)
using bitrate_t = uint16_t;		// bitrate in kbps
using mix_ch_t = uint16_t;		// Mixer-channel (0 to MAX_CHANNEL)
using pi_ch_t = uint16_t;		// plugin channel

using jo_id_t = uint32_t; // (unique) ID of a journalling object


constexpr ch_cnt_t DEFAULT_CHANNELS = 2;

constexpr char LADSPA_PATH_SEPERATOR =
#ifdef LMMS_BUILD_WIN32
';';
#else
':';
#endif


#define LMMS_STRINGIFY(s) LMMS_STR(s)
#define LMMS_STR(PN)	#PN

// Abstract away GUI CTRL key (linux/windows) vs ⌘ (apple)
constexpr const char* UI_CTRL_KEY =
#ifdef LMMS_BUILD_APPLE
"⌘";
#else
"Ctrl";
#endif


/**
 * Simple minimally functional stand-in for C++20's std::span
 *
 * TODO C++20: Use std::span instead
 */
template<typename T, std::size_t extents = static_cast<std::size_t>(-1)>
class Span
{
public:
	using element_type = T;
	using pointer = T*;

	constexpr Span() = default;
	constexpr Span(const Span&) = default;

	//! Constructor from mutable to const
	template<typename U = T, std::enable_if_t<std::is_const_v<U>, bool> = true>
	constexpr Span(const Span<std::remove_const_t<U>, extents>& other)
		: m_data{other.data()}
		, m_size{other.size()}
	{
	}

	constexpr Span(T* data, std::size_t size)
		: m_data{data}
		, m_size{size}
	{
	}

	//! Constructor from mutable to const
	template<typename U = T, std::enable_if_t<std::is_const_v<U>, bool> = true>
	constexpr Span(std::remove_const_t<U>* data, std::size_t size)
		: m_data{data}
		, m_size{size}
	{
	}

	~Span() = default;

	constexpr auto data() const -> T* { return m_data; }
	constexpr auto size() const -> std::size_t
	{
		if constexpr (extents == static_cast<std::size_t>(-1)) { return m_size; }
		else { return extents; }
	}
	constexpr auto size_bytes() const -> std::size_t { return size() * sizeof(T); } // NOLINT

	constexpr auto operator[](std::size_t idx) const -> const T& { return m_data[idx]; }
	constexpr auto operator[](std::size_t idx) -> T& { return m_data[idx]; }

	constexpr auto begin() const -> const T* { return m_data; }
	constexpr auto begin() -> T* { return m_data; }
	constexpr auto end() const -> const T* { return m_data + size(); }
	constexpr auto end() -> T* { return m_data + size(); }

private:
	T* m_data = nullptr;
	std::size_t m_size = 0;
};


/**
 * Stand-in for C++23's std::unreachable
 * Taken from https://en.cppreference.com/w/cpp/utility/unreachable
 *
 * TODO C++23: Use std::unreachable instead
 */
[[noreturn]] inline void unreachable()
{
#if defined(_MSC_VER) && !defined(__clang__) // MSVC
	__assume(false);
#else // GCC, Clang
	__builtin_unreachable();
#endif
}


/**
 * Can be used with static_assert() in an uninstantiated template
 * as a workaround for static_assert(false)
 *
 * TODO C++23: No longer needed with resolution of CWG2518
 */
template<class... T>
inline constexpr bool always_false_v = false;


} // namespace lmms

#endif // LMMS_BASICS_H
