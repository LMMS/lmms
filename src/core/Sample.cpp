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
{
	auto lock = std::shared_lock{other.m_mutex};
	m_buffer = other.m_buffer;
	m_startFrame = other.m_startFrame;
	m_endFrame = other.m_endFrame;
	m_loopStartFrame = other.m_loopStartFrame;
	m_loopEndFrame = other.m_loopEndFrame;
	m_amplification = other.m_amplification;
	m_frequency = other.m_frequency;
	m_reversed = other.m_reversed;
}

Sample::Sample(Sample&& other) noexcept
{
	auto lock = std::unique_lock{other.m_mutex};
	m_buffer = std::move(other.m_buffer);
	m_startFrame = std::exchange(other.m_startFrame, 0);
	m_endFrame = std::exchange(other.m_endFrame, 0);
	m_loopStartFrame = std::exchange(other.m_loopStartFrame, 0);
	m_loopEndFrame = std::exchange(other.m_loopEndFrame, 0);
	m_amplification = std::exchange(other.m_amplification, 0);
	m_frequency = std::exchange(other.m_frequency, DefaultBaseFreq);
	m_reversed = std::exchange(other.m_reversed, false);
}

Sample& Sample::operator=(Sample other) noexcept
{
	swap(*this, other);
	return *this;
}

auto swap(Sample& first, Sample& second) -> void
{
	auto lock = std::scoped_lock{first.m_mutex, second.m_mutex};
	using std::swap;
	swap(first.m_buffer, second.m_buffer);
	swap(first.m_startFrame, second.m_startFrame);
	swap(first.m_endFrame, second.m_endFrame);
	swap(first.m_loopStartFrame, second.m_loopStartFrame);
	swap(first.m_loopEndFrame, second.m_loopEndFrame);
	swap(first.m_amplification, second.m_amplification);
	swap(first.m_frequency, second.m_frequency);
	swap(first.m_reversed, second.m_reversed);
}

