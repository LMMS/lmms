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
#include <cassert>
#include <samplerate.h>
#include <span>

#include "SampleFrame.h"
#include "lmms_export.h"

namespace lmms {

//! An audio resampler.
//! This can be used to resample or transpose an audio signal with various interpolation modes.
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

	//! The callback that writes input data to @p dst of the given size to the resampler when necessary.
	//! The callback should return the number of frames actually written to @p dst.
	using WriteCallback = std::size_t (*)(SampleFrame* dst, std::size_t frames, void* data);

	//! Create a resampler with the given @p interpolationMode.
	AudioResampler(InterpolationMode interpolationMode);

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

	//! Returns a view into the input buffer that callers can write to.
	auto inputWriterView() -> std::span<SampleFrame>;

	//! Returns a view into the output buffer that callers can read from.
	auto outputReaderView() const -> std::span<const SampleFrame>;

	//! Commit to writing @p frames to the input buffer.
	void commitInputWrite(std::size_t frames);

	//! Commit to reading @p frames from the output buffer.
	void commitOutputRead(std::size_t frames);

	//! Resamples the audio from the input buffer at the given @p ratio.
	//! Returns non-zero on error;
	auto resample(double ratio) -> int;

	//! Returns the interpolation mode the resampler is using.
	auto interpolationMode() const -> InterpolationMode { return m_interpolationMode; }

	//! Returns the number of channels expected by the resampler.
	constexpr auto channels() const -> int { return DEFAULT_CHANNELS; }

	//! Returns the textual name for the given interpolation mode.
	static auto interpolationModeName(InterpolationMode mode) -> const char*;

	//! Returns the description for a resampling error.
	static auto errorDescription(int error) -> const char*;

private:
	struct Stream
	{
		std::array<SampleFrame, 32> buffer;
		long readerIndex = 0;
		long writerIndex = 0;
	};

	Stream m_input;
	Stream m_output;
	SRC_STATE* m_state = nullptr;
	SRC_DATA m_data = SRC_DATA{};
	InterpolationMode m_interpolationMode = AudioResampler::InterpolationMode::None;
	int m_error = 0;
};
} // namespace lmms

#endif // LMMS_AUDIO_RESAMPLER_H
