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

AudioResampler::AudioResampler(Mode mode, ch_cnt_t channels)
	: m_state(src_new(converterType(mode), channels, &m_error))
	, m_mode(mode)
	, m_channels(channels)
{
	if (channels <= 0) { throw std::logic_error{"Invalid channel count"}; }
	if (!m_state) { throw std::runtime_error{src_strerror(m_error)}; }
}

AudioResampler::~AudioResampler()
{
	src_delete(static_cast<SRC_STATE*>(m_state));
}

AudioResampler::AudioResampler(AudioResampler&& other) noexcept
	: m_state(std::exchange(other.m_state, nullptr))
	, m_mode(other.m_mode)
	, m_channels(other.m_channels)
	, m_ratio(other.m_ratio)
	, m_error(other.m_error)
{
}

AudioResampler& AudioResampler::operator=(AudioResampler&& other) noexcept
{
	m_state = std::exchange(other.m_state, nullptr);
	m_mode = other.m_mode;
	m_channels = other.m_channels;
	m_ratio = other.m_ratio;
	m_error = other.m_error;
	return *this;
}

auto AudioResampler::process(InterleavedBufferView<const float> input, InterleavedBufferView<float> output) -> Result
{
	if (input.channels() != m_channels || output.channels() != m_channels)
	{
		throw std::invalid_argument{"Invalid channel count"};
	}

	auto data = SRC_DATA{};

	data.data_in = input.data();
	data.input_frames = input.frames();

	data.data_out = output.data();
	data.output_frames = output.frames();

	data.src_ratio = m_ratio;
	data.end_of_input = 0;

	if ((m_error = src_process(static_cast<SRC_STATE*>(m_state), &data))) { throw std::runtime_error{src_strerror(m_error)}; }

	return {static_cast<f_cnt_t>(data.input_frames_used), static_cast<f_cnt_t>(data.output_frames_gen)};
}

void AudioResampler::reset()
{
	if ((m_error = src_reset(static_cast<SRC_STATE*>(m_state)))) { throw std::runtime_error{src_strerror(m_error)}; }
}

auto AudioResampler::converterType(Mode mode) -> int
{
	switch (mode)
	{
	case Mode::ZOH:
		return SRC_ZERO_ORDER_HOLD;
	case Mode::Linear:
		return SRC_LINEAR;
	case Mode::SincFastest:
		return SRC_SINC_FASTEST;
	case Mode::SincMedium:
		return SRC_SINC_MEDIUM_QUALITY;
	case Mode::SincBest:
		return SRC_SINC_BEST_QUALITY;
	default:
		throw std::invalid_argument{"Invalid interpolation mode"};
	}
}

} // namespace lmms
