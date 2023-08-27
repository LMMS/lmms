/*
 * Sample.cpp - State for container-class SampleBuffer2
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
	: m_buffer(std::make_shared<SampleBuffer2>(audioFile))
	, m_startFrame(0)
	, m_endFrame(m_buffer->size())
	, m_loopStartFrame(0)
	, m_loopEndFrame(m_buffer->size())
{
}

Sample::Sample(const QByteArray& base64, int sampleRate)
	: m_buffer(std::make_shared<SampleBuffer2>(base64, sampleRate))
	, m_startFrame(0)
	, m_endFrame(m_buffer->size())
	, m_loopStartFrame(0)
	, m_loopEndFrame(m_buffer->size())
{
}

Sample::Sample(const sampleFrame* data, int numFrames, int sampleRate)
	: m_buffer(std::make_shared<SampleBuffer2>(data, numFrames, sampleRate))
	, m_startFrame(0)
	, m_endFrame(m_buffer->size())
	, m_loopStartFrame(0)
	, m_loopEndFrame(m_buffer->size())
{
}

Sample::Sample(std::shared_ptr<const SampleBuffer2> buffer)
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
	if (m_buffer->sampleRate() <= 0) { return false; }

	const auto lock = std::shared_lock{m_mutex};
	const auto resampleRatio
		= m_frequency / desiredFrequency * Engine::audioEngine()->processingSampleRate() / m_buffer->sampleRate();
	auto playedSuccessfully = false;

	switch (loopMode)
	{
	case Loop::Off:
		playedSuccessfully = playSampleRange(state, dst, numFrames, resampleRatio);
		break;
	case Loop::On:
		playedSuccessfully = playSampleRangeLoop(state, dst, numFrames, resampleRatio);
		break;
	case Loop::PingPong:
		playedSuccessfully = playSampleRangePingPong(state, dst, numFrames, resampleRatio);
		break;
	default:
		return false;
	}

	if (src_error(state->m_resampleState) != 0 || !playedSuccessfully) { return false; }
	amplifySampleRange(dst, numFrames);
	return true;
}

void Sample::visualize(QPainter& p, const QRect& dr, int fromFrame, int toFrame) const
{
	const auto lock = std::shared_lock{m_mutex};
	const auto numFrames = static_cast<int>(m_buffer->size());
	if (numFrames == 0) { return; }

	const bool focusOnRange = toFrame <= numFrames && 0 <= fromFrame && fromFrame < toFrame;
	const int w = dr.width();
	const int h = dr.height();

	const int yb = h / 2 + dr.y();
	const float ySpace = h * 0.5f;
	const int nbFrames = focusOnRange ? toFrame - fromFrame : numFrames;

	const double fpp = std::max(1., static_cast<double>(nbFrames) / w);
	// There are 2 possibilities: Either nbFrames is bigger than
	// the width, so we will have width points, or nbFrames is
	// smaller than the width (fpp = 1) and we will have nbFrames
	// points
	const int totalPoints = nbFrames > w ? w : nbFrames;
	std::vector<QPointF> fEdgeMax(totalPoints);
	std::vector<QPointF> fEdgeMin(totalPoints);
	std::vector<QPointF> fRmsMax(totalPoints);
	std::vector<QPointF> fRmsMin(totalPoints);
	int curPixel = 0;
	const int xb = dr.x();
	const int first = focusOnRange ? fromFrame : 0;
	const int last = focusOnRange ? toFrame - 1 : numFrames - 1;
	// When the number of frames isn't perfectly divisible by the
	// width, the remaining frames don't fit the last pixel and are
	// past the visible area. lastVisibleFrame is the index number of
	// the last visible frame.
	const int visibleFrames = (fpp * w);
	const int lastVisibleFrame = focusOnRange ? fromFrame + visibleFrames - 1 : visibleFrames - 1;

	for (double frame = first; frame <= last && frame <= lastVisibleFrame; frame += fpp)
	{
		float maxData = -1;
		float minData = 1;

		auto rmsData = std::array<float, 2>{};

		// Find maximum and minimum samples within range
		for (int i = 0; i < fpp && frame + i <= last; ++i)
		{
			for (int j = 0; j < 2; ++j)
			{
				auto curData = m_buffer->data()[static_cast<int>(frame) + i][j];

				if (curData > maxData) { maxData = curData; }
				if (curData < minData) { minData = curData; }

				rmsData[j] += curData * curData;
			}
		}

		const float trueRmsData = (rmsData[0] + rmsData[1]) / 2 / fpp;
		const float sqrtRmsData = std::sqrt(trueRmsData);
		const float maxRmsData = std::clamp(sqrtRmsData, minData, maxData);
		const float minRmsData = std::clamp(-sqrtRmsData, minData, maxData);

		// If nbFrames >= w, we can use curPixel to calculate X
		// but if nbFrames < w, we need to calculate it proportionally
		// to the total number of points
		auto x = nbFrames >= w ? xb + curPixel : xb + ((static_cast<double>(curPixel) / nbFrames) * w);

		if (m_reversed) { x = w - 1 - x; }

		// Partial Y calculation
		auto py = ySpace * m_amplification;
		fEdgeMax[curPixel] = QPointF(x, (yb - (maxData * py)));
		fEdgeMin[curPixel] = QPointF(x, (yb - (minData * py)));
		fRmsMax[curPixel] = QPointF(x, (yb - (maxRmsData * py)));
		fRmsMin[curPixel] = QPointF(x, (yb - (minRmsData * py)));
		++curPixel;
	}

	for (int i = 0; i < totalPoints; ++i)
	{
		p.drawLine(fEdgeMax[i], fEdgeMin[i]);
	}

	p.setPen(p.pen().color().lighter(123));

	for (int i = 0; i < totalPoints; ++i)
	{
		p.drawLine(fRmsMax[i], fRmsMin[i]);
	}
}

auto Sample::sampleDuration() const -> int
{
	const auto lock = std::shared_lock{m_mutex};
	return m_buffer->sampleRate() > 0 ? static_cast<double>(m_endFrame - m_startFrame) / m_buffer->sampleRate() * 1000
									  : 0;
}

auto Sample::playbackSize() const -> int
{
	const auto lock = std::shared_lock{m_mutex};
	return m_buffer->sampleRate() > 0
		? m_buffer->size() * Engine::audioEngine()->processingSampleRate() / m_buffer->sampleRate()
		: 0;
}

auto Sample::buffer() const -> std::shared_ptr<const SampleBuffer2>
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

auto Sample::playSampleRange(PlaybackState* state, sampleFrame* dst, int numFrames, float resampleRatio) const -> bool
{
	if (state->m_frameIndex >= m_endFrame || numFrames <= 0) { return false; }
	state->m_frameIndex = std::max(m_startFrame, state->m_frameIndex);

	const auto numFramesToCopy = std::min<int>(
		numFrames / resampleRatio + (resampleRatio != 1.0f ? s_interpolationMargins[state->m_interpolationMode] : 0),
		m_endFrame - state->m_frameIndex);

	auto buffer = std::vector<sampleFrame>(numFramesToCopy);
	copyBufferForward(buffer.data(), state->m_frameIndex, numFramesToCopy);

	auto resample
		= resampleSampleRange(state->m_resampleState, buffer.data(), dst, numFramesToCopy, numFrames, resampleRatio);
	state->m_frameIndex += resample.input_frames_used;
	return true;
}

auto Sample::playSampleRangeLoop(PlaybackState* state, sampleFrame* dst, int numFrames, float resampleRatio) const -> bool
{
	if (numFrames <= 0) { return false; }
	if (state->m_frameIndex >= m_loopEndFrame) { state->m_frameIndex = m_loopStartFrame; }
	auto playFrame = std::max(m_startFrame, state->m_frameIndex);

	const auto totalFramesToCopy = static_cast<int>(
		numFrames / resampleRatio + (resampleRatio != 1.0f ? s_interpolationMargins[state->m_interpolationMode] : 0));
	auto buffer = std::vector<sampleFrame>(totalFramesToCopy);

	auto numFramesCopied = 0;
	while (numFramesCopied != totalFramesToCopy)
	{
		auto numFramesToCopy = std::min(totalFramesToCopy - numFramesCopied, m_loopEndFrame - playFrame);
		copyBufferForward(buffer.data() + numFramesCopied, playFrame, numFramesToCopy);

		playFrame += numFramesToCopy;
		numFramesCopied += numFramesToCopy;

		if (playFrame >= m_loopEndFrame) { playFrame = m_loopStartFrame; }
	}

	const auto resample
		= resampleSampleRange(state->m_resampleState, buffer.data(), dst, totalFramesToCopy, numFrames, resampleRatio);
	state->m_frameIndex
		= getLoopedIndex(state->m_frameIndex + resample.input_frames_used, m_loopStartFrame, m_loopEndFrame);
	return true;
}

auto Sample::playSampleRangePingPong(PlaybackState* state, sampleFrame* dst, int numFrames, float resampleRatio) const -> bool
{
	if (numFrames <= 0) { return false; }
	if (state->m_frameIndex >= m_loopEndFrame)
	{
		state->m_frameIndex = m_loopEndFrame - 1;
		state->m_backwards = true;
	}

	auto playFrame = std::min(m_endFrame, state->m_frameIndex);

	const auto totalFramesToCopy = static_cast<int>(
		numFrames / resampleRatio + (resampleRatio != 1.0f ? s_interpolationMargins[state->m_interpolationMode] : 0));
	auto buffer = std::vector<sampleFrame>(totalFramesToCopy);

	auto numFramesCopied = 0;
	while (numFramesCopied != totalFramesToCopy)
	{
		auto numFramesToCopy = 0;
		if (!state->m_backwards)
		{
			numFramesToCopy = std::min(totalFramesToCopy - numFramesCopied, m_loopEndFrame - playFrame);
			copyBufferForward(buffer.data() + numFramesCopied, playFrame, numFramesToCopy);
			playFrame += numFramesToCopy;
		}
		else
		{
			numFramesToCopy = std::min(totalFramesToCopy - numFramesCopied, playFrame - m_loopStartFrame);
			copyBufferBackward(buffer.data() + numFramesCopied, playFrame, numFramesToCopy);
			playFrame -= numFramesToCopy;
		}

		numFramesCopied += numFramesToCopy;
		if (playFrame >= m_loopEndFrame && !state->m_backwards)
		{
			playFrame = m_loopEndFrame - 1;
			state->m_backwards = true;
		}
		else if (playFrame <= m_loopStartFrame && state->m_backwards)
		{
			playFrame = m_loopStartFrame;
			state->m_backwards = false;
		}
	}

	const auto resample
		= resampleSampleRange(state->m_resampleState, buffer.data(), dst, totalFramesToCopy, numFrames, resampleRatio);
	state->m_frameIndex += (state->m_backwards ? -1 : 1) * resample.input_frames_used;
	state->m_frameIndex = getPingPongIndex(state->m_frameIndex, m_loopStartFrame, m_loopEndFrame);
	return true;
}

auto Sample::copyBufferForward(sampleFrame* dst, int initialPosition, int advanceAmount) const -> void
{
	m_reversed ? std::copy_n(m_buffer->rbegin() + initialPosition, advanceAmount, dst)
			   : std::copy_n(m_buffer->begin() + initialPosition, advanceAmount, dst);
}

auto Sample::copyBufferBackward(sampleFrame* dst, int initialPosition, int advanceAmount) const -> void
{
	m_reversed ? std::reverse_copy(m_buffer->rbegin() + initialPosition - advanceAmount, m_buffer->rbegin() + initialPosition, dst)
			   : std::reverse_copy(m_buffer->begin() + initialPosition - advanceAmount, m_buffer->begin() + initialPosition, dst);
}

auto Sample::getLoopedIndex(int index, int startFrame, int endFrame) const -> int
{
	return index < endFrame ? index : startFrame + (index - startFrame) % (endFrame - startFrame);
}

auto Sample::getPingPongIndex(int index, int startFrame, int endFrame) const -> int
{
	if (index < endFrame) { return index; }
	const auto loopPos = getLoopedIndex(index, startFrame, endFrame * 2);
	return loopPos > endFrame ? endFrame * 2 - loopPos : loopPos;
}

auto Sample::resampleSampleRange(SRC_STATE* state, sampleFrame* src, sampleFrame* dst, int numInputFrames,
	int numOutputFrames, double ratio) const -> SRC_DATA
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

Sample::PlaybackState::PlaybackState(bool varyingPitch, int mode)
	: m_varyingPitch(varyingPitch)
	, m_interpolationMode(mode)
{
	int error = 0;
	m_resampleState = src_new(m_interpolationMode, DEFAULT_CHANNELS, &error);
	if (error != 0) { throw std::runtime_error{"Error creating resample state: " + std::string{src_strerror(error)}}; }
}

Sample::PlaybackState::~PlaybackState() noexcept
{
	src_delete(m_resampleState);
}

auto Sample::PlaybackState::frameIndex() const -> f_cnt_t
{
	return m_frameIndex;
}

auto Sample::PlaybackState::varyingPitch() const -> bool
{
	return m_varyingPitch;
}

auto Sample::PlaybackState::isBackwards() const -> bool
{
	return m_backwards;
}

auto Sample::PlaybackState::interpolationMode() const -> int
{
	return m_interpolationMode;
}

auto Sample::PlaybackState::setFrameIndex(f_cnt_t index) -> void
{
	m_frameIndex = index;
}

auto Sample::PlaybackState::setVaryingPitch(bool varyingPitch) -> void
{
	m_varyingPitch = varyingPitch;
}

auto Sample::PlaybackState::setBackwards(bool backwards) -> void
{
	m_backwards = backwards;
}
} // namespace lmms