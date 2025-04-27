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
	: m_state(src_new(static_cast<int>(interpolationMode), DEFAULT_CHANNELS, &m_error))
	, m_interpolationMode(interpolationMode)
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

auto AudioResampler::resample(double ratio) -> int
{
	m_data.data_in = &m_input.buffer.data()[0][0] + m_input.readerIndex * DEFAULT_CHANNELS;
	m_data.data_out = &m_output.buffer.data()[0][0] + m_output.writerIndex * DEFAULT_CHANNELS;

	m_data.input_frames = m_input.writerIndex - m_input.readerIndex;
	m_data.output_frames = static_cast<long>(m_output.buffer.size()) - m_output.writerIndex;

	m_data.src_ratio = ratio;
	m_data.end_of_input = 0;

	const auto error = src_process(m_state, &m_data);
	if (error != 0) { return error; }

	m_input.readerIndex += m_data.input_frames_used;
	m_output.writerIndex += m_data.output_frames_gen;

	if (m_input.readerIndex == static_cast<long>(m_input.buffer.size()))
	{
		m_input.readerIndex = 0;
		m_input.writerIndex = 0;
	}

	return 0;
}

auto AudioResampler::inputWriterView() -> std::span<SampleFrame>
{
	const auto begin = m_input.buffer.begin() + m_input.writerIndex;
	const auto end = m_input.buffer.end();
	return begin == end ? std::span<SampleFrame>{} : std::span{begin, end};
}

auto AudioResampler::outputReaderView() const -> std::span<const SampleFrame>
{
	const auto begin = m_output.buffer.begin() + m_output.readerIndex;
	const auto end = m_output.buffer.begin() + m_output.writerIndex;
	return begin == end ? std::span<SampleFrame>{} : std::span{begin, end};
}

void AudioResampler::commitInputWrite(std::size_t frames)
{
	assert(m_input.writerIndex + frames <= m_input.buffer.size());
	m_input.writerIndex += static_cast<long>(frames);
}

void AudioResampler::commitOutputRead(std::size_t frames)
{
	assert(m_output.readerIndex + frames <= m_output.writerIndex);
	m_output.readerIndex += static_cast<long>(frames);

	if (m_output.readerIndex == static_cast<long>(m_output.buffer.size()))
	{
		m_output.readerIndex = 0;
		m_output.writerIndex = 0;
	}
}

auto AudioResampler::interpolationModeName(InterpolationMode mode) -> const char*
{
	return src_get_name(static_cast<int>(mode));
}

auto AudioResampler::errorDescription(int error) -> const char*
{
	return src_strerror(error);
}

} // namespace lmms
