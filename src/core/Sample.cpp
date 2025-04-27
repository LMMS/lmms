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

// TODO: This function can take in the source buffer as a parameter, while the shared ownership
// of that buffer should belong to the caller.
bool Sample::play(SampleFrame* dst, PlaybackState* state, size_t numFrames, Loop loop, double ratio) const
{
	ratio *= static_cast<double>(Engine::audioEngine()->outputSampleRate()) / m_buffer->sampleRate();
	std::fill_n(dst, numFrames, SampleFrame{});
	state->frameIndex = std::max<int>(m_startFrame, state->frameIndex);

	while (numFrames > 0)
	{
		const auto resamplerInputView = state->resampler.inputWriterView();
		const auto numFramesRendered = render(resamplerInputView.data(), resamplerInputView.size(), state, loop);
		state->resampler.commitInputWrite(static_cast<long>(numFramesRendered));

		if (!state->resampler.resample(ratio) && numFramesRendered == 0) { return false; }

		const auto resamplerOutputView = state->resampler.outputReaderView();
		const auto outputFramesToRead = std::min(numFrames, resamplerOutputView.size());
		std::copy_n(resamplerOutputView.begin(), outputFramesToRead, dst);
		state->resampler.commitOutputRead(static_cast<long>(outputFramesToRead));

		dst += outputFramesToRead;
		numFrames -= outputFramesToRead;
	}

	return true;
}

std::size_t Sample::render(SampleFrame* dst, std::size_t size, PlaybackState* state, Loop loop) const
{
	for (std::size_t frame = 0; frame < size; ++frame)
	{
		switch (loop)
		{
		case Loop::Off:
			if (state->frameIndex < 0 || state->frameIndex >= m_endFrame) { return frame; }
			break;
		case Loop::On:
			if (state->frameIndex < m_loopStartFrame && state->backwards) { state->frameIndex = m_loopEndFrame - 1; }
			else if (state->frameIndex >= m_loopEndFrame) { state->frameIndex = m_loopStartFrame; }
			break;
		case Loop::PingPong:
			if (state->frameIndex < m_loopStartFrame && state->backwards)
			{
				state->frameIndex = m_loopStartFrame;
				state->backwards = false;
			}
			else if (state->frameIndex >= m_loopEndFrame)
			{
				state->frameIndex = m_loopEndFrame - 1;
				state->backwards = true;
			}
			break;
		default:
			break;
		}

		const auto value = m_buffer->data()[m_reversed ? m_buffer->size() - state->frameIndex - 1 : state->frameIndex];
		dst[frame] = value * m_amplification;
		state->backwards ? --state->frameIndex : ++state->frameIndex;
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
