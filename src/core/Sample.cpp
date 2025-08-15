/*
 * Sample.cpp
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

#include "Sample.h"

namespace lmms {

Sample::Sample(const QString& audioFile)
	: m_buffer(std::make_shared<SampleBuffer>(audioFile))
	, m_startFrame(0)
	, m_endFrame(m_buffer->size())
	, m_loopStartFrame(0)
	, m_loopEndFrame(m_buffer->size())
{
}

Sample::Sample(const QByteArray& base64, int sampleRate)
	: m_buffer(std::make_shared<SampleBuffer>(base64, sampleRate))
	, m_startFrame(0)
	, m_endFrame(m_buffer->size())
	, m_loopStartFrame(0)
	, m_loopEndFrame(m_buffer->size())
{
}

Sample::Sample(const SampleFrame* data, size_t numFrames, int sampleRate)
	: m_buffer(std::make_shared<SampleBuffer>(data, numFrames, sampleRate))
	, m_startFrame(0)
	, m_endFrame(m_buffer->size())
	, m_loopStartFrame(0)
	, m_loopEndFrame(m_buffer->size())
{
}

Sample::Sample(std::shared_ptr<const SampleBuffer> buffer)
	: m_buffer(buffer)
	, m_startFrame(0)
	, m_endFrame(m_buffer->size())
	, m_loopStartFrame(0)
	, m_loopEndFrame(m_buffer->size())
{
}

Sample::Sample(const Sample& other)
	: m_buffer(other.m_buffer)
	, m_startFrame(other.startFrame())
	, m_endFrame(other.endFrame())
	, m_loopStartFrame(other.loopStartFrame())
	, m_loopEndFrame(other.loopEndFrame())
	, m_amplification(other.amplification())
	, m_frequency(other.frequency())
	, m_reversed(other.reversed())
{
}

Sample::Sample(Sample&& other) noexcept
	: m_buffer(std::move(other.m_buffer))
	, m_startFrame(other.startFrame())
	, m_endFrame(other.endFrame())
	, m_loopStartFrame(other.loopStartFrame())
	, m_loopEndFrame(other.loopEndFrame())
	, m_amplification(other.amplification())
	, m_frequency(other.frequency())
	, m_reversed(other.reversed())
{
}

auto Sample::operator=(const Sample& other) -> Sample&
{
	m_buffer = other.m_buffer;
	m_startFrame = other.startFrame();
	m_endFrame = other.endFrame();
	m_loopStartFrame = other.loopStartFrame();
	m_loopEndFrame = other.loopEndFrame();
	m_amplification = other.amplification();
	m_frequency = other.frequency();
	m_reversed = other.reversed();

	return *this;
}

auto Sample::operator=(Sample&& other) noexcept -> Sample&
{
	m_buffer = std::move(other.m_buffer);
	m_startFrame = other.startFrame();
	m_endFrame = other.endFrame();
	m_loopStartFrame = other.loopStartFrame();
	m_loopEndFrame = other.loopEndFrame();
	m_amplification = other.amplification();
	m_frequency = other.frequency();
	m_reversed = other.reversed();

	return *this;
}

bool Sample::play(SampleFrame* dst, PlaybackState* state, size_t numFrames, Loop loop, double ratio) const
{
	state->m_frameIndex = std::max<int>(m_startFrame, state->m_frameIndex);

	const auto sampleRateRatio = static_cast<double>(Engine::audioEngine()->outputSampleRate()) / m_buffer->sampleRate();
	const auto freqRatio = frequency() / DefaultBaseFreq;
	state->m_resampler.setRatio(sampleRateRatio * freqRatio * ratio);

	// TODO: Add an abstraction for these audio pipeline workflows
	auto framesWritten = 0;
	while (numFrames > 0)
	{
		if (state->m_window.empty())
		{
			const auto rendered = render(state->m_buffer.data(), state->m_buffer.size(), state, loop);
			if (rendered == 0) { break; }
			state->m_window = {state->m_buffer.data(), rendered};
		}

		const auto result = state->m_resampler.process(
			{&state->m_window[0][0], 2, state->m_window.size()}, {&dst[0][0], 2, numFrames});

		if (result.outputFramesGenerated == 0) { break; }

		state->m_window = state->m_window.subspan(result.inputFramesUsed, state->m_window.size() - result.inputFramesUsed);
		dst += result.outputFramesGenerated;
		numFrames -= result.outputFramesGenerated;
		framesWritten += result.outputFramesGenerated;
	}

	return framesWritten > 0;
}

f_cnt_t Sample::render(SampleFrame* dst, f_cnt_t size, PlaybackState* state, Loop loop) const
{
	for (f_cnt_t frame = 0; frame < size; ++frame)
	{
		switch (loop)
		{
		case Loop::Off:
			if (state->m_frameIndex < 0 || state->m_frameIndex >= m_endFrame) { return frame; }
			break;
		case Loop::On:
			if (state->m_frameIndex < m_loopStartFrame && state->m_backwards)
			{
				state->m_frameIndex = m_loopEndFrame - 1;
			}
			else if (state->m_frameIndex >= m_loopEndFrame) { state->m_frameIndex = m_loopStartFrame; }
			break;
		case Loop::PingPong:
			if (state->m_frameIndex < m_loopStartFrame && state->m_backwards)
			{
				state->m_frameIndex = m_loopStartFrame;
				state->m_backwards = false;
			}
			else if (state->m_frameIndex >= m_loopEndFrame)
			{
				state->m_frameIndex = m_loopEndFrame - 1;
				state->m_backwards = true;
			}
			break;
		default:
			break;
		}

		const auto value
			= m_buffer->data()[m_reversed ? m_buffer->size() - state->m_frameIndex - 1 : state->m_frameIndex]
			* m_amplification;
		dst[frame] = value;
		state->m_backwards ? --state->m_frameIndex : ++state->m_frameIndex;
	}

	return size;
}

auto Sample::sampleDuration() const -> std::chrono::milliseconds
{
	const auto numFrames = endFrame() - startFrame();
	const auto duration = numFrames / static_cast<float>(m_buffer->sampleRate()) * 1000;
	return std::chrono::milliseconds{static_cast<int>(duration)};
}

void Sample::setAllPointFrames(int startFrame, int endFrame, int loopStartFrame, int loopEndFrame)
{
	setStartFrame(startFrame);
	setEndFrame(endFrame);
	setLoopStartFrame(loopStartFrame);
	setLoopEndFrame(loopEndFrame);
}

} // namespace lmms
