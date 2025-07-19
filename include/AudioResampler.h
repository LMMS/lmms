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
#include <stdexcept>
#include <vector>

#include "AudioBufferView.h"
#include "lmms_export.h"

namespace lmms {

/**
 * @class AudioResampler
 * @brief A utility for resampling interleaved audio buffers using various resampling algorithms.
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

	//! @brief Destructor. Releases any internal state or buffers.
	~AudioResampler();

	//! @brief Cannot copy.
	AudioResampler(const AudioResampler&) = delete;

	//! @brief Cannot copy.
	AudioResampler& operator=(const AudioResampler&) = delete;

	//! @brief Moves the internal state from one resampler to another.
	AudioResampler(AudioResampler&&) noexcept;

	//! @brief Moves the internal state from one resampler to another.
	AudioResampler& operator=(AudioResampler&&) noexcept;

	/**
	 * @brief Processes a block of interleaved audio input and writes resampled output.
	 *
	 * @param input Interleaved input buffer view of type `InterleavedBufferView<const float>`.
	 * @param output Interleaved output buffer view of type `InterleavedBufferView<float>`.
	 * @return Result containing input/output frame counts.
	 */
	[[nodiscard]] auto process(InterleavedBufferView<const float> input, InterleavedBufferView<float> output) -> Result;

	/**
	 * @brief Processes resampling using a pull-based input provider.
	 *
	 * This variant uses a callable `input` source that accepts an output buffer view and returns
	 * the number of frames it filled.
	 *
	 * @tparam InterleavedBufferSource as the callable type.
	 * @param input Function providing input audio frames on demand.
	 * @param output Destination buffer for resampled frames of type `InterleavedBufferView<float>`.
	 * @return Total number of output frames written.
	 * @throws `std::runtime_error` if channel mismatch is detected.
	 */
	template <typename InterleavedBufferSource>
		requires(std::is_invocable_r_v<std::size_t, InterleavedBufferSource, InterleavedBufferView<float>>)
	[[nodiscard]] auto process(InterleavedBufferSource input, InterleavedBufferView<float> output) -> f_cnt_t
	{
		if (output.channels() != m_channels) { throw std::runtime_error{"Invalid channel count"}; }

		auto outputFramesGenerated = std::size_t{0};
		while (!output.empty())
		{
			if (m_bufferWindow.empty())
			{
				const auto rendered = input({m_buffer.data(), m_channels, m_buffer.size() / m_channels});
				m_bufferWindow = {m_buffer.data(), m_channels, rendered};
			}

			const auto result = process(m_bufferWindow, output);
			if (result.inputFramesUsed == 0 && result.outputFramesGenerated == 0) { break; }

			m_bufferWindow
				= m_bufferWindow.subspan(result.inputFramesUsed, m_bufferWindow.frames() - result.inputFramesUsed);
			output = output.subspan(result.outputFramesGenerated, output.frames() - result.outputFramesGenerated);
			outputFramesGenerated += result.outputFramesGenerated;
		}

		return outputFramesGenerated;
	}

	/**
	 * @brief Resets the internal resampler state.
	 *
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

	//! @return The resampling ratio.
	auto ratio() const -> double { return m_ratio; }

	//! @return The number of channels expected by the resampler.
	auto channels() const -> ch_cnt_t { return m_channels; }

	//! @return The interpolation mode used by this resampler.
	auto mode() const -> Mode { return m_mode; }

private:
	static auto converterType(Mode mode) -> int;
	static constexpr auto DefaultFrames = 16;
	std::vector<float> m_buffer;
	InterleavedBufferView<float> m_bufferWindow;
	SRC_STATE* m_state = nullptr;
	Mode m_mode;
	ch_cnt_t m_channels = 0;
	double m_ratio = 1.0;
	int m_error = 0;
};

} // namespace lmms

#endif // LMMS_AUDIO_RESAMPLER_H
