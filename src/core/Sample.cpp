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

Sample::Sample(const sampleFrame* data, int numFrames, int sampleRate)
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

bool Sample::play(sampleFrame* dst, PlaybackState* state, int numFrames, float desiredFrequency, Loop loopMode)
{
	if (loopMode == Loop::Off && state->m_frameIndex >= m_endFrame) { return false; }

	const auto outputSampleRate = Engine::audioEngine()->processingSampleRate() * m_frequency / desiredFrequency;
	const auto inputSampleRate = m_buffer->sampleRate();
	const auto resampleRatio = outputSampleRate / inputSampleRate;
	const auto marginSize = s_interpolationMargins[state->resampler().interpolationMode()];

	auto playBuffer = std::vector<sampleFrame>(numFrames / resampleRatio + marginSize);
	switch (loopMode)
	{
	case Loop::Off: {
		const auto framesToPlay = state->m_backwards
			? std::min<int>(playBuffer.size(), state->m_frameIndex)
			: std::min<int>(playBuffer.size(), m_buffer->size() - state->m_frameIndex);
		playSampleRange(playBuffer.data(), framesToPlay, state->m_frameIndex, state->m_backwards);
		break;
	}
	case Loop::On: {
		auto loopIndex = state->m_frameIndex;
		for (auto& frame : playBuffer)
		{
			if (loopIndex < m_loopStartFrame && state->m_backwards) { loopIndex = m_loopEndFrame - 1; }
			else if (loopIndex >= m_loopEndFrame) { loopIndex = m_loopStartFrame; }
			playSampleRange(&frame, 1, state->m_backwards ? loopIndex-- : loopIndex++, state->m_backwards);
		}
		break;
	}
	case Loop::PingPong: {
		auto loopIndex = state->m_frameIndex;
		auto backwards = state->m_backwards;
		for (auto& frame : playBuffer)
		{
			if (loopIndex < m_loopStartFrame && backwards)
			{
				loopIndex = m_loopStartFrame;
				backwards = false;
			}
			else if (loopIndex >= m_loopEndFrame)
			{
				loopIndex = m_loopEndFrame - 1;
				backwards = true;
			}
			playSampleRange(&frame, 1, backwards ? loopIndex-- : loopIndex++, backwards);
		}
		break;
	}
	}

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
		amplifySampleRange(dst, resampleResult.outputFramesGenerated);
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

void Sample::advance(PlaybackState* state, size_t advanceAmount, Loop loopMode)
{
	switch (loopMode)
	{
	case Loop::Off:
		state->m_frameIndex += (state->m_backwards ? -1 : 1) * advanceAmount;
		break;
	case Loop::On:
		if (state->m_backwards)
		{
			state->m_frameIndex -= advanceAmount;
			if (state->m_frameIndex < m_loopStartFrame)
			{
				state->m_frameIndex
					= m_loopEndFrame - 1 - std::abs(state->m_frameIndex) % (m_loopEndFrame - m_loopStartFrame);
			}
		}
		else
		{
			state->m_frameIndex += advanceAmount;
			if (state->m_frameIndex >= m_loopEndFrame)
			{
				state->m_frameIndex
					= m_loopStartFrame + std::abs(state->m_frameIndex) % (m_loopEndFrame - m_loopStartFrame);
			}
		}
		break;
	case Loop::PingPong:
		if (state->m_backwards)
		{
			state->m_frameIndex -= advanceAmount;
			if (state->m_frameIndex < m_loopStartFrame)
			{
				state->m_frameIndex
					= m_loopStartFrame + std::abs(state->m_frameIndex) % (m_loopEndFrame - m_loopStartFrame);
				state->m_backwards = false;
			}
		}
		else
		{
			state->m_frameIndex += advanceAmount;
			if (state->m_frameIndex >= m_loopEndFrame)
			{
				state->m_frameIndex
					= m_loopEndFrame - 1 - state->m_frameIndex % (m_loopEndFrame - m_loopStartFrame);
				state->m_backwards = true;
			}
		}
		break;
	}
}

void Sample::playSampleRange(sampleFrame* dst, size_t numFrames, int index, bool backwards) const
{
	backwards ? copyBufferBackward(dst, index, numFrames) : copyBufferForward(dst, index, numFrames);
}

void Sample::copyBufferForward(sampleFrame* dst, size_t initialPosition, size_t advanceAmount) const
{
	m_reversed ? std::copy_n(m_buffer->rbegin() + initialPosition, advanceAmount, dst)
			   : std::copy_n(m_buffer->begin() + initialPosition, advanceAmount, dst);
}

void Sample::copyBufferBackward(sampleFrame* dst, size_t initialPosition, size_t advanceAmount) const
{
	m_reversed ? std::copy_n(m_buffer->begin() + initialPosition, advanceAmount, dst)
			   : std::copy_n(m_buffer->rbegin() + initialPosition, advanceAmount, dst);
}

void Sample::amplifySampleRange(sampleFrame* src, int numFrames) const
{
	const auto amplification = m_amplification.load(std::memory_order_relaxed);
	for (int i = 0; i < numFrames; ++i)
	{
		src[i][0] *= amplification;
		src[i][1] *= amplification;
	}
}
} // namespace lmms
