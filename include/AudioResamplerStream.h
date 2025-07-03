/*
 * AudioResamplerStream.h
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

#ifndef LMMS_AUDIO_RESAMPLER_STREAM_H
#define LMMS_AUDIO_RESAMPLER_STREAM_H

#include <algorithm>
#include <stdexcept>
#include <vector>

#include "AudioBufferView.h"
#include "AudioResampler.h"

namespace lmms {
class AudioResamplerStream
{
public:
	//! The resampling results returned from calls to @ref resample.
	struct Result
	{
		f_cnt_t inputFramesGenerated;
		f_cnt_t inputFramesUsed;
		f_cnt_t outputFramesGenerated;
		f_cnt_t outputFramesConsumed;
	};

	//! Specifies how the resampled output should be written.
	enum class WriteMode
	{
		Copy, //! The resampled output will be copied.
		Mix	  //! The resampled output will be mixed.
	};

	//! Create an audio resampler stream with the given @p mode and channel count @p channels .
	//! Assumes stereo audio by default.
	AudioResamplerStream(AudioResampler::Mode mode, ch_cnt_t channels = DEFAULT_CHANNELS)
		: m_resampler(mode, channels)
		, m_inputBuffer(bufferSize(channels))
		, m_outputBuffer(bufferSize(channels))
		, m_inputWindow({m_inputBuffer.data(), channels, 0})
		, m_outputWindow({m_outputBuffer.data(), channels, m_outputBuffer.size() / channels})
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
			if (m_inputWindow.empty())
			{
				const auto rendered = input(
					{m_inputBuffer.data(), m_resampler.channels(), m_inputBuffer.size() / m_resampler.channels()});

				resampleResult.inputFramesGenerated += rendered;
				m_inputWindow = {m_inputBuffer.data(), m_resampler.channels(), rendered};
			}

			if (m_outputWindow.empty())
			{
				m_outputWindow
					= {m_outputBuffer.data(), m_resampler.channels(), m_outputBuffer.size() / m_resampler.channels()};
			}

			const auto result = m_resampler.process(m_inputWindow, m_outputWindow);
			const auto outputFrames = std::min(result.outputFramesGenerated, output.frames());

			if (m_inputWindow.empty() && outputFrames == 0) { break; }

			if constexpr (writeMode == WriteMode::Copy)
			{
				std::copy_n(m_outputWindow.data(), outputFrames * m_outputWindow.channels(), output.data());
			}
			else if constexpr (writeMode == WriteMode::Mix)
			{
				std::transform(m_outputWindow.data(), m_outputWindow.data() + outputFrames * m_outputWindow.channels(),
					output.data(), output.data(), std::plus{});
			}

			m_inputWindow = {m_inputWindow.data() + result.inputFramesUsed * m_resampler.channels(),
				m_resampler.channels(), m_inputWindow.frames() - result.inputFramesUsed};

			m_outputWindow = {m_outputWindow.data() + outputFrames * m_resampler.channels(), m_resampler.channels(),
				m_outputWindow.frames() - outputFrames};

			output = {output.data() + outputFrames * m_resampler.channels(), m_resampler.channels(),
				output.frames() - outputFrames};

			resampleResult.inputFramesUsed += result.inputFramesUsed;
			resampleResult.outputFramesGenerated += result.outputFramesGenerated;
			resampleResult.outputFramesConsumed += outputFrames;
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

	AudioResampler m_resampler;
	std::vector<float> m_inputBuffer;
	std::vector<float> m_outputBuffer;
	InterleavedBufferView<float> m_inputWindow;
	InterleavedBufferView<float> m_outputWindow;
};
} // namespace lmms

#endif // LMMS_AUDIO_RESAMPLER_STREAM_H