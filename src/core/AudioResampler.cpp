/*
 * AudioResampler.cpp
 *
 * Copyright (c) 2025 Sotonye Atemie <sakertooth@gmail.com>
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

#include "AudioResampler.h"

#include <samplerate.h>
#include <stdexcept>
#include <string>
#include <utility>

namespace lmms {

AudioResampler::AudioResampler(InterpolationMode interpolationMode)
	: m_interpolationMode(interpolationMode)
	, m_state(src_new(static_cast<int>(interpolationMode), DEFAULT_CHANNELS, &m_error))
{
	if (!m_state)
	{
		const auto errorMessage = std::string{src_strerror(m_error)};
		const auto fullMessage = std::string{"Failed to create an AudioResampler: "} + errorMessage;
		throw std::runtime_error{fullMessage};
	}
}

AudioResampler::~AudioResampler()
{
	if (m_state) { src_delete(m_state); }
}

AudioResampler::AudioResampler(AudioResampler&& other) noexcept
	: m_state(std::exchange(other.m_state, nullptr))
	, m_data(std::exchange(other.m_data, SRC_DATA{}))
{
}

AudioResampler& AudioResampler::operator=(AudioResampler&& other) noexcept
{
	m_state = std::exchange(other.m_state, nullptr);
	m_data = std::exchange(other.m_data, SRC_DATA{});
	return *this;
}

bool AudioResampler::resample(SampleFrame* dst, size_t frames, double ratio, WriteCallback callback, void* callbackData)
{
	std::fill_n(dst, frames, SampleFrame{});
	if (!m_state) { return false; }

	m_data.data_out = &dst[0][0];
	m_data.output_frames = static_cast<long>(frames);
	m_data.src_ratio = ratio;

	while (m_data.output_frames > 0)
	{
		if (m_data.input_frames == 0)
		{
			const auto numInputFrames = callback(m_writeBuffer.data(), m_writeBuffer.size(), callbackData);
			m_data.data_in = &m_writeBuffer.data()[0][0];
			m_data.input_frames = numInputFrames;
			m_data.end_of_input = numInputFrames < m_writeBuffer.size();
		}

		if (src_process(m_state, &m_data)) { return false; }
		if (m_data.end_of_input && m_data.output_frames_gen == 0) { return false; }

		m_data.data_in += m_data.input_frames_used * DEFAULT_CHANNELS;
		m_data.input_frames -= m_data.input_frames_used;

		m_data.data_out += m_data.output_frames_gen * DEFAULT_CHANNELS;
		m_data.output_frames -= m_data.output_frames_gen;
	}

	return true;
}

} // namespace lmms
