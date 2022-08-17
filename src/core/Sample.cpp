/*
 * Sample.cpp - a SampleBuffer with its own characteristics
 *
 * Copyright (c) 2022 sakertooth <sakertooth@gmail.com>
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

#include <cmath>
#include <cstring>
#include <iostream>
#include <variant>

#include "ConfigManager.h"
#include "FileDialog.h"
#include "PathUtil.h"
#include "SampleBufferV2.h"

namespace lmms
{
	std::array<int, 5> Sample::s_sampleMargin = {64, 64, 64, 4, 4};

	Sample::Sample(const fs::path& sampleFile)
	{
		loadSampleFile(sampleFile);
	}

	Sample::Sample(const sampleFrame* data, const int numFrames)
		: m_sampleBuffer(std::make_shared<const SampleBufferV2>(data, numFrames))
		, m_endFrame(m_sampleBuffer->numFrames())
	{
	}

	Sample::Sample(const SampleBufferV2* buffer)
		: m_sampleBuffer(std::shared_ptr<const SampleBufferV2>(buffer))
		, m_endFrame(m_sampleBuffer->numFrames())
	{
	}

	Sample::Sample(const int numFrames)
		: m_sampleBuffer(std::make_shared<const SampleBufferV2>(numFrames))
		, m_endFrame(numFrames)
	{
	}

	Sample::Sample(const Sample& other)
		: m_sampleBuffer(other.m_sampleBuffer)
		, m_sampleRate(other.m_sampleRate)
		, m_amplification(other.m_amplification)
		, m_frequency(other.m_frequency)
		, m_reversed(other.m_reversed)
		, m_varyingPitch(other.m_varyingPitch)
		, m_interpolationMode(other.m_interpolationMode)
		, m_startFrame(other.m_startFrame)
		, m_endFrame(other.m_endFrame)
		, m_frameIndex(other.m_frameIndex)
	{
	}

	Sample::Sample(Sample&& other)
		: m_sampleBuffer(std::exchange(other.m_sampleBuffer, nullptr))
		, m_sampleRate(std::exchange(other.m_sampleRate, 0))
		, m_amplification(std::exchange(other.m_amplification, 1.0f))
		, m_frequency(std::exchange(other.m_frequency, 0.0f))
		, m_varyingPitch(std::exchange(other.m_varyingPitch, false))
		, m_interpolationMode(std::exchange(other.m_interpolationMode, SRC_LINEAR))
		, m_startFrame(std::exchange(other.m_startFrame, 0))
		, m_endFrame(std::exchange(other.m_endFrame, 0))
		, m_frameIndex(std::exchange(other.m_frameIndex, 0))
	{
	}

	Sample& Sample::operator=(Sample other)
	{
		std::swap(*this, other);
		return *this;
	}

	void swap(Sample& first, Sample& second)
	{
		first.m_sampleBuffer.swap(second.m_sampleBuffer);
		std::swap(first.m_sampleRate, second.m_sampleRate);
		std::swap(first.m_amplification, second.m_amplification);
		std::swap(first.m_frequency, second.m_frequency);
		std::swap(first.m_reversed, second.m_reversed);
		std::swap(first.m_varyingPitch, second.m_varyingPitch);
		std::swap(first.m_pingPongBackwards, second.m_pingPongBackwards);
		std::swap(first.m_interpolationMode, second.m_interpolationMode);
		std::swap(first.m_startFrame, second.m_startFrame);
		std::swap(first.m_endFrame, second.m_endFrame);
		std::swap(first.m_loopStartFrame, second.m_loopStartFrame);
		std::swap(first.m_loopEndFrame, second.m_loopEndFrame);
		std::swap(first.m_frameIndex, second.m_frameIndex);
		std::swap(first.m_resampleState, second.m_resampleState);
	}

	bool Sample::play(sampleFrame* dst, const int framesToPlay, const float freq, PlaybackType playback)
	{
		if (!m_sampleBuffer || framesToPlay <= 0) { return false; }

		const auto freqFactor = static_cast<double>(freq) / static_cast<double>(m_frequency) 
			* m_sampleRate / Engine::audioEngine()->processingSampleRate();

		const f_cnt_t totalFramesForCurrentPitch = static_cast<f_cnt_t>((m_endFrame - m_startFrame) / freqFactor);
		if (totalFramesForCurrentPitch == 0) { return false; }

		if (playback == PlaybackType::Regular && (m_frameIndex >= m_endFrame || (m_endFrame - m_frameIndex) / freqFactor == 0)) 
		{
			return false;
		}

		m_frameIndex = calculatePlaybackIndex(playback);

		f_cnt_t fragmentSize = static_cast<f_cnt_t>(framesToPlay * freqFactor) + s_sampleMargin[m_interpolationMode];
		f_cnt_t framesUsed = framesToPlay;

		auto& sampleData = m_sampleBuffer->sampleData();
		if (freqFactor != 1.0 || m_varyingPitch) 
		{
			SRC_DATA srcData;
			srcData.data_in = (sampleData.data() + m_frameIndex)->data();
			srcData.data_out = dst->data();
			srcData.input_frames = fragmentSize;
			srcData.output_frames = framesToPlay;
			srcData.src_ratio = 1.0 / freqFactor;
			srcData.end_of_input = 0;

			int error = src_process(m_resampleState, &srcData);
			if (error) 
			{
				std::cout << "SampleBuffer: error while resampling: " <<
					src_strerror(error) << '\n';
				return false;
			}

			if (srcData.output_frames_gen > framesToPlay)
			{
				std::cout << "SampleBuffer: not enough frames: " <<
					srcData.output_frames_gen << "/" << framesToPlay << '\n';
				return false;
			}

			framesUsed = srcData.input_frames_used;
		}

		m_reversed ? 
			std::reverse_copy(sampleData.end() - m_frameIndex - framesUsed - 1, sampleData.end() - m_frameIndex - 1, dst) :
			std::copy(sampleData.begin() + m_frameIndex, sampleData.begin() + m_frameIndex + framesUsed, dst);

		advanceFrameIndex(framesUsed, playback);
		for (int i = 0; i < framesToPlay; ++i) 
		{
			dst[i][0] *= m_amplification;
			dst[i][1] *= m_amplification;
		}

		return true;
	}

	/* @brief Draws a sample on the QRect given in the range [fromFrame, toFrame)
	* @param QPainter p: Painter object for the painting operations
	* @param QRect dr: QRect where the buffer will be drawn in
	* @param QRect clip: QRect used for clipping
	* @param int fromFrame: First frame of the range
	* @param int toFrame: Last frame of the range non-inclusive
	*/
	void Sample::visualize(QPainter& painter, const QRect& drawingRect, int fromFrame, int toFrame)
	{
		/*TODO:
			This function needs to be optimized.
			- We do not have to recalculate peaks and rms every time we want to visualize the sample.
			- You can store peaks and rms in 2 std::vector<QLines> instead of 4 std::vector<QPointF>'s
			- Allocating large std::vectors on a hot path like Sample::visualize is not good.
			- You can potentially reduce the number of frames you draw per pixel by choosing a certain frame per pixel
		ratio beforehand.

		This function also needs to be moved out of Sample in favor of no Qt in the core.
		*/

		if (!m_sampleBuffer || m_sampleBuffer->numFrames() == 0) { return; }

		const bool focusOnRange = toFrame <= m_sampleBuffer->numFrames() && 0 <= fromFrame && fromFrame < toFrame;
		// TODO: If the clip QRect is not being used we should remove it
		// p.setClipRect(clip);
		const int w = drawingRect.width();
		const int h = drawingRect.height();

		const int yb = h / 2 + drawingRect.y();
		const float ySpace = h * 0.5f;
		const int nbFrames = focusOnRange ? toFrame - fromFrame : m_sampleBuffer->numFrames();

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
		const int xb = drawingRect.x();
		const int first = focusOnRange ? fromFrame : 0;
		const int last = focusOnRange ? toFrame - 1 : m_sampleBuffer->numFrames() - 1;
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

			float rmsData[2] = {0, 0};

			// Find maximum and minimum samples within range
			for (int i = 0; i < fpp && frame + i <= last; ++i)
			{
				for (int j = 0; j < 2; ++j)
				{
					auto curData = m_sampleBuffer->sampleData()[static_cast<int>(frame) + i][j];

					if (curData > maxData) { maxData = curData; }
					if (curData < minData) { minData = curData; }

					rmsData[j] += curData * curData;
				}
			}

			const float trueRmsData = (rmsData[0] + rmsData[1]) / 2 / fpp;
			const float sqrtRmsData = sqrt(trueRmsData);
			const float maxRmsData = qBound(minData, sqrtRmsData, maxData);
			const float minRmsData = qBound(minData, -sqrtRmsData, maxData);

			// If nbFrames >= w, we can use curPixel to calculate X
			// but if nbFrames < w, we need to calculate it proportionally
			// to the total number of points
			auto x = nbFrames >= w ? xb + curPixel : xb + ((static_cast<double>(curPixel) / nbFrames) * w);
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
			painter.drawLine(fEdgeMax[i], fEdgeMin[i]);
		}

		painter.setPen(painter.pen().color().lighter(123));

		for (int i = 0; i < totalPoints; ++i)
		{
			painter.drawLine(fRmsMax[i], fRmsMin[i]);
		}
	}

	std::string Sample::sampleFile() const
	{
		if (!m_sampleBuffer || !m_sampleBuffer->filePath()) { return ""; }
		return m_sampleBuffer->filePath()->generic_string(); 
	}

	std::shared_ptr<const SampleBufferV2> Sample::sampleBuffer() const
	{
		return m_sampleBuffer;
	}

	sample_rate_t Sample::sampleRate() const 
	{
		return m_sampleRate;
	}

	float Sample::amplification() const
	{
		return m_amplification;
	}

	float Sample::frequency() const
	{
		return m_frequency;
	}

	bool Sample::reversed() const
	{
		return m_reversed;
	}

	bool Sample::varyingPitch() const
	{
		return m_varyingPitch;
	}

	int Sample::interpolationMode() const
	{
		return m_interpolationMode;
	}

	int Sample::startFrame() const
	{
		return m_startFrame;
	}

	int Sample::endFrame() const
	{
		return m_endFrame;
	}

	int Sample::loopStartFrame() const
	{
		return m_loopStartFrame;
	}

	int Sample::loopEndFrame() const
	{
		return m_loopEndFrame;
	}

	int Sample::frameIndex() const
	{
		return m_frameIndex;
	}

	int Sample::numFrames() const
	{
		return m_sampleBuffer ? m_sampleBuffer->numFrames() : 0;
	}

	void Sample::setSampleBuffer(const SampleBufferV2* buffer)
	{
		m_sampleBuffer.reset(buffer);
		resetMarkers();
	}

	void Sample::setSampleRate(const sample_rate_t sampleRate) 
	{
		m_sampleRate = sampleRate;
	}

	void Sample::setAmplification(const float amplification)
	{
		m_amplification = amplification;
	}

	void Sample::setFrequency(const float frequency)
	{
		m_frequency = frequency;
	}

	void Sample::setReversed(const bool reversed)
	{
		m_reversed = reversed;
	}

	void Sample::setVaryingPitch(const bool varyingPitch)
	{
		m_varyingPitch = varyingPitch;
	}

	void Sample::setInterpolationMode(const int interpolationMode)
	{
		m_interpolationMode = interpolationMode;
	}

	void Sample::setStartFrame(const int start)
	{
		m_startFrame = start;
	}

	void Sample::setEndFrame(const int end)
	{
		m_endFrame = end;
	}

	void Sample::setLoopStartFrame(const int loopStart)
	{
		m_loopStartFrame = loopStart;
	}

	void Sample::setLoopEndFrame(const int loopEnd)
	{
		m_loopEndFrame = loopEnd;
	}

	void Sample::setAllPointFrames(const int start, const int end, const int loopStart, const int loopEnd) 
	{
		m_startFrame = start;
		m_endFrame = end;
		m_loopStartFrame = loopStart;
		m_loopEndFrame = loopEnd;
	}

	void Sample::setFrameIndex(const int frameIndex)
	{
		m_frameIndex = frameIndex;
	}

	std::string Sample::toBase64() const
	{
		if (!m_sampleBuffer) { return ""; }
		
		//TODO: Base64 encoding without the use of Qt
		const char* rawData = reinterpret_cast<const char*>(m_sampleBuffer->sampleData().data());
		QByteArray data = QByteArray(rawData, m_sampleBuffer->sampleData().size() * sizeof(sampleFrame));
		return data.toBase64().constData();
	}

	void Sample::loadSampleFile(const fs::path& sampleFile)
	{
		auto cachedSampleBuffer = Engine::sampleBufferCache()->get(sampleFile.generic_string());
		m_sampleBuffer = cachedSampleBuffer ? cachedSampleBuffer :
			Engine::sampleBufferCache()->add(sampleFile.generic_string());
		resetMarkers();
	}

	void Sample::loadBase64(const std::string& base64)
	{
		m_sampleBuffer = SampleBufferV2::loadFromBase64(base64);
		resetMarkers();
	}

	void Sample::resetMarkers()
	{
		m_startFrame = 0;
		m_endFrame = m_sampleBuffer->numFrames();
		m_loopStartFrame = std::clamp(0, m_loopStartFrame, m_endFrame);
		m_loopEndFrame = std::clamp(0, m_loopEndFrame, m_endFrame);
		m_frameIndex = std::clamp(0, m_frameIndex, m_endFrame);
	}

	int Sample::calculateLength() const 
	{
		return static_cast<double>(m_endFrame - m_startFrame) / m_sampleRate * 1000;
	}

	int Sample::calculateTickLength() const
	{
		return 1 / Engine::framesPerTick() * m_sampleBuffer->numFrames();
	}

	void Sample::advanceFrameIndex(f_cnt_t amount, PlaybackType playback) 
	{
		if (playback == PlaybackType::Regular || playback == PlaybackType::LoopPoints) 
		{
			m_frameIndex += amount;
		}
		else if (playback == PlaybackType::PingPong) 
		{
			f_cnt_t left = amount;
				
			if (m_reversed)
			{
				m_frameIndex -= amount;
				if (m_frameIndex < m_loopStartFrame)
				{
					left -= (m_loopStartFrame - m_frameIndex);
					m_frameIndex = m_loopStartFrame;
				}
				else left = 0;
			}
			
			m_frameIndex += left;
		}

		m_frameIndex = calculatePlaybackIndex(playback);
	}

	f_cnt_t Sample::calculatePlaybackIndex(PlaybackType playback)
	{
		if (playback == PlaybackType::Regular) 
		{
			return std::max(m_frameIndex, m_startFrame);
		}
		else if (playback == PlaybackType::LoopPoints) 
		{
			if (m_frameIndex < m_loopEndFrame) { return m_frameIndex; }
			return m_frameIndex + (m_frameIndex - m_loopStartFrame) % (m_loopEndFrame - m_loopStartFrame);
		}
		else if (playback == PlaybackType::PingPong) 
		{
			if (m_frameIndex < m_loopEndFrame) { return m_frameIndex; }
			
			const f_cnt_t loopLen = m_loopEndFrame - m_loopStartFrame;
			const f_cnt_t loopPos = (m_frameIndex - m_loopEndFrame) % (loopLen * 2);

			return (loopPos < loopLen)
				? m_loopEndFrame - loopPos
				: m_loopStartFrame + (loopPos - loopLen);
		}

		return m_frameIndex;
	}

	SRC_STATE* Sample::createResampleState() 
	{
		int error = 0;
		SRC_STATE* state = src_new(m_interpolationMode, DEFAULT_CHANNELS, &error);

		if (error != 0)
		{
			throw std::runtime_error{"Sample.cpp: Failed to create resample state"};
		}

		return state;
	}
}

