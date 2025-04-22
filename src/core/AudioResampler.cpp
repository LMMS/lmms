/*
 * AudioResampler.cpp - wrapper for libsamplerate
 *
 * Copyright (c) 2023 saker <sakertooth@gmail.com>
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

namespace lmms {

AudioResampler::AudioResampler(InterpolationMode interpolationMode, int channels)
	: m_interpolationMode(interpolationMode)
	, m_writeBuffer({})
	, m_state(src_new(static_cast<int>(interpolationMode), channels, &m_error))
	, m_channels(channels)
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

auto AudioResampler::resample(
	SampleFrame* dst, size_t frames, double ratio, WriteCallback writeCallback, void* writeCallbackData) -> Result
{
	auto result = Result{};
	while (frames > 0)
	{
		const auto numInputFrames = std::min(m_writeBuffer.size(), frames);
		writeCallback(m_writeBuffer.data(), numInputFrames, writeCallbackData);

		auto data = SRC_DATA{.data_in = &m_writeBuffer.data()[0][0],
			.data_out = &dst[0][0],
			.input_frames = static_cast<long>(numInputFrames),
			.output_frames = static_cast<long>(frames),
			.end_of_input = 0,
			.src_ratio = ratio};

		result.error = src_process(m_state, &data);
		if (result.error) { break; }

		dst += data.output_frames_gen;
		frames -= data.output_frames_gen;
		result.inputFramesUsed += data.input_frames_used;
		result.outputFramesGenerated += data.output_frames_gen;
	}

	return result;
}

} // namespace lmms
