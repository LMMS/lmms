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
	float* dst, long frames, double ratio, WriteCallback writeCallback, void* writeCallbackData) -> Result
{
	auto result = Result{};
	while (frames > 0)
	{
		const auto numWriteBufferFrames = static_cast<long>(m_writeBuffer.size() / m_channels);
		const auto framesToWrite = std::min(numWriteBufferFrames, frames);
		writeCallback(m_writeBuffer.data(), framesToWrite, m_channels, writeCallbackData);

		auto data = SRC_DATA{.data_in = m_writeBuffer.data(),
			.data_out = dst,
			.input_frames = framesToWrite,
			.output_frames = frames,
			.end_of_input = framesToWrite < numWriteBufferFrames,
			.src_ratio = ratio};

		result.error = src_process(m_state, &data);
		if (result.error) { break; }

		dst += data.output_frames_gen * m_channels;
		frames -= data.output_frames_gen;
		result.inputFramesUsed += data.input_frames_used;
		result.outputFramesGenerated += data.output_frames_gen;
	}

	return result;
}

} // namespace lmms
