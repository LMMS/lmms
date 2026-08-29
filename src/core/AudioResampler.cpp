/*
 * AudioResampler.cpp
 *
 * Copyright (c) 2025 saker <sakertooth@gmail.com>
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

namespace lmms {

namespace {

constexpr auto converterType(AudioResampler::Mode mode) -> int
{
	switch (mode)
	{
	case AudioResampler::Mode::ZOH:
		return SRC_ZERO_ORDER_HOLD;
	case AudioResampler::Mode::Linear:
		return SRC_LINEAR;
	case AudioResampler::Mode::SincFastest:
		return SRC_SINC_FASTEST;
	case AudioResampler::Mode::SincMedium:
		return SRC_SINC_MEDIUM_QUALITY;
	case AudioResampler::Mode::SincBest:
		return SRC_SINC_BEST_QUALITY;
	default:
		throw std::invalid_argument{"Invalid interpolation mode"};
	}
}
} // namespace

AudioResampler::AudioResampler(Mode mode, ch_cnt_t channels)
	: m_state{src_new(converterType(mode), channels, &m_error)}
	, m_mode{mode}
	, m_channels{channels}
{
	if (channels <= 0) { throw std::logic_error{"Invalid channel count"}; }
	if (!m_state) { throw std::runtime_error{src_strerror(m_error)}; }
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

	if ((m_error = src_process(static_cast<SRC_STATE*>(m_state.get()), &data)))
	{
		throw std::runtime_error{src_strerror(m_error)};
	}

	return {static_cast<f_cnt_t>(data.input_frames_used), static_cast<f_cnt_t>(data.output_frames_gen)};
}

void AudioResampler::reset()
{
	if ((m_error = src_reset(static_cast<SRC_STATE*>(m_state.get()))))
	{
		throw std::runtime_error{src_strerror(m_error)};
	}
}

void AudioResampler::StateDeleter::operator()(void* state)
{
	src_delete(static_cast<SRC_STATE*>(state));
}

} // namespace lmms
