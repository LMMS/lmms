/*
 * MixHelpers.h - helper functions for mixing buffers
 *
 * Copyright (c) 2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef LMMS_MIX_HELPERS_H
#define LMMS_MIX_HELPERS_H

#include "AudioBufferView.h"

namespace lmms
{

class ValueBuffer;
class SampleFrame;

namespace MixHelpers
{

bool isSilent( const SampleFrame* src, int frames );

bool useNaNHandler();

void setNaNHandler( bool use );

/**
 * @brief Sanitizes a buffer of infs/NaNs, zeroing the entire buffer if
 *        any is detected.
 *
 * Only performs sanitization when the NaN handler is active.
 *
 * @returns true if inf or NaN was detected
 */
bool sanitize(std::span<sample_t> buffer);

/*! \brief Add samples from src to dst */
void add( SampleFrame* dst, const SampleFrame* src, int frames );

/*! \brief Add samples from src to dst */
void add(PlanarBufferView<sample_t> dst, PlanarBufferView<const sample_t> src);

/*! \brief Multiply samples from `dst` by `coeff` */
void multiply(SampleFrame* dst, float coeff, int frames);

/*! \brief Add samples from src multiplied by coeffSrc to dst */
void addMultiplied( SampleFrame* dst, const SampleFrame* src, float coeffSrc, int frames );

/*! \brief Add samples from src multiplied by coeffSrc to dst, swap inputs */
void addSwappedMultiplied( SampleFrame* dst, const SampleFrame* src, float coeffSrc, int frames );

/*! \brief Add samples from src multiplied by coeffSrc and coeffSrcBuf to dst */
void addMultipliedByBuffer( SampleFrame* dst, const SampleFrame* src, float coeffSrc, ValueBuffer * coeffSrcBuf, int frames );

/*! \brief Add samples from src multiplied by coeffSrc and coeffSrcBuf to dst */
void addMultipliedByBuffers( SampleFrame* dst, const SampleFrame* src, ValueBuffer * coeffSrcBuf1, ValueBuffer * coeffSrcBuf2, int frames );

/*! \brief Same as addMultiplied, but sanitize output (strip out infs/nans) */
void addSanitizedMultiplied( SampleFrame* dst, const SampleFrame* src, float coeffSrc, int frames );

/*! \brief Add samples from src multiplied by coeffSrc and coeffSrcBuf to dst - sanitized version */
void addSanitizedMultipliedByBuffer( SampleFrame* dst, const SampleFrame* src, float coeffSrc, ValueBuffer * coeffSrcBuf, int frames );

/*! \brief Add samples from src multiplied by coeffSrc and coeffSrcBuf to dst - sanitized version */
void addSanitizedMultipliedByBuffers( SampleFrame* dst, const SampleFrame* src, ValueBuffer * coeffSrcBuf1, ValueBuffer * coeffSrcBuf2, int frames );

/*! \brief Add samples from src multiplied by coeffSrcLeft/coeffSrcRight to dst */
void addMultipliedStereo( SampleFrame* dst, const SampleFrame* src, float coeffSrcLeft, float coeffSrcRight, int frames );

/*! \brief Multiply dst by coeffDst and add samples from src multiplied by coeffSrc */
void multiplyAndAddMultiplied( SampleFrame* dst, const SampleFrame* src, float coeffDst, float coeffSrc, int frames );

/*! \brief Multiply dst by coeffDst and add samples from srcLeft/srcRight multiplied by coeffSrc */
void multiplyAndAddMultipliedJoined( SampleFrame* dst, const sample_t* srcLeft, const sample_t* srcRight, float coeffDst, float coeffSrc, int frames );

//! Copies planar buffers to interleaved buffers
template<class T, proc_ch_t inputs, proc_ch_t outputs>
constexpr void copy(InterleavedBufferView<std::remove_const_t<T>, outputs> dst,
	PlanarBufferView<T, inputs> src)
{
	assert(src.frames() == dst.frames());
	if constexpr (inputs == DynamicChannelCount || outputs == DynamicChannelCount)
	{
		assert(src.channels() == dst.channels());
	}
	else { static_assert(inputs == outputs); }

	for (f_cnt_t frame = 0; frame < dst.frames(); ++frame)
	{
		auto* framePtr = dst.framePtr(frame);
		for (proc_ch_t channel = 0; channel < dst.channels(); ++channel)
		{
			framePtr[channel] = src.bufferPtr(channel)[frame];
		}
	}
}

//! Copies interleaved buffers to planar buffers
template<class T, proc_ch_t inputs, proc_ch_t outputs>
constexpr void copy(PlanarBufferView<std::remove_const_t<T>, outputs> dst,
	InterleavedBufferView<T, inputs> src)
{
	assert(src.frames() == dst.frames());
	if constexpr (inputs == DynamicChannelCount || outputs == DynamicChannelCount)
	{
		assert(src.channels() == dst.channels());
	}
	else { static_assert(inputs == outputs); }

	for (proc_ch_t channel = 0; channel < dst.channels(); ++channel)
	{
		auto* channelPtr = dst.bufferPtr(channel);
		for (f_cnt_t frame = 0; frame < dst.frames(); ++frame)
		{
			channelPtr[frame] = src.framePtr(frame)[channel];
		}
	}
}

} // namespace MixHelpers


} // namespace lmms

#endif // LMMS_MIX_HELPERS_H
