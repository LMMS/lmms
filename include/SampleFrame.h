/*
 * SampleFrame.h - Representation of a stereo sample
 *
 * Copyright (c) 2004-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * Copyright (c) 2024- Michael Gregorius
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

#ifndef LMMS_SAMPLEFRAME_H
#define LMMS_SAMPLEFRAME_H

#include "AudioData.h"
#include "lmms_basics.h"

#include <algorithm>
#include <array>
#include <cmath>


namespace lmms
{

class SampleFrame
{
public:
	SampleFrame() : SampleFrame(0., 0.)
	{
	}

	explicit SampleFrame(sample_t value) : SampleFrame(value, value)
	{
	}

	SampleFrame(sample_t left, sample_t right) :
		m_samples({ left, right })
	{
	}

	InterleavedAudioDataPtr<sample_t> data()
	{
		return m_samples.data();
	}

	InterleavedAudioDataPtr<const sample_t> data() const
	{
		return m_samples.data();
	}

	sample_t& left()
	{
		return m_samples[0];
	}

	const sample_t& left() const
	{
		return m_samples[0];
	}

	void setLeft(const sample_t& value)
	{
		m_samples[0] = value;
	}

	sample_t& right()
	{
		return m_samples[1];
	}

	const sample_t& right() const
	{
		return m_samples[1];
	}

	void setRight(const sample_t& value)
	{
		m_samples[1] = value;
	}

	sample_t& operator[](size_t index)
	{
		return m_samples[index];
	}

	const sample_t& operator[](size_t index) const
	{
		return m_samples[index];
	}

	SampleFrame operator+(const SampleFrame& other) const
	{
		return SampleFrame(left() + other.left(), right() + other.right());
	}

	SampleFrame& operator+=(const SampleFrame& other)
	{
		auto & l = left();
		auto & r = right();

		l += other.left();
		r += other.right();

		return *this;
	}

	SampleFrame operator*(float value) const
	{
		return SampleFrame(left() * value, right() * value);
	}

	SampleFrame& operator*=(float value)
	{
		setLeft(left() * value);
		setRight(right() * value);

		return *this;
	}

	SampleFrame operator*(const SampleFrame& other) const
	{
		return SampleFrame(left() * other.left(), right() * other.right());
	}

	void operator*=(const SampleFrame& other)
	{
		left() *= other.left();
		right() *= other.right();
	}

	sample_t sumOfSquaredAmplitudes() const
	{
		return left() * left() + right() * right();
	}

	SampleFrame abs() const
	{
		return SampleFrame{std::abs(this->left()), std::abs(this->right())};
	}

	SampleFrame absMax(const SampleFrame& other)
	{
		const auto a = abs();
		const auto b = other.abs();

		return SampleFrame(std::max(a.left(), b.left()), std::max(a.right(), b.right()));
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

inline void zeroSampleFrames(SampleFrame* buffer, size_t frames)
{
	// The equivalent of the following operation which yields compiler warnings
	// memset(buffer, 0, sizeof(SampleFrame) * frames);

	std::fill(buffer, buffer + frames, SampleFrame());
}

inline SampleFrame getAbsPeakValues(SampleFrame* buffer, size_t frames)
{
	SampleFrame peaks;

	for (f_cnt_t i = 0; i < frames; ++i)
	{
		peaks = peaks.absMax(buffer[i]);
	}

	return peaks;
}

inline void copyToSampleFrames(SampleFrame* target, InterleavedAudioDataPtr<const float> source, size_t frames)
{
	for (size_t i = 0; i < frames; ++i)
	{
		target[i].setLeft(source[2*i]);
		target[i].setRight(source[2*i + 1]);
	}
}

inline void copyFromSampleFrames(InterleavedAudioDataPtr<float> target, const SampleFrame* source, size_t frames)
{
	for (size_t i = 0; i < frames; ++i)
	{
		target[2*i] = source[i].left();
		target[2*i + 1] = source[i].right();
	}
}

using CoreAudioData = Span<const SampleFrame* const>;
using CoreAudioDataMut = Span<SampleFrame* const>;

} // namespace lmms

#endif // LMMS_SAMPLEFRAME_H
