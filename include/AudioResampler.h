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

#include <functional>
#include <samplerate.h>
#include <span>

#include "lmms_constants.h"
#include "lmms_export.h"

namespace lmms {

//! An audio resampler.
//! This can be used to convert sample rates, transpose a signal in pitch, etc.
//! Only works with interleaved audio (i.e, [LRLRLR...])
class LMMS_EXPORT AudioResampler
{
public:
	//! The resampling results returned after calls are made to @ref process.
	struct Result
	{
		long inputFramesUsed;		//! The number of input frames used.
		long outputFramesGenerated; //! The number of output frames generated.
	};

	//! Writes data into @p dst.
	//! @p dst is of size `channels() * frames`.
	//! @return The number of frames written into @p dst.
	using InputCallback = std::function<long(float* dst, long frames, int channels)>;

	//! Reads all data from @p src.
	//! Must acknowledge reading all data given to it.
	//! @p src is of size `channels() * frames`.
	using OutputCallback = std::function<void(const float* src, long frames, int channels)>;

	//! Create a resampler with the given interpolation mode and number of channels.
	//! The constructor assumes stereo audio by default.
	AudioResampler(int mode, int channels = DEFAULT_CHANNELS);

	//! Destroys the resampler and frees any internal state it holds.
	~AudioResampler();

	//! Resamplers cannot be copied.
	AudioResampler(const AudioResampler&) = delete;

	//! Resamplers cannot be copied.
	AudioResampler& operator=(const AudioResampler&) = delete;

	//! Moves the internal state from one resampler to another.
	AudioResampler(AudioResampler&&) noexcept;

	//! Moves the internal state from one resampler to another.
	AudioResampler& operator=(AudioResampler&&) noexcept;

	//! Resamples audio into @p dst at the given @p ratio.
	//! Stops when @p dst is full, or @p callback stops writing input.
	//! Uses @p callback to fetch input data as necessary.
	//! @return The resampling results. See @ref Result.
	[[nodiscard]] auto process(float* dst, long frames, double ratio, InputCallback callback) -> Result;

	//! Resamples audio from @p src at the given @p ratio.
	//! Stops when @p src is empty, or @p callback stops reading output.
	//! Uses @p callback to send resampled output data as necessary.
	//! @return The resampling results. See @ref Result.
	[[nodiscard]] auto process(const float* src, long frames, double ratio, OutputCallback callback) -> Result;

	//! Resamples audio from @p src into @p dst at the given @p ratio.
	//! Stops when @p dst is full, or @p src is empty.
	//! @return The resampling results. See @ref Result.
	[[nodiscard]] auto process(const float* src, float* dst, long srcFrames, long dstFrames, double ratio) -> Result;

	//! Returns the number of channels expected by the resampler.
	auto channels() const -> int { return m_channels; }

	//! Returns the interpolation mode used by this resampler.
	auto mode() const -> int { return m_mode; }

private:
	static constexpr auto BufferFrameSize = 64;
	std::vector<float> m_inputBuffer;
	std::vector<float> m_outputBuffer;
	std::span<const float> m_inputBufferWindow;
	SRC_STATE* m_state = nullptr;
	int m_channels = 0;
	int m_mode = 0;
	int m_error = 0;
};
} // namespace lmms

#endif // LMMS_AUDIO_RESAMPLER_H
