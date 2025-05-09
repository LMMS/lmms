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
#include <utility>

namespace lmms {

AudioResampler::AudioResampler(int mode, int channels)
	: m_state(src_new(mode, channels, &m_error))
	, m_channels(channels)
	, m_mode(mode)
{
	if (!m_state) { throw std::runtime_error{src_strerror(m_error)}; }
}

AudioResampler::~AudioResampler()
{
	src_delete(m_state);
}

AudioResampler::AudioResampler(AudioResampler&& other) noexcept
	: m_state(std::exchange(other.m_state, nullptr))
{
}

AudioResampler& AudioResampler::operator=(AudioResampler&& other) noexcept
{
	m_state = std::exchange(other.m_state, nullptr);
	return *this;
}

AudioResampler::ResampleResult AudioResampler::resample(float* dst, std::size_t frames, double ratio)
{
	m_data.data_out = dst;
	m_data.output_frames = static_cast<long>(frames);

	m_data.src_ratio = ratio;
	m_data.end_of_input = 0;

	auto result = ResampleResult{};
	while (m_data.output_frames > 0)
	{
		if (m_data.input_frames == 0)
		{
			auto framesWritten = std::size_t{0};
			if (m_callback)
			{
				framesWritten = m_callback(m_buffer.data(), m_buffer.size() / m_channels, m_channels);
				m_data.data_in = m_buffer.data();
				m_data.input_frames = static_cast<long>(framesWritten);
			}

			if (framesWritten == 0)
			{
				// Silence unfilled output
				std::fill(dst + (frames - m_data.output_frames) * m_channels, dst + frames * m_channels, 0.f);
				return result;
			}
		}

		if ((m_error = src_process(m_state, &m_data))) { throw std::runtime_error{src_strerror(m_error)}; }

		m_data.data_in += m_data.input_frames_used * m_channels;
		m_data.data_out += m_data.output_frames_gen * m_channels;

		m_data.input_frames -= m_data.input_frames_used;
		m_data.output_frames -= m_data.output_frames_gen;

		result.inputFramesUsed += m_data.input_frames_used;
		result.outputFramesGenerated += m_data.output_frames_gen;
	}

	return result;
}

void AudioResampler::setSource(WriteCallback callback)
{
	m_callback = callback;
}

void AudioResampler::setSource(const float* src, std::size_t frames)
{
	m_data.data_in = src;
	m_data.input_frames = static_cast<long>(frames);
}

} // namespace lmms
