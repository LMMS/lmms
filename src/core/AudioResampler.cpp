/*
 * AudioResampler.cpp
 *
 * Copyright (c) 2024 saker
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

#include <algorithm>
#include <samplerate.h>

#include "interpolation.h"

namespace lmms {

void AudioResampler::resample(float* dst, const float* src, size_t numDstFrames, size_t numSrcFrames, double ratio)
{
	const auto numSrcSamples = numSrcFrames * DEFAULT_CHANNELS;
	for (size_t dstFrameIndex = 0; dstFrameIndex < numDstFrames; ++dstFrameIndex)
	{
		const auto srcFrameIndex = static_cast<double>(dstFrameIndex) / ratio;
		const auto truncatedSrcFrameIndex = static_cast<int>(srcFrameIndex);
		const auto fractionalOffset = srcFrameIndex - truncatedSrcFrameIndex;

		const auto leftX = truncatedSrcFrameIndex * DEFAULT_CHANNELS;
		const auto rightX = leftX + 1;

		const auto dstLeftIndex = dstFrameIndex * DEFAULT_CHANNELS;
		const auto dstRightIndex = dstLeftIndex + 1;

		dst[dstLeftIndex] = interpolate(src, numSrcSamples, leftX, fractionalOffset, m_prevSamples.data());
		dst[dstRightIndex] = interpolate(src, numSrcSamples, rightX, fractionalOffset, m_prevSamples.data());
	}

	const auto endFrame = numDstFrames * DEFAULT_CHANNELS;
	m_prevSamples[0] = dst[endFrame - 2];
	m_prevSamples[1] = dst[endFrame - 1];
}

float AudioResampler::interpolate(const float* src, size_t size, int index, float fractionalOffset, float* prev)
{
	const auto x0 = index - DEFAULT_CHANNELS;
	const auto x1 = index;
	const auto x2 = index + DEFAULT_CHANNELS;
	const auto x3 = index + DEFAULT_CHANNELS * 2;

	const auto y0 = (x0 < 0 && prev) ? prev[index] : (x0 < 0 || x0 >= size) ? 0.0f : src[x0];
	const auto y1 = (x1 < 0 || x1 >= size) ? 0.0f : src[x1];
	const auto y2 = (x2 < 0 || x2 >= size) ? 0.0f : src[x2];
	const auto y3 = (x3 < 0 || x3 >= size) ? 0.0f : src[x3];

	return hermiteInterpolate(y0, y1, y2, y3, fractionalOffset);
}

void AudioResampler::clear()
{
	std::fill(m_prevSamples.begin(), m_prevSamples.end(), 0.0f);
}

} // namespace lmms
