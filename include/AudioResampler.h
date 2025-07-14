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

#include <algorithm>
#include <samplerate.h>
#include <stdexcept>
#include <vector>

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

	//! A class with additional functionality on top of a resampler to meant for resampling audio streams.
	//! Audio streams are simply functions that output audio. Usually, clients that want to resample audio streams have
	//! to handle a bit of buffer logic to do so. This class handles those use cases.
	class Stream;

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

class AudioResampler::Stream
{
public:
	//! The resampling results returned from calls to @ref resample.
	struct Result
	{
		f_cnt_t inputFramesGenerated;
		f_cnt_t outputFramesWritten;
	};

	//! Specifies how the resampled output should be written.
	enum class WriteMode
	{
		Copy, //! The resampled output will be copied.
		Mix	  //! The resampled output will be mixed.
	};

	//! Create an audio resampler stream with the given @p mode and channel count @p channels .
	//! Assumes stereo audio by default.
	Stream(Mode mode, ch_cnt_t channels = DEFAULT_CHANNELS)
		: m_resampler(mode, channels)
		, m_inputBuffer(bufferSize(channels))
		, m_outputBuffer(bufferSize(channels))
		, m_inputWindow({m_inputBuffer.data(), channels, 0})
		, m_outputWindow({m_outputBuffer.data(), channels, 0})
	{
	}

	//! Resample audio from the @p input into the @p output at the assigned resampling ratio .
	//! If @p writeMode is set to `WriteMode::Copy`, then the resampled output will be copied directly into @p output .
	//! If @p writeMode is set to `WriteMode::Mix`, then the resampled output will be mixed additively into @p output .
	template <WriteMode writeMode, typename Input>
	auto resample(Input input, InterleavedBufferView<float> output) -> Result
	{
		if (output.channels() != m_resampler.channels()) { throw std::invalid_argument{"Invalid channel count"}; }

		auto resampleResult = Result{};
		while (!output.empty())
		{
			if (!m_outputWindow.empty())
			{
				const auto numFramesToWrite = std::min(m_outputWindow.frames(), output.frames());

				if constexpr (writeMode == WriteMode::Copy)
				{
					std::copy_n(m_outputWindow.data(), numFramesToWrite * m_resampler.channels(), output.data());
				}
				else if constexpr (writeMode == WriteMode::Mix)
				{
					std::transform(m_outputWindow.data(),
						m_outputWindow.data() + numFramesToWrite * m_resampler.channels(), output.data(), output.data(),
						std::plus{});
				}

				m_outputWindow = m_outputWindow.subspan(numFramesToWrite, m_outputWindow.frames() - numFramesToWrite);
				output = output.subspan(numFramesToWrite, output.frames() - numFramesToWrite);
				resampleResult.outputFramesWritten += numFramesToWrite;
			}
			else if (!m_inputWindow.empty())
			{
				const auto result = m_resampler.process(m_inputWindow, outputBufferView());
				m_inputWindow
					= m_inputWindow.subspan(result.inputFramesUsed, m_inputWindow.frames() - result.inputFramesUsed);
				m_outputWindow = outputBufferView().subspan(0, result.outputFramesGenerated);
			}
			else
			{
				const auto rendered = input(inputBufferView());
				if (rendered == 0) { break; }

				m_inputWindow = inputBufferView().subspan(0, rendered);
				resampleResult.inputFramesGenerated += rendered;
			}
		}

		return resampleResult;
	}

	//! See @ref `AudioResampler::ratio`.
	auto ratio() const -> double { return m_resampler.ratio(); }

	//! See @ref `AudioResampler::setRatio`.
	void setRatio(double ratio) { m_resampler.setRatio(ratio); }

	//! See @ref `AudioResampler::setRatio`.
	void setRatio(sample_rate_t input, sample_rate_t output) { m_resampler.setRatio(input, output); }

private:
	static constexpr auto bufferSize(ch_cnt_t channels) -> std::size_t
	{
		constexpr auto BufferSize = 64 / sizeof(float);
		return std::max(BufferSize, static_cast<std::size_t>(channels));
	}

	auto inputBufferView() -> InterleavedBufferView<float>
	{
		return {m_inputBuffer.data(), m_resampler.channels(), m_inputBuffer.size() / m_resampler.channels()};
	}

	auto outputBufferView() -> InterleavedBufferView<float>
	{
		return {m_outputBuffer.data(), m_resampler.channels(), m_outputBuffer.size() / m_resampler.channels()};
	}

	AudioResampler m_resampler;
	std::vector<float> m_inputBuffer;
	std::vector<float> m_outputBuffer;
	InterleavedBufferView<float> m_inputWindow;
	InterleavedBufferView<float> m_outputWindow;
};

} // namespace lmms

#endif // LMMS_AUDIO_RESAMPLER_H
