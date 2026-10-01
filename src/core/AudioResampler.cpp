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

AudioResampler::AudioResampler(Mode mode, ch_cnt_t channels, bool interleaved)
	: m_mode{mode}
	, m_channels{channels}
	, m_interleaved{interleaved}
{
	if (channels <= 0) { throw std::logic_error{"Invalid channel count"}; }

	if (interleaved)
	{
		// one single State with `channels` channels
		auto state = State{src_new(converterType(mode), channels, &m_error)};
		if (!state) { throw std::runtime_error{src_strerror(m_error)}; }
		m_states.push_back(std::move(state));
	}
	else
	{
		if (channels > MaxChannelsPerAudioBuffer)
		{
			throw std::invalid_argument{"Too many planar channels"};
		}

		// `channels` States with 1 channel each
		m_states.reserve(channels);
		for (ch_cnt_t ch = 0; ch < channels; ++ch)
		{
			auto state = State{src_new(converterType(mode), 1, &m_error)};
			if (!state) { throw std::runtime_error{src_strerror(m_error)}; }
			m_states.push_back(std::move(state));
		}
	}
}

auto AudioResampler::process(InterleavedBufferSpan<const float> input, InterleavedBufferSpan<float> output) -> Result
{
	if (input.channels() != m_channels || output.channels() != m_channels)
	{
		throw std::invalid_argument{"Invalid channel count"};
	}

	if (!interleaved())
	{
		throw std::invalid_argument{"Resampler was not configured to process interleaved buffers"};
	}

	auto data = SRC_DATA{};

	data.data_in = input.data();
	data.input_frames = input.frames();

	data.data_out = output.data();
	data.output_frames = output.frames();

	data.src_ratio = m_ratio;
	data.end_of_input = 0;

	if ((m_error = src_process(static_cast<SRC_STATE*>(m_states[0].get()), &data)))
	{
		throw std::runtime_error{src_strerror(m_error)};
	}

	return {static_cast<f_cnt_t>(data.input_frames_used), static_cast<f_cnt_t>(data.output_frames_gen)};
}

auto AudioResampler::process(PlanarBufferSpan<const float> input, PlanarBufferSpan<float> output) -> Result
{
	if (input.channels() != m_channels || output.channels() < m_channels)
	{
		throw std::invalid_argument{"Invalid channel count"};
	}

	if (interleaved())
	{
		throw std::invalid_argument{"Resampler was not configured to process planar buffers"};
	}

	long inputFramesUsed = 0;
	long outputFramesGen = 0;
	for (ch_cnt_t ch = 0; ch < m_channels; ++ch)
	{
		auto data = SRC_DATA{};

		data.data_in = input.bufferPtr(ch);
		data.input_frames = input.frames();

		data.data_out = output.bufferPtr(ch);
		data.output_frames = output.frames();

		data.src_ratio = m_ratio;
		data.end_of_input = 0;

		if ((m_error = src_process(static_cast<SRC_STATE*>(m_states[ch].get()), &data)))
		{
			throw std::runtime_error{src_strerror(m_error)};
		}

		if (ch > 0)
		{
			if (data.input_frames_used != inputFramesUsed)
			{
				throw std::runtime_error{
					"Resampler used a different number of input frames for one of the channel buffers"};
			}
			if (data.output_frames_gen != outputFramesGen)
			{
				throw std::runtime_error{
					"Resampler generated a different number of output frames for one of the channel buffers"};
			}
		}
		inputFramesUsed = data.input_frames_used;
		outputFramesGen = data.output_frames_gen;
	}

	// Can have extra output channels, which are filled with the 1st channel's contents.
	// This allows mono-to-stereo conversion.
	for (ch_cnt_t ch = m_channels; ch < output.channels(); ++ch)
	{
		std::ranges::copy(output.buffer(0), output.bufferPtr(ch));
	}

	return {static_cast<f_cnt_t>(inputFramesUsed), static_cast<f_cnt_t>(outputFramesGen)};
}

void AudioResampler::reset()
{
	for (const State& state : m_states)
	{
		if ((m_error = src_reset(static_cast<SRC_STATE*>(state.get()))))
		{
			throw std::runtime_error{src_strerror(m_error)};
		}
	}
}

void AudioResampler::StateDeleter::operator()(void* state)
{
	src_delete(static_cast<SRC_STATE*>(state));
}

} // namespace lmms
