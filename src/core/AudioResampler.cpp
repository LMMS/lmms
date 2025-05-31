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
	: m_inputBuffer(BufferFrameSize * channels)
	, m_outputBuffer(BufferFrameSize * channels)
	, m_inputBufferWindow(m_inputBuffer.data(), 0)
	, m_state(src_new(mode, channels, &m_error))
	, m_channels(channels)
	, m_mode(mode)
{
	if (channels <= 0) { throw std::logic_error{"Invalid channel count"}; }
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

auto AudioResampler::process(std::span<float> dst, double ratio, InputCallback callback) -> long
{
	if (dst.size() % m_channels != 0)
	{
		throw std::logic_error{"Invalid channel count"};
	}

	auto data = SRC_DATA{.data_in = m_inputBufferWindow.data(),
		.data_out = dst.data(),
		.input_frames = static_cast<long>(m_inputBufferWindow.size()) / m_channels,
		.output_frames = static_cast<long>(dst.size()) / m_channels,
		.end_of_input = 0,
		.src_ratio = ratio};

	auto outputFramesGenerated = long{0};

	while (data.output_frames > 0)
	{
		if (data.input_frames == 0)
		{
			const auto inputFramesWritten = callback(m_inputBuffer.data(), BufferFrameSize, m_channels);
			data.data_in = m_inputBuffer.data();
			data.input_frames = static_cast<long>(inputFramesWritten);
		}

		if ((m_error = src_process(m_state, &data))) { throw std::runtime_error{src_strerror(m_error)}; }
		if (data.input_frames == 0 && data.output_frames_gen == 0) { break; }

		data.data_in += data.input_frames_used * m_channels;
		data.data_out += data.output_frames_gen * m_channels;

		data.input_frames -= data.input_frames_used;
		data.output_frames -= data.output_frames_gen;

		outputFramesGenerated += data.output_frames_gen;
	}

	m_inputBufferWindow = {data.data_in, static_cast<std::size_t>(data.input_frames * m_channels)};
	return outputFramesGenerated;
}

auto AudioResampler::process(std::span<const float> src, double ratio, OutputCallback callback) -> long
{
	if (src.size() % m_channels != 0)
	{
		throw std::logic_error{"Invalid channel count"};
	}

	auto data = SRC_DATA{.data_in = src.data(),
		.data_out = m_outputBuffer.data(),
		.input_frames = static_cast<long>(src.size()) / m_channels,
		.output_frames = static_cast<long>(m_outputBuffer.size()) / m_channels,
		.end_of_input = 0,
		.src_ratio = ratio};

	auto inputFramesUsed = long{0};

	while (data.input_frames > 0)
	{
		if ((m_error = src_process(m_state, &data))) { throw std::runtime_error{src_strerror(m_error)}; }

		data.data_in += data.input_frames_used * m_channels;
		data.input_frames -= data.input_frames_used;

		inputFramesUsed += data.input_frames_used;
		callback(data.data_out, data.output_frames_gen, m_channels);
	}

	return inputFramesUsed;
}

auto AudioResampler::process(std::span<const float> src, std::span<float> dst, double ratio) -> std::pair<long, long>
{
	if (src.size() % m_channels != 0 || dst.size() % m_channels != 0)
	{
		throw std::logic_error{"Invalid channel count"};
	}

	auto data = SRC_DATA{.data_in = src.data(),
		.data_out = dst.data(),
		.input_frames = static_cast<long>(src.size() / m_channels),
		.output_frames = static_cast<long>(dst.size() / m_channels),
		.end_of_input = 0,
		.src_ratio = ratio};

	auto inputFramesUsed = long{0};
	auto outputFramesGenerated = long{0};

	while (data.input_frames > 0 && data.output_frames > 0)
	{
		if ((m_error = src_process(m_state, &data))) { throw std::runtime_error{src_strerror(m_error)}; }

		data.data_in += data.input_frames_used * m_channels;
		data.input_frames -= data.input_frames_used;

		data.data_out += data.output_frames_gen * m_channels;
		data.output_frames -= data.output_frames_gen;

		inputFramesUsed += data.input_frames_used;
		outputFramesGenerated += data.output_frames_gen;
	}

	return {inputFramesUsed, outputFramesGenerated};
}

} // namespace lmms
