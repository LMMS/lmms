/*
 * AudioResampler.h
 *
 * Copyright (c) 2025 saker <sakertooth@gmail.com>
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

#include <memory>
#include "AudioBufferView.h"
#include "lmms_export.h"

namespace lmms {

/**
 * @class AudioResampler
 * @brief A utility class for resampling interleaved audio buffers using various resampling algorithms.
 *
 * This class provides support for zero-order hold, linear, and several levels of sinc-based resampling.
 */
class LMMS_EXPORT AudioResampler
{
public:
	/**
	 * @enum Mode
	 * @brief Defines the resampling method to use.
	 */
	enum class Mode
	{
		ZOH,		 //!< Zero Order Hold (nearest-neighbor) interpolation.
		Linear,		 //!< Linear interpolation.
		SincFastest, //!< Fastest sinc-based resampling.
		SincMedium,	 //!< Medium quality sinc-based resampling.
		SincBest	 //!< Highest quality sinc-based resampling.
	};

	/**
	 * @struct Result
	 * @brief Result of a resampling operation.
	 */
	struct Result
	{
		f_cnt_t inputFramesUsed;	   //!< The number of input frames used during processing.
		f_cnt_t outputFramesGenerated; //!< The number of output frames generated during processing.
	};

	/**
	 * @brief Constructs an `AudioResampler` instance.
	 * @param mode The resampling mode to use.
	 * @param channels Number of audio channels. Defaults to `2` (stereo).
	 */
	AudioResampler(Mode mode, ch_cnt_t channels = 2);

	/**
	 * @brief Process a block of interleaved audio input from `input` and resample it into `output`.
	 *
	 * @param input The interleaved audio input.
	 * @param output The interleaved audio output.
	 *
	 * @throws `std::invalid_argument` if a channel mismatch has been detected.
	 * @throws `std::runtime_error` if the resampling process has failed.
	 *
	 * @remark This utility class does not cache the input and output buffers, making it stateless. In other words,
	 * `input` is directly resampled into the `output`.
	 *
	 * @returns the result of the resampling process. See @ref Result for more details.
	 */
	[[nodiscard]] auto process(InterleavedBufferView<const float> input, InterleavedBufferView<float> output) -> Result;

	/**
	 * @brief Resets the internal resampler state.
	 * Useful when working with unreleated pieces of audio.
	 */
	void reset();

	/**
	 * @brief Sets the resampling ratio to `ratio`.
	 * @param ratio Output sample rate divided by input sample rate.
	 */
	void setRatio(double ratio) { m_ratio = ratio; }

	/**
	 * @brief Sets the resampling ratio to `output / input`.
	 * @param input Input sample rate.
	 * @param output Output sample rate.
	 */
	void setRatio(sample_rate_t input, sample_rate_t output) { m_ratio = static_cast<double>(output) / input; }

	//! @returns the resampling ratio.
	auto ratio() const -> double { return m_ratio; }

	//! @returns the number of channels expected by the resampler.
	auto channels() const -> ch_cnt_t { return m_channels; }

	//! @returns the interpolation mode used by this resampler.
	auto mode() const -> Mode { return m_mode; }

private:
	struct LMMS_EXPORT StateDeleter { void operator()(void* state); };
	std::unique_ptr<void, StateDeleter> m_state;
	Mode m_mode;
	ch_cnt_t m_channels = 0;
	double m_ratio = 1.0;
	int m_error = 0;
};

} // namespace lmms

#endif // LMMS_AUDIO_RESAMPLER_H
