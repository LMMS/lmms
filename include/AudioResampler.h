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

#include <samplerate.h>

#include "AudioBufferView.h"
#include "lmms_constants.h"
#include "lmms_export.h"

namespace lmms {

//! An audio resampler.
//! This can be used to convert sample rates, transpose a signal in pitch, etc.
//! Only works with interleaved audio (i.e, [LRLRLR...])
class LMMS_EXPORT AudioResampler
{
public:
	enum class Mode
	{
		ZOH,		 //!< Zero order hold mode.
		Linear,		 //!< Linear mode.
		SincFastest, //!< Fastest sinc mode.
		SincMedium,	 //!< Medium sinc quality mode.
		SincBest	 //!< Best sinc quality mode.
	};

	//! The resampling results returned by @ref process.
	struct Result
	{
		f_cnt_t inputFramesUsed;	   //!< The number of input frames used.
		f_cnt_t outputFramesGenerated; //!< The number of output frames generated.
	};

	//! Create a resampler with the given interpolation mode and number of channels.
	//! The constructor assumes stereo audio by default.
	AudioResampler(Mode mode, ch_cnt_t channels = DEFAULT_CHANNELS);

	//! Destroys the resampler and frees any internal state it holds.
	~AudioResampler();

	//! Cannot copy resampler.
	AudioResampler(const AudioResampler&) = delete;

	//! Cannot copy resampler.
	AudioResampler& operator=(const AudioResampler&) = delete;

	//! Moves the internal state from one resampler to another.
	AudioResampler(AudioResampler&&) noexcept;

	//! Moves the internal state from one resampler to another.
	AudioResampler& operator=(AudioResampler&&) noexcept;

	//! Resample audio from the @p input into the @p output at the assigned resampling ratio.
	//! @return The resampling results. See @ref Result.
	[[nodiscard]] auto process(InterleavedBufferView<const float> input, InterleavedBufferView<float> output) -> Result;

	//! Reset the resampler state. Needed when working on unrelated pieces of audio.
	void reset();

	//! Set the resampling ratio to @p ratio .
	void setRatio(double ratio) { m_ratio = ratio; }

	//! Set the resampling ratio to @p output / @p input .
	void setRatio(sample_rate_t input, sample_rate_t output) { m_ratio = static_cast<double>(output) / input; }

	//! @return The resampling ratio.
	auto ratio() const -> double { return m_ratio; }

	//! @return The number of channels expected by the resampler.
	auto channels() const -> ch_cnt_t { return m_channels; }

	//! @return The interpolation mode used by this resampler.
	auto mode() const -> Mode { return m_mode; }

private:
	static auto converterType(Mode mode) -> int;
	SRC_STATE* m_state = nullptr;
	Mode m_mode;
	ch_cnt_t m_channels = 0;
	double m_ratio = 1.0;
	int m_error = 0;
};
} // namespace lmms

#endif // LMMS_AUDIO_RESAMPLER_H
