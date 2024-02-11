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

Sample::Sample(const sampleFrame* data, size_t numFrames, int sampleRate)
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

bool Sample::play(sampleFrame* dst, PlaybackState* state, size_t numFrames, float desiredFrequency, Loop loopMode)
{
	if (loopMode == Loop::Off && (state->m_frameIndex >= m_endFrame || (state->m_frameIndex < 0 && state->m_backwards)))
	{
		return false;
	}

	const auto outputSampleRate = Engine::audioEngine()->processingSampleRate() * m_frequency / desiredFrequency;
	const auto inputSampleRate = m_buffer->sampleRate();
	const auto resampleRatio = outputSampleRate / inputSampleRate;
	const auto marginSize = s_interpolationMargins[state->resampler().interpolationMode()];

	auto playBuffer = std::vector<sampleFrame>(numFrames / resampleRatio + marginSize);
	playRaw(playBuffer.data(), playBuffer.size(), state, loopMode);

	const auto resampleResult
		= state->resampler().resample(&playBuffer[0][0], playBuffer.size(), &dst[0][0], numFrames, resampleRatio);
	advance(state, resampleResult.inputFramesUsed, loopMode);

	if (resampleResult.outputFramesGenerated < numFrames)
	{
		std::fill(dst + resampleResult.outputFramesGenerated, dst + resampleResult.outputFramesGenerated + numFrames,
			sampleFrame{0, 0});
	}

	if (!typeInfo<float>::isEqual(m_amplification, 1.0f))
	{
		for (int i = 0; i < numFrames; ++i)
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

void Sample::playRaw(sampleFrame* dst, size_t numFrames, const PlaybackState* state, Loop loopMode) const
{
	auto index = state->m_frameIndex;
	auto backwards = state->m_backwards;

	for (size_t i = 0; i < numFrames; ++i)
	{
		switch (loopMode)
		{
		case Loop::Off:
			if (index > m_endFrame || (index < 0 && state->m_backwards)) { return; }
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

		auto playIndex = backwards ? index-- : index++;
		if (m_reversed) { playIndex = m_buffer->size() - playIndex; }
		dst[i] = m_buffer->data()[playIndex];
	}
}

void Sample::advance(PlaybackState* state, size_t advanceAmount, Loop loopMode) const
{
	const auto distanceAfterLoopEnd = std::abs(state->m_frameIndex - m_loopEndFrame);
	const auto distanceAfterLoopStart = std::abs(state->m_frameIndex - m_loopStartFrame);

	switch (loopMode)
	{
	case Loop::On:
		if (state->m_frameIndex < m_loopStartFrame && state->m_backwards)
		{
			state->m_frameIndex = m_loopEndFrame - 1 - distanceAfterLoopStart % (m_loopEndFrame - m_loopStartFrame);
		}
		else if (state->m_frameIndex >= m_loopEndFrame)
		{
			state->m_frameIndex = m_loopStartFrame + distanceAfterLoopEnd % (m_loopEndFrame - m_loopStartFrame);
		}
		break;
	case Loop::PingPong:
		if (state->m_frameIndex < m_loopStartFrame && state->m_backwards)
		{
			state->m_frameIndex = m_loopStartFrame + distanceAfterLoopStart % (m_loopEndFrame - m_loopStartFrame);
			state->m_backwards = false;
		}
		else if (state->m_frameIndex >= m_loopEndFrame)
		{
			state->m_frameIndex = m_loopEndFrame - 1 - distanceAfterLoopEnd % (m_loopEndFrame - m_loopStartFrame);
			state->m_backwards = true;
		}
		break;
	default:
		break;
	}

	state->m_frameIndex += (state->m_backwards ? -1 : 1) * advanceAmount;
}

} // namespace lmms
