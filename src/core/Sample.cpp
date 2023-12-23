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

#include <QPainter>
#include <QRect>

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

auto Sample::operator=(const Sample& other) -> Sample&
{
	if (this == &other) { return *this; }

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
	if (this == &other) { return *this; }

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

bool Sample::play(sampleFrame* dst, PlaybackState* state, int numFrames, float desiredFrequency, Loop loopMode) const
{
	if (numFrames <= 0 || desiredFrequency <= 0) { return false; }

	auto resampleRatio = static_cast<float>(Engine::audioEngine()->processingSampleRate()) / m_buffer->sampleRate();
	resampleRatio *= frequency() / desiredFrequency;

	auto playBuffer = std::vector<sampleFrame>(numFrames / resampleRatio);
	if (!typeInfo<float>::isEqual(resampleRatio, 1.0f))
	{
		playBuffer.resize(playBuffer.size() + s_interpolationMargins[state->m_interpolationMode]);
	}

	const auto start = startFrame();
	const auto end = endFrame();
	const auto loopStart = loopStartFrame();
	const auto loopEnd = loopEndFrame();

	switch (loopMode)
	{
	case Loop::Off:
		state->m_frameIndex = std::clamp(state->m_frameIndex, start, end);
		if (state->m_frameIndex == end) { return false; }
		break;
	case Loop::On:
		state->m_frameIndex = std::clamp(state->m_frameIndex, start, loopEnd);
		if (state->m_frameIndex == loopEnd) { state->m_frameIndex = loopStart; }
		break;
	case Loop::PingPong:
		state->m_frameIndex = std::clamp(state->m_frameIndex, start, loopEnd);
		if (state->m_frameIndex == loopEnd)
		{
			state->m_frameIndex = loopEnd - 1;
			state->m_backwards = true;
		}
		else if (state->m_frameIndex <= loopStart && state->m_backwards)
		{
			state->m_frameIndex = loopStart;
			state->m_backwards = false;
		}
		break;
	}

	playSampleRange(state, playBuffer.data(), playBuffer.size());

	auto resample = resampleSampleRange(
		state->m_resampleState, playBuffer.data(), dst, playBuffer.size(), numFrames, resampleRatio);
	state->m_frameIndex += (state->m_backwards ? -1 : 1) * resample.input_frames_used;
	if (src_error(state->m_resampleState) != 0) { return false; }

	amplifySampleRange(dst, numFrames);
	return true;
}

void Sample::visualize(QPainter& p, const QRect& dr, int fromFrame, int toFrame) const
{
	if (m_buffer->size() == 0) { return; }

	const auto x = dr.x();
	const auto height = dr.height();
	const auto width = dr.width();
	const auto centerY = dr.center().y();

	const auto halfHeight = height / 2;
	const auto buffer = m_buffer->data() + fromFrame;

	const auto color = p.pen().color();
	const auto rmsColor = color.lighter(123);

	auto numFrames = toFrame - fromFrame;
	if (numFrames == 0) { numFrames = m_buffer->size(); }

	const auto framesPerPixel = std::max(1, numFrames / width);

	constexpr auto maxFramesPerPixel = 512;
	const auto resolution = std::max(1, framesPerPixel / maxFramesPerPixel);
	const auto framesPerResolution = framesPerPixel / resolution;

	const auto numPixels = std::min(numFrames, width);
	auto min = std::vector<float>(numPixels, 1);
	auto max = std::vector<float>(numPixels, -1);
	auto squared = std::vector<float>(numPixels);

	const auto maxFrames = numPixels * framesPerPixel;
	for (int i = 0; i < maxFrames; i += resolution)
	{
		const auto pixelIndex = i / framesPerPixel;
		const auto value = std::accumulate(buffer[i].begin(), buffer[i].end(), 0.0f) / buffer[i].size();
		if (value > max[pixelIndex]) { max[pixelIndex] = value; }
		if (value < min[pixelIndex]) { min[pixelIndex] = value; }
		squared[pixelIndex] += value * value;
	}

	const auto amplification = m_amplification.load(std::memory_order_relaxed);
	const auto reversed = m_reversed.load(std::memory_order_relaxed);

	for (int i = 0; i < numPixels; i++)
	{
		const auto lineY1 = centerY - max[i] * halfHeight * amplification;
		const auto lineY2 = centerY - min[i] * halfHeight * amplification;

		auto lineX = i + x;
		if (reversed) { lineX = width - lineX; }

		p.drawLine(lineX, lineY1, lineX, lineY2);

		const auto rms = std::sqrt(squared[i] / framesPerResolution);
		const auto maxRMS = std::clamp(rms, min[i], max[i]);
		const auto minRMS = std::clamp(-rms, min[i], max[i]);

		const auto rmsLineY1 = centerY - maxRMS * halfHeight * amplification;
		const auto rmsLineY2 = centerY - minRMS * halfHeight * amplification;

		p.setPen(rmsColor);
		p.drawLine(lineX, rmsLineY1, lineX, rmsLineY2);
		p.setPen(color);
	}
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

void Sample::playSampleRange(PlaybackState* state, sampleFrame* dst, size_t numFrames) const
{
	auto framesToCopy = 0;
	if (state->m_backwards)
	{
		framesToCopy = std::min<int>(state->m_frameIndex - startFrame(), numFrames);
		copyBufferBackward(dst, state->m_frameIndex, framesToCopy);
	}
	else
	{
		framesToCopy = std::min<int>(endFrame() - state->m_frameIndex, numFrames);
		copyBufferForward(dst, state->m_frameIndex, framesToCopy);
	}

	if (framesToCopy < numFrames) { std::fill_n(dst + framesToCopy, numFrames - framesToCopy, sampleFrame{0, 0}); }
}

void Sample::copyBufferForward(sampleFrame* dst, int initialPosition, int advanceAmount) const
{
	reversed() ? std::copy_n(m_buffer->rbegin() + initialPosition, advanceAmount, dst)
			   : std::copy_n(m_buffer->begin() + initialPosition, advanceAmount, dst);
}

void Sample::copyBufferBackward(sampleFrame* dst, int initialPosition, int advanceAmount) const
{
	reversed() ? std::reverse_copy(
		m_buffer->rbegin() + initialPosition - advanceAmount, m_buffer->rbegin() + initialPosition, dst)
			   : std::reverse_copy(
				   m_buffer->begin() + initialPosition - advanceAmount, m_buffer->begin() + initialPosition, dst);
}

auto Sample::resampleSampleRange(SRC_STATE* state, sampleFrame* src, sampleFrame* dst, size_t numInputFrames,
	size_t numOutputFrames, double ratio) const -> SRC_DATA
{
	auto data = SRC_DATA{};
	data.data_in = &src[0][0];
	data.data_out = &dst[0][0];
	data.input_frames = numInputFrames;
	data.output_frames = numOutputFrames;
	data.src_ratio = ratio;
	data.end_of_input = 0;
	src_process(state, &data);
	return data;
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
