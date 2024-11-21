/*
 * Sample.cpp - State for container-class SampleBuffer
 *
 * Copyright (c) 2023 saker <sakertooth@gmail.com>
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

#include "lmms_math.h"

#include <cassert>

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

Sample::Sample(Sample&& other)
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

auto Sample::operator=(Sample&& other) -> Sample&
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

bool Sample::play(SampleFrame* dst, PlaybackState* state, size_t numFrames, float desiredFrequency, Loop loopMode) const
{
	assert(numFrames > 0);
	assert(desiredFrequency > 0);

	const auto pastBounds = state->m_frameIndex >= m_endFrame || (state->m_frameIndex < 0 && state->m_backwards);
	if (loopMode == Loop::Off && pastBounds) { return false; }

	const auto outputSampleRate = Engine::audioEngine()->outputSampleRate() * m_frequency / desiredFrequency;
	const auto inputSampleRate = m_buffer->sampleRate();
	const auto resampleRatio = outputSampleRate / inputSampleRate;
	const auto marginSize = s_interpolationMargins[state->resampler().interpolationMode()];

	state->m_frameIndex = std::max<int>(m_startFrame, state->m_frameIndex);

	auto playBuffer = std::vector<SampleFrame>(numFrames / resampleRatio + marginSize);
	playRaw(playBuffer.data(), playBuffer.size(), state, loopMode);

	state->resampler().setRatio(resampleRatio);

	const auto resampleResult
		= state->resampler().resample(&playBuffer[0][0], playBuffer.size(), &dst[0][0], numFrames, resampleRatio);
	advance(state, resampleResult.inputFramesUsed, loopMode);

	const auto outputFrames = static_cast<f_cnt_t>(resampleResult.outputFramesGenerated);
	if (outputFrames < numFrames) { std::fill_n(dst + outputFrames, numFrames - outputFrames, SampleFrame{}); }

	if (!approximatelyEqual(m_amplification, 1.0f))
	{
		for (auto i = std::size_t{0}; i < numFrames; ++i)
		{
			dst[i][0] *= m_amplification;
			dst[i][1] *= m_amplification;
		}
	}

	return true;
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

void Sample::playRaw(SampleFrame* dst, size_t numFrames, const PlaybackState* state, Loop loopMode) const
{
	if (m_buffer->size() < 1) { return; }

	auto index = state->m_frameIndex;
	auto backwards = state->m_backwards;

	for (size_t i = 0; i < numFrames; ++i)
	{
		switch (loopMode)
		{
		case Loop::Off:
			if (index < 0 || index >= m_endFrame) { return; }
			break;
		case Loop::On:
			if (index < m_loopStartFrame && backwards) { index = m_loopEndFrame - 1; }
			else if (index >= m_loopEndFrame) { index = m_loopStartFrame; }
			break;
		case Loop::PingPong:
			if (index < m_loopStartFrame && backwards)
			{
				index = m_loopStartFrame;
				backwards = false;
			}
			else if (index >= m_loopEndFrame)
			{
				index = m_loopEndFrame - 1;
				backwards = true;
			}
			break;
		default:
			break;
		}

		dst[i] = m_buffer->data()[m_reversed ? m_buffer->size() - index - 1 : index];
		backwards ? --index : ++index;
	}
}

void Sample::advance(PlaybackState* state, size_t advanceAmount, Loop loopMode) const
{
	state->m_frameIndex += (state->m_backwards ? -1 : 1) * advanceAmount;
	if (loopMode == Loop::Off) { return; }

	const auto distanceFromLoopStart = std::abs(state->m_frameIndex - m_loopStartFrame);
	const auto distanceFromLoopEnd = std::abs(state->m_frameIndex - m_loopEndFrame);
	const auto loopSize = m_loopEndFrame - m_loopStartFrame;
	if (loopSize == 0) { return; }

	switch (loopMode)
	{
	case Loop::On:
		if (state->m_frameIndex < m_loopStartFrame && state->m_backwards)
		{
			state->m_frameIndex = m_loopEndFrame - 1 - distanceFromLoopStart % loopSize;
		}
		else if (state->m_frameIndex >= m_loopEndFrame)
		{
			state->m_frameIndex = m_loopStartFrame + distanceFromLoopEnd % loopSize;
		}
		break;
	case Loop::PingPong:
		if (state->m_frameIndex < m_loopStartFrame && state->m_backwards)
		{
			state->m_frameIndex = m_loopStartFrame + distanceFromLoopStart % loopSize;
			state->m_backwards = false;
		}
		else if (state->m_frameIndex >= m_loopEndFrame)
		{
			state->m_frameIndex = m_loopEndFrame - 1 - distanceFromLoopEnd % loopSize;
			state->m_backwards = true;
		}
		break;
	default:
		break;
	}
}

} // namespace lmms
