/*
 * MixHelpers.h - helper functions for mixing buffers
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

#ifndef LMMS_MIX_HELPERS_H
#define LMMS_MIX_HELPERS_H

#include "AudioBufferSpan.h"

#include "lmms_export.h"

namespace lmms {

class ValueBuffer;

namespace MixHelpers {

//! @returns true if all samples within @p buffer fall below a silence threshold
//! @note NaN is considered non-silent
LMMS_EXPORT bool isSilent(std::span<const float> buffer);

//! @returns true if all samples within @p buffer fall below a silence threshold
//! @note NaN is considered non-silent
LMMS_EXPORT bool isSilent(PlanarBufferSpan<const float> buffer);

//! Fills the entire span with 0.f
LMMS_EXPORT void zero(PlanarBufferSpan<float> dst);

//! @brief Copies data from @a src to @a dst, upmixing from mono to stereo
//! @note If @a dst has more frames than @a src, the additional
//!       frames are left unmodified.
//! @param dst the output buffer
//! @param src the input buffer
//! @pre dst.channels() == 2
//! @pre src.channels() == 1
//! @pre dst.frames() >= src.frames()
LMMS_EXPORT void monoUpmix(PlanarBufferSpan<float> dst, PlanarBufferSpan<const float> src);

//! @brief Copies data from @a src to @a dst, downmixing from stereo to mono
//! @note If @a dst has more frames than @a src, the additional
//!       frames are left unmodified.
//! @param dst the output buffer
//! @param src the input buffer
//! @pre dst.channels() == 1
//! @pre src.channels() == 2
//! @pre dst.frames() >= src.frames()
LMMS_EXPORT void stereoDownmix(PlanarBufferSpan<float> dst, PlanarBufferSpan<const float> src);

//! @brief Copies data from @a src to @a dst
//! @note If @a dst has more channels or frames than @a src, the additional channels or frames are left unmodified.
//! @param dst the output buffer
//! @param src the input buffer
//! @pre dst.channels() >= src.channels()
//! @pre dst.frames() >= src.frames()
LMMS_EXPORT void copy(PlanarBufferSpan<float> dst, PlanarBufferSpan<const float> src);

//! @brief Copies data from @a src to @a dst
//! @note If @a dst has more channels or frames than @a src, the additional channels or frames are left unmodified.
//! @param dst the output buffer
//! @param src the input buffer
//! @pre dst.channels() >= src.channels()
//! @pre dst.frames() >= src.frames()
LMMS_EXPORT void copy(PlanarBufferView<float> dst, PlanarBufferView<const float> src);

//! @brief Copies data from @a src to @a dst, performing interleaved to planar conversion
//! @note If @a dst  has more channels or frames than @a src,
//!       the additional channels or frames are left unmodified.
//! @param dst the output buffer
//! @param src the input buffer
//! @pre dst.channels() >= src.channels()
//! @pre dst.frames() >= src.frames()
LMMS_EXPORT void copy(PlanarBufferSpan<float> dst, InterleavedBufferSpan<const float> src);

//! @brief Copies data from @a src to @a dst, performing planar to interleaved conversion
//! @note If @a dst  has more channels or frames than @a src,
//!       the additional channels or frames are left unmodified.
//! @param dst the output buffer
//! @param src the input buffer
//! @pre dst.channels() >= src.channels()
//! @pre dst.frames() >= src.frames()
LMMS_EXPORT void copy(InterleavedBufferSpan<float> dst, PlanarBufferSpan<const float> src);

//! @brief Copies data from @a src to @a dst
//! @note If @a dst has more channels than @a src, the additional channels are zeroed,
//!       but only the first `src.frames()` frames.
//! @note If @a dst has more frames than @a src, the additional frames are left unmodified.
//! @param dst the output buffer
//! @param src the input buffer
//! @pre dst.channels() >= src.channels()
//! @pre dst.frames() >= src.frames()
LMMS_EXPORT void copyAndZero(PlanarBufferSpan<float> dst, PlanarBufferSpan<const float> src);

//! Same as @ref copy(PlanarBufferSpan<float>, PlanarBufferSpan<const float>) but
//! applies @ref monoUpmix or @ref stereoDownmix if possible.
//! @pre dst.channels() >= src.channels() || (dst.channels() == 1 && src.channels() == 2)
//! @pre dst.frames() >= src.frames()
LMMS_EXPORT void copyMix(PlanarBufferSpan<float> dst, PlanarBufferSpan<const float> src);

//! Same as @ref copyAndZero(PlanarBufferSpan<float>, PlanarBufferSpan<const float>) but
//! applies @ref monoUpmix or @ref stereoDownmix if possible.
//! @pre dst.channels() >= src.channels() || (dst.channels() == 1 && src.channels() == 2)
//! @pre dst.frames() >= src.frames()
LMMS_EXPORT void copyMixAndZero(PlanarBufferSpan<float> dst, PlanarBufferSpan<const float> src);

//! @brief Add samples from src to dst
LMMS_EXPORT void add(PlanarBufferView<sample_t> dst, PlanarBufferView<const sample_t> src);

//! @brief Multiply samples from `dst` by `coeff` starting at `offset`
LMMS_EXPORT void multiply(PlanarBufferView<float> dst, float coeff, f_cnt_t offset);

//! @brief Multiply samples from `dst` by `coeff`
LMMS_EXPORT void multiply(PlanarBufferView<float> dst, float coeff);

//! @brief Add samples from src multiplied by coeffSrc to dst
LMMS_EXPORT void addMultiplied(PlanarBufferSpan<float> dst, PlanarBufferSpan<const float> src, float coeffSrc);

//! @brief Add samples from src multiplied by coeffSrc to dst, swap inputs
LMMS_EXPORT void addSwappedMultiplied(PlanarBufferSpan<float> dst, PlanarBufferSpan<const float> src, float coeffSrc);

//! @brief Add samples from src multiplied by coeffSrc and coeffSrcBuf to dst
LMMS_EXPORT void addMultipliedByBuffer(PlanarBufferView<float> dst, PlanarBufferView<const float> src,
	float coeffSrc, const ValueBuffer* coeffSrcBuf);

//! @brief Add samples from src multiplied by coeffSrcBuf1 and coeffSrcBuf2 to dst
LMMS_EXPORT void addMultipliedByBuffers(PlanarBufferView<float> dst, PlanarBufferView<const float> src,
	const ValueBuffer* coeffSrcBuf1, const ValueBuffer* coeffSrcBuf2);

} // namespace MixHelpers
} // namespace lmms

#endif // LMMS_MIX_HELPERS_H
