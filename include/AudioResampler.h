/*
 * AudioResampler.h
 *
 * Copyright (c) 2025 Sotonye Atemie <sakertooth@gmail.com>
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

#include <cassert>
#include <functional>
#include <samplerate.h>

#include "lmms_constants.h"
#include "lmms_export.h"

namespace lmms {

//! A RAII wrapper over libsamplerate meant to simplify its usage.
class LMMS_EXPORT AudioResampler
{
public:
	//! A result returned by @ref `WriteCallback`.
	struct WriteCallbackResult
	{
		bool done = false; //! The callback should set this to `true` if the callback is done streaming input.
		long frames = 0;   //! The number of frames the callback has written in.
	};

	//! The callback that writes input data to @p dst of the given size to the resampler when necessary.
	//! The callback should return the number of frames actually written to @p dst, or
	using WriteCallback = std::function<WriteCallbackResult(float* dst, long frames, int channels)>;

	//! Create a resampler with the given interpolation mode and number of channels.
	//! The constructor assumes stereo audio by default.
	AudioResampler(int mode, int channels = DEFAULT_CHANNELS);

	//! Destroys the resampler and frees any internal state it holds.
	~AudioResampler();

	//! Resamplers cannot be copied.
	AudioResampler(const AudioResampler&) = delete;

	//! Resamplers cannot be copied.
	AudioResampler& operator=(const AudioResampler&) = delete;

	//! Moves the state from one resampler to another.
	//! Use of the moved from resampler is a no-op.
	AudioResampler(AudioResampler&&) noexcept;

	//! Moves the state from one resampler to another.
	//! Use of the moved from resampler is a no-op.
	AudioResampler& operator=(AudioResampler&&) noexcept;

	//! Resamples audio into @p dst with at the given @p ratio.
	//! The source audio is fetched periodically from @p callback.
	//! Returns `false` on error or when no more input can be resampled and outputted into @p dst.
	//! Returns `true` if all of @p dst was successfully filled with resampled audio data.
	auto resample(float* dst, long frames, double ratio, WriteCallback callback) -> bool;

	//! Resamples audio into @p dst frames at the given @p ratio.
	//! @p src is used as a source for retrieving input to resample.
	//! Callers are expected to give enough size to fit @p src into @p dst when resampled.
	//! Returns `true` when all input has been resampled and stored in @p dst, and `false` otherwise.
	auto resample(float* dst, long dstFrames, const float* src, long srcFrames, double ratio) -> bool;

	//! Returns the number of channels expected by the resampler.
	auto channels() const -> int { return m_channels; }

	//! Returns the interpolation mode used by this resampler.
	auto mode() const -> int { return m_mode; }

private:
	std::array<float, 256> m_buffer{};
	SRC_STATE* m_state = nullptr;
	SRC_DATA m_data = SRC_DATA{};
	int m_channels = 0;
	int m_mode = 0;
	int m_error = 0;
};
} // namespace lmms

#endif // LMMS_AUDIO_RESAMPLER_H
