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

bool Sample::play(SampleFrame* dst, PlaybackState* state, size_t numFrames, double frequency, Loop loopMode) const
{
	assert(numFrames > 0);
	assert(frequency > 0);
	if (m_buffer->empty()) { return false; }

	const auto outputSampleRate = Engine::audioEngine()->outputSampleRate() * m_frequency / frequency;
	const auto inputSampleRate = m_buffer->sampleRate();
	const auto resampleRatio = outputSampleRate / inputSampleRate;

	state->frameIndex = std::max<int>(m_startFrame, state->frameIndex);
	state->sample = this;
	state->loop = &loopMode;

	src_set_ratio(state->resampleState, resampleRatio);
	if (src_callback_read(state->resampleState, resampleRatio, numFrames, &dst[0][0]) != 0)
	{
		MixHelpers::multiply(dst, m_amplification, numFrames);
		return true;
	}

	return false;
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

long Sample::render(void* callbackData, float** data)
{
	const auto state = static_cast<PlaybackState*>(callbackData);
	const auto loop = *state->loop;
	const auto sample = state->sample;
	auto& index = state->frameIndex;
	auto& backwards = state->backwards;

	switch (loop)
	{
	case Loop::Off:
		if (index < 0 || index >= sample->m_endFrame) { return 0; }
		break;
	case Loop::On:
		if (index < sample->m_loopStartFrame && state->backwards) { index = sample->m_loopEndFrame - 1; }
		else if (index >= sample->m_loopEndFrame) { index = sample->m_loopStartFrame; }
		break;
	case Loop::PingPong:
		if (index < sample->m_loopStartFrame && state->backwards)
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

	const auto srcIndex = sample->m_reversed ? sample->m_buffer->size() - index - 1 : index;
	*data = const_cast<float*>(&sample->m_buffer->data()[srcIndex][0]);
	backwards ? --index : ++index;
	return 1;
}

} // namespace lmms
