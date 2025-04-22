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

AudioResampler::AudioResampler(InterpolationMode interpolationMode)
	: m_interpolationMode(interpolationMode)
	, m_state(src_new(static_cast<int>(interpolationMode), DEFAULT_CHANNELS, &m_error))
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

void AudioResampler::resample(SampleFrame* dst, size_t frames, double ratio, WriteCallback callback, void* callbackData)
{
	auto data = SRC_DATA{.data_in = &m_writeBuffer.data()[0][0],
		.data_out = &dst[0][0],
		.input_frames = 0,
		.output_frames = static_cast<long>(frames),
		.end_of_input = 0,
		.src_ratio = ratio};

	while (data.output_frames > 0)
	{
		if (data.input_frames == 0)
		{
			const auto numInputFrames = callback(m_writeBuffer.data(), m_writeBuffer.size(), callbackData);
			data.data_in = &m_writeBuffer.data()[0][0];
			data.input_frames = numInputFrames;
		}

		if (data.input_frames < 0 || src_process(m_state, &data))
		{
			std::fill_n(data.data_out, data.output_frames * DEFAULT_CHANNELS, 0.0f);
			break;
		}

		data.data_in += data.input_frames_used * DEFAULT_CHANNELS;
		data.input_frames -= data.input_frames_used;

		data.data_out += data.output_frames_gen * DEFAULT_CHANNELS;
		data.output_frames -= data.output_frames_gen;
	}
}

} // namespace lmms
