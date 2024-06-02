/*
 * AudioResampler.h - wrapper around libsamplerate
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

#ifndef LMMS_AUDIO_RESAMPLER_H
#define LMMS_AUDIO_RESAMPLER_H

#include <cstddef>
#include <samplerate.h>

#include "lmms_basics.h"
#include "lmms_export.h"

namespace lmms {
class LMMS_EXPORT AudioResampler
{
public:
	//! Resample `numSrcFrames` sample frames from `src` to `numDstFrames` sample frames in `dst` with a ratio of
	//! `ratio = output_sample_rate/input_sample_rate`. Callers are expected to provide some margin of sample values (at
	//! least 4 extra samples) at the end of `src` to ensure correct interpolation of sample values. Returns an error
	//! when this function completes, if any.
	void resample(float* dst, const float* src, size_t numDstFrames, size_t numSrcFrames, double ratio);

	//! Interpolates the sample value at position `index` within `src` containing `size` samples.
	//! Assumes that `src` represents an array of sample frames containing two samples each.
	//! `prev` can be provided to provide the sample frame that occurs at `index - 1 < 0`.
	static float interpolate(
		const float* src, size_t size, int index, float fractionalOffset, float* prev = nullptr);

private:
	std::array<float, DEFAULT_CHANNELS> m_prevSamples{0.0f, 0.0f};
};
} // namespace lmms

#endif // LMMS_AUDIO_RESAMPLER_H
