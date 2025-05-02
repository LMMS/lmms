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

#include <array>
#include <functional>
#include <samplerate.h>

#include "lmms_constants.h"
#include "lmms_export.h"

namespace lmms {

//! An audio resampler.
//! This can be used to convert sample rates, transpose a signal in pitch, etc.
//! Only works with interleaved audio (i.e, [LRLRLR...])
class LMMS_EXPORT AudioResampler
{
public:
	//! The callback that writes input data to @p dst of the given size to the resampler when necessary.
	//! The callback should return the number of frames written into @p dst.
	using WriteCallback = std::function<std::size_t(float* dst, std::size_t frames, int channels)>;

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

	//! Fetch source data from a callback when resampling next.
	void setSource(WriteCallback callback);

	//! Retrieve source data from @p src when resampling next.
	void setSource(const float* src, std::size_t size);

	//! Resamples audio into @p dst with at the given @p ratio.
	void resample(float* dst, std::size_t frames, double ratio);

	//! Returns the number of channels expected by the resampler.
	auto channels() const -> int { return m_channels; }

	//! Returns the interpolation mode used by this resampler.
	auto mode() const -> int { return m_mode; }

	//! Returns an error description for errors returned by \ref resample.
	static auto errorDescription(int error) -> const char* { return src_strerror(error); }

private:
	std::array<float, 256> m_buffer{};
	SRC_DATA m_data = SRC_DATA{};
	WriteCallback m_callback;
	SRC_STATE* m_state = nullptr;
	int m_channels = 0;
	int m_mode = 0;
	int m_error = 0;
};
} // namespace lmms

#endif // LMMS_AUDIO_RESAMPLER_H
