/*
 * AudioResampler.h - wrapper around libsamplerate
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
#include <samplerate.h>

#include "lmms_export.h"

namespace lmms {

class LMMS_EXPORT AudioResampler
{
public:
	//! The interpolation modes supported by this resampler.
	//! Note that this is subject to change if the resampler is implemented differently.
	enum class InterpolationMode : int
	{
		None = -1,
		ZeroOrderHold = SRC_ZERO_ORDER_HOLD,
		Linear = SRC_LINEAR,
		SincFast = SRC_SINC_FASTEST,
		SincMedium = SRC_SINC_MEDIUM_QUALITY,
		SincBest = SRC_SINC_BEST_QUALITY,
		Count
	};

	struct Result
	{
		int error;
		long outputFramesGenerated;
		long inputFramesUsed;
	};

	//! The callback that writes input data to @p dst of the given size to the resampler when necessary.
	//! @p data is an optional parameter that can be specified by callers for any additional context needed to
	//! process their callback.
	using WriteCallback = void (*)(float* dst, std::size_t frames, std::size_t channels, void* data);

	AudioResampler(InterpolationMode interpolationMode, int channels);
	AudioResampler(const AudioResampler&) = delete;
	AudioResampler(AudioResampler&&) = delete;
	~AudioResampler();

	AudioResampler& operator=(const AudioResampler&);
	AudioResampler& operator=(AudioResampler&&) noexcept = delete;

	//! Resample the audio outputted from the write callback at the given conversion @p ratio. 
	//! @p dst is expected to be a interleaved audio buffer (e.g. LRLRLR for stereo)
	//! @p dst is expected to consist of @p frames frames (number of samples / number of channels)
	auto resample(float* dst, long frames, double ratio, WriteCallback writeCallback, void* writeCallbackData) -> Result;

	//! Returns the interpolation mode the resampler is using.
	auto interpolationMode() const -> InterpolationMode { return m_interpolationMode; }

	//! Returns the number of channels expected by the resampler.
	auto channels() const -> int { return m_channels; }

	//! Returns the textual name for the given interpolation mode.
	static auto interpolationModeName(InterpolationMode mode) -> const char* { return src_get_name(static_cast<int>(mode)); }

private:
	InterpolationMode m_interpolationMode = AudioResampler::InterpolationMode::None;
	std::array<float, 64> m_writeBuffer;
	SRC_STATE* m_state = nullptr;
	int m_channels = 0;
	int m_error = 0;
};
} // namespace lmms

#endif // LMMS_AUDIO_RESAMPLER_H
