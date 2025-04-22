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

	const auto pastBounds = state->frameIndex >= m_endFrame || (state->frameIndex < 0 && state->backwards);
	if (loopMode == Loop::Off && pastBounds) { return false; }

	const auto outputSampleRate = Engine::audioEngine()->outputSampleRate() * desiredFrequency;
	const auto inputSampleRate = m_buffer->sampleRate() * m_frequency;
	const auto resampleRatio = outputSampleRate / inputSampleRate;

	state->frameIndex = std::max<int>(m_startFrame, state->frameIndex);

	auto callbackData = CallbackData{.sample = this, .state = state, .loopMode = loopMode};
	state->resampler.resample(dst, static_cast<long>(numFrames), resampleRatio, &Sample::render, &callbackData);

	const auto amplification = m_amplification.load(std::memory_order_relaxed);
	for (auto i = std::size_t{0}; i < numFrames; ++i)
	{
		dst[i][0] *= amplification;
		dst[i][1] *= amplification;
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

std::size_t Sample::render(SampleFrame* dst, const std::size_t frames, void* data)
{
	const auto callbackData = static_cast<CallbackData*>(data);
	const auto state = callbackData->state;
	const auto sample = callbackData->sample;
	const auto loopMode = callbackData->loopMode;

	auto& index = state->frameIndex;
	auto& backwards = state->backwards;

	for (size_t frame = 0; frame < frames; ++frame)
	{
		switch (loopMode)
		{
		case Loop::Off:
			if (index < 0 || index >= sample->m_endFrame) { return frame; }
			break;
		case Loop::On:
			if (index < sample->m_loopStartFrame && backwards) { index = sample->m_loopEndFrame - 1; }
			else if (index >= sample->m_loopEndFrame) { index = sample->m_loopStartFrame; }
			break;
		case Loop::PingPong:
			if (index < sample->m_loopStartFrame && backwards)
			{
				index = sample->m_loopStartFrame;
				backwards = false;
			}
			else if (index >= sample->m_loopEndFrame)
			{
				index = sample->m_loopEndFrame - 1;
				backwards = true;
			}
			break;
		default:
			break;
		}

		const auto value = sample->m_buffer->data()[sample->m_reversed ? sample->m_buffer->size() - index - 1 : index];
		dst[frame] = value;
		backwards ? --index : ++index;
	}

	return frames;
}

} // namespace lmms
