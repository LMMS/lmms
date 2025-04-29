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

AudioResampler::AudioResampler(int mode, int channels)
	: m_state(src_new(mode, channels, &m_error))
	, m_channels(channels)
	, m_mode(mode)
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

auto AudioResampler::resample(float* dst, long frames, double ratio, WriteCallback callback) -> int
{
	m_data.data_out = dst;
	m_data.output_frames = frames;

	m_data.src_ratio = ratio;
	m_data.end_of_input = 0;

	const auto maxInputFrames = static_cast<long>(m_buffer.size()) / m_channels;
	auto latestCallbackResult = WriteCallbackResult{};

	while (m_data.output_frames > 0)
	{
		if (m_data.input_frames == 0)
		{
			latestCallbackResult = callback(m_buffer.data(), maxInputFrames, m_channels);
			m_data.data_in = m_buffer.data();
			m_data.input_frames = latestCallbackResult.frames;
		}

		m_error = src_process(m_state, &m_data);
		if (m_error || (latestCallbackResult.done && m_data.output_frames_gen == 0)) { break; }

		m_data.data_in += m_data.input_frames_used * m_channels;
		m_data.data_out += m_data.output_frames_gen * m_channels;

		m_data.input_frames -= m_data.input_frames_used;
		m_data.output_frames -= m_data.output_frames_gen;
	}

	// Silence any unfilled output
	std::fill(dst + (frames - m_data.output_frames) * m_channels, dst + frames * m_channels, 0.f);
	return 0;
}

auto AudioResampler::resample(float* dst, long dstFrames, const float* src, long srcFrames, double ratio) -> int
{
	m_data.data_out = dst;
	m_data.output_frames = dstFrames;

	m_data.src_ratio = ratio;
	m_data.end_of_input = 0;

	while (m_data.output_frames > 0)
	{
		if (m_data.input_frames == 0)
		{
			// Dont try to read from src again, thats all the extra input we have
			if (m_data.data_in == src) { break; }

			m_data.data_in = src;
			m_data.input_frames = srcFrames;
		}

		m_error = src_process(m_state, &m_data);
		if (m_error) { break; }

		m_data.data_in += m_data.input_frames_used * m_channels;
		m_data.data_out += m_data.output_frames_gen * m_channels;

		m_data.input_frames -= m_data.input_frames_used;
		m_data.output_frames -= m_data.output_frames_gen;
	}

	// Enforce that the entire source buffer must be resampled into the destination buffer
	// This avoids having to worry about dropped source audio after this function completes.
	if (m_data.input_frames > 0)
	{
		throw std::logic_error{"Cannot resample entire source buffer into destination buffer"};
	}

	// Silence any unfilled output
	std::fill(dst + (dstFrames - m_data.output_frames) * m_channels, dst + dstFrames * m_channels, 0.f);
	return 0;
}

} // namespace lmms
