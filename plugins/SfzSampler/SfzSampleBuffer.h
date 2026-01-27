/*
 * SfzSampleBuffer.h - Custom sample data class
 *
 * Copyright (c) 2026 Keratin
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

#ifndef LMMS_SFZ_SAMPLE_BUFFER_H
#define LMMS_SFZ_SAMPLE_BUFFER_H

#include "SampleFrame.h"
#include <memory>

namespace lmms
{

class SampleFrame;

class SfzSampleBuffer
{
public:
	SfzSampleBuffer() = default;
	SfzSampleBuffer(const SampleFrame* data, const f_cnt_t size, const float sampleRate);

	//! Returns a hermite-interpolated value for the data at the given sample index and channel.
	//! The interpolation is so that pitch shifting and resampling is as easy as possible
	//! If index is out of range, this function returns 0.0f
	float at(const float index, const size_t channel) const;

	//! Returns the number of frames in the sample.
	f_cnt_t size() const { return m_size; }

	//! Returns the sample rate of the audio
	float sampleRate() const { return m_sampleRate; }

private:
	static constexpr const size_t NUM_CHANNELS = 2;
	//! The raw sample data is stored as a shared pointer to a 2d array of floats.
	//! This was originally done so that it would not be copied unnecessarily if, for example, the vector or map containing the SfzSampleBuffer were resized/reallocated
	//! TODO: is this optimal?
	std::shared_ptr<float[][NUM_CHANNELS]> m_data;

	f_cnt_t m_size;
	float m_sampleRate;
};

} // namespace lmms

#endif // LMMS_SFZ_SAMPLE_BUFFER_H
