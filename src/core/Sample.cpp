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

#include <cassert>

#include "MixHelpers.h"
#include "interpolation.h"

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

bool Sample::play(sampleFrame* dst, PlaybackState* state, size_t numFrames, double frequency, Loop loopMode) const
{
	assert(numFrames > 0);
	assert(frequency > 0);

	const auto pastBounds = state->frameIndex >= m_endFrame || (state->frameIndex < 0 && state->backwards);
	if (m_buffer->empty() || (loopMode == Loop::Off && pastBounds)) { return false; }

	const auto outputSampleRate = Engine::audioEngine()->outputSampleRate() * m_frequency / frequency;
	const auto inputSampleRate = m_buffer->sampleRate();
	const auto resampleRatio = outputSampleRate / inputSampleRate;

	state->frameIndex = std::max<float>(m_startFrame, state->frameIndex);
	render(dst, numFrames, state, loopMode, resampleRatio);

	if (m_amplification != 1.0f)
	{
		MixHelpers::multiply(dst, m_amplification, numFrames);
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

void Sample::render(sampleFrame* dst, size_t numFrames, PlaybackState* state, Loop loopMode, double resampleRatio) const
{
	for (size_t i = 0; i < numFrames; ++i)
	{
		switch (loopMode)
		{
		case Loop::Off:
			if (state->frameIndex < 0 || state->frameIndex >= m_endFrame) { return; }
			break;
		case Loop::On:
			if (state->frameIndex < m_loopStartFrame && state->backwards)
			{
				state->frameIndex = m_loopEndFrame - 1;
			}
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

		const auto srcIndex = static_cast<int>(state->frameIndex);
		const auto leftX1Index = srcIndex * DEFAULT_CHANNELS;
		const auto rightX1Index = leftX1Index + 1;

		const auto leftX0Index = leftX1Index - DEFAULT_CHANNELS;
		const auto leftX2Index = leftX1Index + DEFAULT_CHANNELS;
		const auto leftX3Index = leftX1Index + DEFAULT_CHANNELS * 2;

		const auto rightX0Index = rightX1Index - DEFAULT_CHANNELS;
		const auto rightX2Index = rightX1Index + DEFAULT_CHANNELS;
		const auto rightX3Index = rightX1Index + DEFAULT_CHANNELS * 2;

		const auto src = m_buffer->data()->data();
		const auto srcSize = m_buffer->size() * DEFAULT_CHANNELS;

		const auto leftX0 = (leftX0Index < 0 || leftX0Index >= srcSize) ? 0.0f : src[leftX0Index];
		const auto leftX1 = (leftX1Index < 0 || leftX1Index >= srcSize) ? 0.0f : src[leftX1Index];
		const auto leftX2 = (leftX2Index < 0 || leftX2Index >= srcSize) ? 0.0f : src[leftX2Index];
		const auto leftX3 = (leftX3Index < 0 || leftX3Index >= srcSize) ? 0.0f : src[leftX3Index];

		const auto rightX0 = (rightX0Index < 0 || rightX0Index >= srcSize) ? 0.0f : src[rightX0Index];
		const auto rightX1 = (rightX1Index < 0 || rightX1Index >= srcSize) ? 0.0f : src[rightX1Index];
		const auto rightX2 = (rightX2Index < 0 || rightX2Index >= srcSize) ? 0.0f : src[rightX2Index];
		const auto rightX3 = (rightX3Index < 0 || rightX3Index >= srcSize) ? 0.0f : src[rightX3Index];

		const auto fractionalPosition = state->frameIndex - srcIndex;
		const auto leftSample = hermiteInterpolate(leftX0, leftX1, leftX2, leftX3, fractionalPosition);
		const auto rightSample = hermiteInterpolate(rightX0, rightX1, rightX2, rightX3, fractionalPosition);

		dst[i] = {leftSample, rightSample};
		state->frameIndex += (state->backwards ? -1.0 : 1.0)  / resampleRatio;
	}
}

} // namespace lmms
