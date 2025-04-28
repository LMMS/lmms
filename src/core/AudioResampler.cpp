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
	, m_data(SRC_DATA{.data_in = m_buffer.data(), .input_frames = 0})
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
	if (m_state) { src_delete(m_state); }
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
	if (!m_state) { return false; }

	m_data.data_out = dst;
	m_data.output_frames = frames;

	m_data.src_ratio = ratio;
	m_data.end_of_input = 0;

	std::fill_n(dst, frames * m_channels, 0.f);
	const auto maxInputFrames = static_cast<long>(m_buffer.size()) / m_channels;

	while (m_data.output_frames > 0)
	{
		if (m_data.input_frames == 0)
		{
			const auto framesWritten = callback(m_buffer.data(), maxInputFrames, m_channels);
			m_data.data_in = m_buffer.data();
			m_data.input_frames = framesWritten;
			m_data.end_of_input = framesWritten < maxInputFrames;
		}

		if (src_process(m_state, &m_data)) { return false; }
		if (m_data.end_of_input && m_data.output_frames_gen == 0) { return true; }

		m_data.data_in += m_data.input_frames_used * m_channels;
		m_data.data_out += m_data.output_frames_gen * m_channels;

		m_data.input_frames -= m_data.input_frames_used;
		m_data.output_frames -= m_data.output_frames_gen;
	}

	return true;
}

auto AudioResampler::resample(float* dst, long dstFrames, const float* src, long srcFrames, double ratio) -> int
{
	auto data = SRC_DATA{
		.data_in = src,
		.data_out = dst,
		.input_frames = srcFrames,
		.output_frames = dstFrames,
		.end_of_input = 0,
		.src_ratio = ratio
	};

	std::fill_n(dst, dstFrames * m_channels, 0.f);
	while (data.output_frames > 0)
	{
		if (src_process(m_state, &data)) { return false; }

		data.data_in += data.input_frames_used * m_channels;
		data.data_out += data.output_frames_gen * m_channels;

		data.input_frames -= data.input_frames_used;
		data.output_frames -= data.output_frames_gen;
	}

	return data.input_frames == 0;
}

} // namespace lmms