bool Sample::play(sampleFrame* dst, PlaybackState* state, int numFrames, float desiredFrequency, Loop loopMode) const
{
	auto lock = std::shared_lock{m_mutex};
	if (numFrames <= 0 || desiredFrequency <= 0) { return false; }

	auto resampleRatio = static_cast<float>(Engine::audioEngine()->processingSampleRate()) / m_buffer->sampleRate();
	resampleRatio *= m_frequency / desiredFrequency;

	auto playBuffer = std::vector<sampleFrame>(numFrames / resampleRatio);
	if (!typeInfo<float>::isEqual(resampleRatio, 1.0f))
	{
		playBuffer.resize(playBuffer.size() + s_interpolationMargins[state->m_interpolationMode]);
	}

	switch (loopMode)
	{
	case Loop::Off:
		state->m_frameIndex = std::clamp(state->m_frameIndex, m_startFrame, m_endFrame);
		if (state->m_frameIndex == m_endFrame) { return false; }
		break;
	case Loop::On:
		state->m_frameIndex = std::clamp(state->m_frameIndex, m_startFrame, m_loopEndFrame);
		if (state->m_frameIndex == m_loopEndFrame) { state->m_frameIndex = m_loopStartFrame; }
		break;
	case Loop::PingPong:
		state->m_frameIndex = std::clamp(state->m_frameIndex, m_startFrame, m_loopEndFrame);
		if (state->m_frameIndex == m_loopEndFrame)
		{
			state->m_frameIndex = m_loopEndFrame - 1;
			state->m_backwards = true;
		}
		else if (state->m_frameIndex <= m_loopStartFrame && state->m_backwards)
		{
			state->m_frameIndex = m_loopStartFrame;
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

	const auto lock = std::shared_lock{m_mutex};

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

	const auto numPoints = std::min(numFrames, width);
	auto min = std::vector<float>(numPoints, 1);
	auto max = std::vector<float>(numPoints, -1);
	auto squared = std::vector<float>(numPoints);

	for (int i = 0; i < numFrames; i += resolution)
	{
		const auto pixelIndex = i / framesPerPixel;
		if (pixelIndex >= numPoints) { break; }

		const auto value = std::accumulate(buffer[i].begin(), buffer[i].end(), 0.0f) / buffer[i].size();
		if (value > max[pixelIndex]) { max[pixelIndex] = value; }
		if (value < min[pixelIndex]) { min[pixelIndex] = value; }
		squared[pixelIndex] += value * value;
	}

	for (int i = 0; i < numPoints; i++)
	{
		const auto lineY1 = centerY - max[i] * halfHeight * m_amplification;
		const auto lineY2 = centerY - min[i] * halfHeight * m_amplification;

		auto lineX = i + x;
		if (m_reversed) { lineX = width - lineX; }

		p.drawLine(lineX, lineY1, lineX, lineY2);

		const auto rms = std::sqrt(squared[i] / framesPerResolution);
		const auto maxRMS = std::clamp(rms, min[i], max[i]);
		const auto minRMS = std::clamp(-rms, min[i], max[i]);

		const auto rmsLineY1 = centerY - maxRMS * halfHeight * m_amplification;
		const auto rmsLineY2 = centerY - minRMS * halfHeight * m_amplification;

		p.setPen(rmsColor);
		p.drawLine(lineX, rmsLineY1, lineX, rmsLineY2);
		p.setPen(color);
	}
}

auto Sample::sampleDuration() const -> std::chrono::milliseconds
{
	const auto lock = std::shared_lock{m_mutex};
	const auto numFrames = m_endFrame - m_startFrame;
	const auto duration = numFrames / static_cast<float>(m_buffer->sampleRate()) * 1000;
	return std::chrono::milliseconds{static_cast<int>(duration)};
}

auto Sample::sampleFile() const -> const QString&
{
	return m_buffer->audioFile();
}

auto Sample::sampleRate() const -> int
{
	return m_buffer->sampleRate();
}

auto Sample::sampleSize() const -> int
{
	return m_buffer->size();
}

auto Sample::toBase64() const -> QString
{
	return m_buffer->toBase64();
}

auto Sample::data() -> const sampleFrame*
{
	return m_buffer->data();
}

auto Sample::buffer() const -> std::shared_ptr<const SampleBuffer>
{
	const auto lock = std::shared_lock{m_mutex};
	return m_buffer;
}

auto Sample::startFrame() const -> int
{
	const auto lock = std::shared_lock{m_mutex};
	return m_startFrame;
}

auto Sample::endFrame() const -> int
{
	const auto lock = std::shared_lock{m_mutex};
	return m_endFrame;
}

auto Sample::loopStartFrame() const -> int
{
	const auto lock = std::shared_lock{m_mutex};
	return m_loopStartFrame;
}

auto Sample::loopEndFrame() const -> int
{
	const auto lock = std::shared_lock{m_mutex};
	return m_loopEndFrame;
}

auto Sample::amplification() const -> float
{
	const auto lock = std::shared_lock{m_mutex};
	return m_amplification;
}

auto Sample::frequency() const -> float
{
	const auto lock = std::shared_lock{m_mutex};
	return m_frequency;
}

auto Sample::reversed() const -> bool
{
	const auto lock = std::shared_lock{m_mutex};
	return m_reversed;
}

auto Sample::setStartFrame(int startFrame) -> void
{
	const auto lock = std::unique_lock{m_mutex};
	m_startFrame = startFrame;
}

auto Sample::setEndFrame(int endFrame) -> void
{
	const auto lock = std::unique_lock{m_mutex};
	m_endFrame = endFrame;
}

auto Sample::setLoopStartFrame(int loopStartFrame) -> void
{
	const auto lock = std::unique_lock{m_mutex};
	m_loopStartFrame = loopStartFrame;
}

auto Sample::setLoopEndFrame(int loopEndFrame) -> void
{
	const auto lock = std::unique_lock{m_mutex};
	m_loopEndFrame = loopEndFrame;
}

void Sample::setAllPointFrames(int startFrame, int endFrame, int loopStartFrame, int loopEndFrame)
{
	const auto lock = std::unique_lock{m_mutex};
	m_startFrame = startFrame;
	m_endFrame = endFrame;
	m_loopStartFrame = loopStartFrame;
	m_loopEndFrame = loopEndFrame;
}

auto Sample::setAmplification(float amplification) -> void
{
	const auto lock = std::unique_lock{m_mutex};
	m_amplification = amplification;
}

auto Sample::setFrequency(float frequency) -> void
{
	const auto lock = std::unique_lock{m_mutex};
	m_frequency = frequency;
}

auto Sample::setReversed(bool reversed) -> void
{
	const auto lock = std::unique_lock{m_mutex};
	m_reversed = reversed;
}

auto Sample::playSampleRange(PlaybackState* state, sampleFrame* dst, size_t numFrames) const -> void
{
	auto framesToCopy = 0;
	if (state->m_backwards)
	{
		framesToCopy = std::min<int>(state->m_frameIndex - m_startFrame, numFrames);
		copyBufferBackward(dst, state->m_frameIndex, framesToCopy);
	}
	else
	{
		framesToCopy = std::min<int>(m_endFrame - state->m_frameIndex, numFrames);
		copyBufferForward(dst, state->m_frameIndex, framesToCopy);
	}

	if (framesToCopy < numFrames) { std::fill_n(dst + framesToCopy, numFrames - framesToCopy, sampleFrame{0, 0}); }
}

auto Sample::copyBufferForward(sampleFrame* dst, int initialPosition, int advanceAmount) const -> void
{
	m_reversed ? std::copy_n(m_buffer->rbegin() + initialPosition, advanceAmount, dst)
			   : std::copy_n(m_buffer->begin() + initialPosition, advanceAmount, dst);
}

auto Sample::copyBufferBackward(sampleFrame* dst, int initialPosition, int advanceAmount) const -> void
{
	m_reversed ? std::reverse_copy(
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

auto Sample::amplifySampleRange(sampleFrame* src, int numFrames) const -> void
{
	const auto lock = std::shared_lock{m_mutex};
	for (int i = 0; i < numFrames; ++i)
	{
		src[i][0] *= m_amplification;
		src[i][1] *= m_amplification;
	}
}
} // namespace lmms
