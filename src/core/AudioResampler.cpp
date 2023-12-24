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

namespace lmms {

AudioResampler::AudioResampler(SRC_STATE* state, int interpolationMode, int channels)
	: m_state(state)
	, m_interpolationMode(interpolationMode)
	, m_channels(channels)
{
	if (!m_state)
	{
		const auto message = std::string{"Failed to create an AudioResampler: "} + src_strerror(m_error);
		throw std::runtime_error{message};
	}
}

AudioResampler::AudioResampler(int interpolationMode, int channels)
	: AudioResampler(src_new(interpolationMode, channels, &m_error), interpolationMode, channels)
{
}

AudioResampler::~AudioResampler()
{
	src_delete(m_state);
}

auto AudioResampler::resample(const float* in, long inputFrames, float* out, long outputFrames, double ratio) const
	-> ProcessResult
{
	auto data = SRC_DATA{};
	data.data_in = in;
	data.input_frames = inputFrames;
	data.data_out = out;
	data.output_frames = outputFrames;
	data.src_ratio = ratio;
	return {src_process(m_state, &data), data.input_frames_used, data.output_frames_gen};
}

} // namespace lmms