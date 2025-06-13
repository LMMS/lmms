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

	//! Copy resampler state from one to another.
	AudioResampler(const AudioResampler&);

	//! Copy resampler state from one to another.
	AudioResampler& operator=(const AudioResampler&);

	//! Moves the internal state from one resampler to another.
	AudioResampler(AudioResampler&&) noexcept;

	//! Moves the internal state from one resampler to another.
	AudioResampler& operator=(AudioResampler&&) noexcept;

	//! Resample audio from the input to the output.
	//! @return The resampling results. See @ref Result.
	[[nodiscard]] auto process() -> Result;

	//! Advance and shrink the input buffer by @p frames frames.
	void advanceInput(std::size_t frames);

	//! Advance and shrink the output buffer by @p frames frames.
	void advanceOutput(std::size_t frames);

	//! Set the input buffer view to @p input .
	void setInput(InterleavedBufferView<const float> input);

	//! Set the output buffer view to @p output .
	void setOutput(InterleavedBufferView<float> output);

	//! Set the resampling ratio to @p ratio .
	//! @p ratio is equal to the output sample rate divided by the input sample rate.
	void setRatio(double ratio) { m_data.src_ratio = ratio; }

	//! Set the end of input flag.
	void setEndOfInput(bool endOfInput) { m_data.end_of_input = endOfInput; }

	//! @return The input buffer.
	auto input() -> InterleavedBufferView<const float>
	{
		return {m_data.data_in, m_channels, static_cast<f_cnt_t>(m_data.input_frames)};
	}

	//! @return The output buffer.
	auto output() -> InterleavedBufferView<float>
	{
		return {m_data.data_out, m_channels, static_cast<f_cnt_t>(m_data.output_frames)};
	}

	//! @return The resampling ratio.
	auto ratio() const -> double { return m_data.src_ratio; }

	//! @return End of input flag. `true` if no more input data is available.
	auto endOfInput() const -> bool { return m_data.end_of_input; }

	//! @return The number of channels expected by the resampler.
	auto channels() const -> ch_cnt_t { return m_channels; }

	//! @return The interpolation mode used by this resampler.
	auto mode() const -> Mode { return m_mode; }

private:
	static auto converterType(Mode mode) -> int;
	SRC_STATE* m_state = nullptr;
	SRC_DATA m_data{};
	Mode m_mode;
	ch_cnt_t m_channels = 0;
	int m_error = 0;
};
} // namespace lmms

#endif // LMMS_AUDIO_RESAMPLER_H
