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

#ifndef LMMS_TYPES_H
#define LMMS_TYPES_H

#include <cstddef>
#include <limits>

#include "lmmsconfig.h"

#include <cstdint>
#include <array>

#include <cmath>
#include <algorithm>


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
using ch_cnt_t = uint8_t;		// channel-count (0-DEFAULT_CHANNELS)
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

constexpr char LADSPA_PATH_SEPERATOR =
#ifdef LMMS_BUILD_WIN32
';';
#else
':';
#endif


class sampleFrame
{
public:
	sampleFrame() : sampleFrame(0., 0.)
	{
	}

	sampleFrame(sample_t value) : sampleFrame(value, value)
	{
	}

	sampleFrame(sample_t left, sample_t right) :
		m_samples({ left, right })
	{
	}

	sample_t * data()
	{
		return m_samples.data();
	}

	sample_t const * data() const
	{
		return m_samples.data();
	}

	sample_t & left()
	{
		return m_samples[0];
	}

	sample_t const & left() const
	{
		return m_samples[0];
	}

	void setLeft(sample_t const & value)
	{
		m_samples[0] = value;
	}

	sample_t & right()
	{
		return m_samples[1];
	}

	sample_t const & right() const
	{
		return m_samples[1];
	}

	void setRight(sample_t const & value)
	{
		m_samples[1] = value;
	}

	sample_t & operator[](size_t index)
	{
		return m_samples[index];
	}

	sample_t const & operator[](size_t index) const
	{
		return m_samples[index];
	}

	sampleFrame & operator=(float v)
	{
		left() = v;
		right() = v;

		return *this;
	}

	sampleFrame operator+(sampleFrame const & other) const
	{
		return sampleFrame(left() + other.left(), right() + other.right());
	}

	void operator+=(sampleFrame const & other)
	{
		auto & l = left();
		auto & r = right();

		l += other.left();
		r += other.right();
	}

	sampleFrame operator*(float value) const
	{
		return sampleFrame(left() * value, right() * value);
	}

	void operator*=(float value)
	{
		setLeft(left() * value);
		setRight(right() * value);
	}

	sampleFrame operator*(sampleFrame const & other) const
	{
		return sampleFrame(left() * other.left(), right() * other.right());
	}

	void operator*=(sampleFrame const & other)
	{
		left() *= other.left();
		right() *= other.right();
	}

	sample_t scalarProduct(sampleFrame const & other) const
	{
		return left() * other.left() + right() * other.right();
	}

	sampleFrame abs() const
	{
		return sampleFrame{std::abs(this->left()), std::abs(this->right())};
	}

	void max(sampleFrame const & other)
	{
		if (other.left() > left())
		{
			setLeft(other.left());
		}

		if (other.right() > right())
		{
			setRight(other.right());
		}
	}

	sample_t average() const
	{
		return (left() + right()) / 2;
	}

	void clamp(sample_t low, sample_t high)
	{
		auto & l = left();
		l = std::clamp(l, low, high);

		auto & r = right();
		r = std::clamp(r, low, high);
	}

	bool containsInf() const
	{
		return std::isinf(left()) || std::isinf(right());
	}

	bool containsNaN() const
	{
		return std::isnan(left()) || std::isnan(right());
	}

private:
	std::array<sample_t, DEFAULT_CHANNELS> m_samples;
};

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


} // namespace lmms

#endif // LMMS_TYPES_H
