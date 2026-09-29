/*
 * MixHelpers.cpp - helper functions for mixing buffers
 *
 * Copyright (c) 2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * Copyright (c) 2026 Dalton Messmer <messmer.dalton/at/gmail.com>
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

#include "MixHelpers.h"

#include <algorithm>
#include <cmath>

#include "ValueBuffer.h"

namespace lmms::MixHelpers {
namespace {
constexpr auto SilenceThreshold = 0.000001f; // -120 dBFS
} // namespace

bool isSilent(std::span<const float> buffer)
{
	return std::ranges::all_of(buffer, [&](const float s) { return std::abs(s) < SilenceThreshold; });
}

bool isSilent(PlanarBufferSpan<const float> buffer)
{
	for (ch_cnt_t ch = 0; ch < buffer.channels(); ++ch)
	{
		if (!isSilent(buffer.buffer(ch))) { return false; }
	}
	return true;
}

void zero(PlanarBufferSpan<float> dst)
{
	const auto frames = dst.frames();
	const auto channels = dst.channels();
	for (ch_cnt_t ch = 0; ch < channels; ++ch)
	{
		std::fill_n(dst.bufferPtr(ch), frames, 0.f);
	}
}

void monoUpmix(PlanarBufferSpan<float> dst, PlanarBufferSpan<const float> src)
{
	assert(dst.channels() == 2);
	assert(src.channels() == 1);
	assert(dst.frames() >= src.frames());

	const auto frames = src.frames();
	for (f_cnt_t frame = 0; frame < frames; ++frame)
	{
		float sample = src[0][frame];
		dst[0][frame] = sample;
		dst[1][frame] = sample;
	}
}

void stereoDownmix(PlanarBufferSpan<float> dst, PlanarBufferSpan<const float> src)
{
	assert(dst.frames() >= src.frames());

	const auto frames = src.frames();
	for (f_cnt_t frame = 0; frame < frames; ++frame)
	{
		dst[0][frame] = (src[0][frame] + src[1][frame]) / 2;
	}
}

void copy(PlanarBufferSpan<float> dst, PlanarBufferSpan<const float> src)
{
	assert(dst.channels() >= src.channels());
	assert(dst.frames() >= src.frames());

	const auto channels = src.channels();
	const auto frames = src.frames();
	for (ch_cnt_t ch = 0; ch < channels; ++ch)
	{
		float* dstPtr = dst.bufferPtr(ch);
		const float* srcPtr = src.bufferPtr(ch);
		for (f_cnt_t frame = 0; frame < frames; ++frame)
		{
			dstPtr[frame] = srcPtr[frame];
		}
	}
}

void copy(PlanarBufferSpan<float> dst, InterleavedBufferSpan<const float> src)
{
	assert(dst.channels() >= src.channels());
	assert(dst.frames() >= src.frames());

	const auto channels = src.channels();
	const auto frames = src.frames();
	const float* const srcData = src.data();
	for (ch_cnt_t ch = 0; ch < channels; ++ch)
	{
		float* const dstPtr = dst.bufferPtr(ch);
		const float* srcPtr = srcData + ch;
		for (f_cnt_t frame = 0; frame < frames; ++frame, ++srcPtr)
		{
			dstPtr[frame] = *srcPtr;
		}
	}
}

void copyAndZero(PlanarBufferSpan<float> dst, PlanarBufferSpan<const float> src)
{
	copy(dst, src);

	// Zero any additional channels in the output buffer
	for (ch_cnt_t ch = src.channels(); ch < dst.channels(); ++ch)
	{
		std::ranges::fill(dst.buffer(ch).subspan(0, src.frames()), 0.f);
	}
}

void copyMix(PlanarBufferSpan<float> dst, PlanarBufferSpan<const float> src)
{
	if (dst.channels() == 2 && src.channels() == 1)
	{
		monoUpmix(dst, src);
	}
	else if (dst.channels() == 1 && src.channels() == 2)
	{
		stereoDownmix(dst, src);
	}
	else
	{
		copy(dst, src);
	}
}

void copyMixAndZero(PlanarBufferSpan<float> dst, PlanarBufferSpan<const float> src)
{
	if (dst.channels() == 2 && src.channels() == 1)
	{
		monoUpmix(dst, src);
	}
	else if (dst.channels() == 1 && src.channels() == 2)
	{
		stereoDownmix(dst, src);
	}
	else
	{
		copyAndZero(dst, src);
	}
}

void add(PlanarBufferView<sample_t> dst, PlanarBufferView<const sample_t> src)
{
	assert(dst.channels() == src.channels());
	assert(dst.frames() == src.frames());

	const auto channels = dst.channels();
	const auto frames = dst.frames();
	for (ch_cnt_t channel = 0; channel < channels; ++channel)
	{
		auto* dstPtr = dst.bufferPtr(channel);
		const auto* srcPtr = src.bufferPtr(channel);
		for (f_cnt_t frame = 0; frame < frames; ++frame)
		{
			dstPtr[frame] += srcPtr[frame];
		}
	}
}

void addMultiplied(PlanarBufferSpan<float> dst, PlanarBufferSpan<const float> src, float coeffSrc)
{
	assert(dst.channels() >= src.channels());
	assert(dst.frames() >= src.frames());

	const auto channels = src.channels();
	const auto frames = src.frames();
	for (ch_cnt_t ch = 0; ch < channels; ++ch)
	{
		float* dstPtr = dst.bufferPtr(ch);
		const float* srcPtr = src.bufferPtr(ch);
		for (f_cnt_t frame = 0; frame < frames; ++frame)
		{
			dstPtr[frame] += srcPtr[frame] * coeffSrc;
		}
	}
}

void multiply(PlanarBufferView<float> dst, float coeff, f_cnt_t offset)
{
	assert(offset < dst.frames());

	const ch_cnt_t channels = dst.channels();
	const f_cnt_t frames = dst.frames();
	for (ch_cnt_t ch = 0; ch < channels; ++ch)
	{
		float* dstPtr = dst.bufferPtr(ch);
		for (f_cnt_t frame = offset; frame < frames; ++frame)
		{
			dstPtr[frame] *= coeff;
		}
	}
}

void multiply(PlanarBufferView<float> dst, float coeff)
{
	const ch_cnt_t channels = dst.channels();
	const f_cnt_t frames = dst.frames();
	for (ch_cnt_t ch = 0; ch < channels; ++ch)
	{
		float* dstPtr = dst.bufferPtr(ch);
		for (f_cnt_t frame = 0; frame < frames; ++frame)
		{
			dstPtr[frame] *= coeff;
		}
	}
}

void addSwappedMultiplied(PlanarBufferSpan<float> dst, PlanarBufferSpan<const float> src, float coeffSrc)
{
	assert(dst.channels() == 2);
	assert(src.channels() == 2);
	assert(dst.frames() >= src.frames());

	const auto frames = src.frames();
	float* dstPtrL = dst.bufferPtr(0);
	float* dstPtrR = dst.bufferPtr(1);
	const float* srcPtrL = src.bufferPtr(0);
	const float* srcPtrR = src.bufferPtr(1);
	for (f_cnt_t frame = 0; frame < frames; ++frame)
	{
		dstPtrL[frame] += srcPtrR[frame] * coeffSrc;
		dstPtrR[frame] += srcPtrL[frame] * coeffSrc;
	}
}

void addMultipliedByBuffer(PlanarBufferView<float> dst, PlanarBufferView<const float> src,
	float coeffSrc, const ValueBuffer* coeffSrcBuf)
{
	assert(dst.channels() == src.channels());
	assert(dst.frames() == src.frames());

	const ch_cnt_t channels = dst.channels();
	const f_cnt_t frames = dst.frames();
	for (ch_cnt_t ch = 0; ch < channels; ++ch)
	{
		float* dstPtr = dst.bufferPtr(ch);
		const float* srcPtr = src.bufferPtr(ch);
		for (f_cnt_t frame = 0; frame < frames; ++frame)
		{
			dstPtr[frame] += srcPtr[frame] * coeffSrc * coeffSrcBuf->values()[frame];
		}
	}
}

void addMultipliedByBuffers(PlanarBufferView<float> dst, PlanarBufferView<const float> src,
	const ValueBuffer* coeffSrcBuf1, const ValueBuffer* coeffSrcBuf2)
{
	assert(dst.channels() == src.channels());
	assert(dst.frames() == src.frames());

	const ch_cnt_t channels = dst.channels();
	const f_cnt_t frames = dst.frames();
	for (ch_cnt_t ch = 0; ch < channels; ++ch)
	{
		float* dstPtr = dst.bufferPtr(ch);
		const float* srcPtr = src.bufferPtr(ch);
		for (f_cnt_t frame = 0; frame < frames; ++frame)
		{
			dstPtr[frame] += srcPtr[frame] * coeffSrcBuf1->values()[frame] * coeffSrcBuf2->values()[frame];
		}
	}
}

} // namespace lmms::MixHelpers
