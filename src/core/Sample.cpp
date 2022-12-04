/*
 * Sample.cpp - a SampleBuffer with its own characteristics
 *
 * Copyright (c) 2022 saker <sakertooth@gmail.com>
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
#include "SampleBufferCache.h"
#include "lmms_basics.h"

#include <cmath>
#include <algorithm>
#include <iostream>

namespace lmms 
{
	Sample::Sample(const QString& sampleData, bool isBase64)
	{
		const auto cachedSampleBuffer = Engine::sampleBufferCache()->get(sampleData);
		m_sampleBuffer = cachedSampleBuffer ? cachedSampleBuffer : Engine::sampleBufferCache()->add(sampleData, isBase64);
		m_endFrame = m_sampleBuffer->numFrames();
	}

	Sample::Sample(const sampleFrame* data, const int numFrames) :
		m_sampleBuffer(std::make_shared<const SampleBufferV2>(data, numFrames)),
		m_endFrame(m_sampleBuffer->numFrames()) {}

	Sample::Sample(const SampleBufferV2* buffer) :
		m_sampleBuffer(std::shared_ptr<const SampleBufferV2>(buffer)),
		m_endFrame(m_sampleBuffer->numFrames()) {}

	Sample::Sample(const int numFrames) :
		m_sampleBuffer(std::make_shared<const SampleBufferV2>(numFrames)),
		m_endFrame(m_sampleBuffer->numFrames()) {}

	Sample::Sample(const Sample& other) :
		m_sampleBuffer(other.m_sampleBuffer),
		m_reversed(other.m_reversed),
		m_startFrame(other.m_startFrame),
		m_endFrame(other.m_endFrame),
		m_frameIndex(other.m_frameIndex) {}

	Sample::Sample(Sample&& other) :
		m_sampleBuffer(std::exchange(other.m_sampleBuffer, nullptr)),
		m_reversed(std::exchange(other.m_reversed, false)),
		m_startFrame(std::exchange(other.m_startFrame, 0)),
		m_endFrame(std::exchange(other.m_endFrame, 0)),
		m_frameIndex(std::exchange(other.m_frameIndex, 0)) {}

	Sample& Sample::operator=(const Sample& other) 
	{
		m_sampleBuffer = other.m_sampleBuffer;
		m_reversed = other.m_reversed;
		m_startFrame = other.m_startFrame;
		m_endFrame = other.m_endFrame;
		m_frameIndex = other.m_frameIndex;

		return *this;
	}

	Sample& Sample::operator=(Sample&& other) 
	{
		m_sampleBuffer = std::exchange(other.m_sampleBuffer, nullptr);
		m_reversed = std::exchange(other.m_reversed, false);
		m_startFrame = std::exchange(other.m_startFrame, 0);
		m_endFrame = std::exchange(other.m_endFrame, 0);
		m_frameIndex = std::exchange(other.m_frameIndex, 0);

		return *this;
	}

	Sample::~Sample() 
	{
		if (m_resampleState) 
		{
			src_delete(m_resampleState);
		}
	}

	bool Sample::play(sampleFrame* dst, const int numFramesRequested, const float frequencyToPlay) 
	{
		rescaleMarkers(m_startFrame, m_endFrame);
		m_frameIndex = m_reversed ? std::min(m_frameIndex, m_endFrame) : std::max(m_startFrame, m_frameIndex);

		const auto numFramesAvailable = m_reversed ? m_frameIndex - m_startFrame : m_endFrame - m_frameIndex;
		const auto& sampleData = m_sampleBuffer->sampleData();
		if (numFramesAvailable <= 0) { return false; }

		auto numFramesToPlay = std::min(numFramesAvailable, numFramesRequested);	
		if (!m_reversed) 
		{
			std::copy(sampleData.begin() + m_frameIndex, sampleData.begin() + m_frameIndex + numFramesToPlay, dst);
			m_frameIndex += numFramesToPlay;
		}
		else
		{
			std::reverse_copy(sampleData.begin() + m_frameIndex - numFramesToPlay, sampleData.begin() + m_frameIndex, dst);
			m_frameIndex -= numFramesToPlay;
		}

		const auto frequencyFactor = frequencyToPlay / DefaultBaseFreq;
		if (frequencyFactor != 1.0f)
		{
			int error = 0;
			if (!m_resampleState && (m_resampleState = src_new(SRC_LINEAR, DEFAULT_CHANNELS, &error)) == nullptr)
			{
				std::cerr << "Sample.cpp: src_new() failed\n";
			}
			
			SRC_DATA srcData;
			srcData.data_in = m_reversed ? sampleData.data()->data() + m_frameIndex - numFramesToPlay : sampleData.data()->data() + m_frameIndex;
			srcData.data_out = dst->data();
			srcData.input_frames = numFramesToPlay;
			srcData.output_frames = numFramesRequested;
			srcData.src_ratio = 1.0 / frequencyFactor;
			srcData.end_of_input = 0;

			error = src_process(m_resampleState, &srcData);
			if (error)
			{
				std::cerr << "Sample.cpp: error while resampling: " << src_strerror(error) << '\n';
			}

			if (srcData.output_frames_gen > numFramesRequested)
			{
				std::cerr << "Sample.cpp: not enough frames: " << srcData.output_frames_gen << "/" <<  numFramesRequested << '\n';
			}

			numFramesToPlay = srcData.output_frames_gen;
			if (m_reversed) 
			{
				std::reverse(dst, dst + numFramesToPlay);
			}
		}

		std::fill_n(dst + numFramesToPlay, numFramesRequested - numFramesToPlay, sampleFrame{0, 0});
		return true;
	}

	bool Sample::play(sampleFrame *dst, const int numFramesRequested, const float frequencyToPlay, int loopStart, int loopEnd, const LoopPlayback loopPlayback) 
	{
		rescaleMarkers(m_startFrame, m_endFrame);
		rescaleMarkers(loopStart, loopEnd);

		auto boundedLoopStart = std::clamp(m_startFrame, loopStart, m_endFrame);
		auto boundedLoopEnd = std::clamp(m_startFrame, loopEnd, m_endFrame);
		if (loopStart != boundedLoopStart || loopEnd != boundedLoopEnd) { return false; }

		const auto numFramesToCopy = std::min(loopEnd - loopStart, numFramesRequested);
		const auto [numFrameBatches, remainingFrames] = std::div(numFramesRequested, numFramesToCopy);

		if (loopPlayback == LoopPlayback::LoopPoints)
		{
			for (int i = 0; i < numFrameBatches; ++i)
			{
				m_frameIndex = loopStart;
				play(dst + i * numFramesToCopy, numFramesToCopy, frequencyToPlay);
			}

			m_frameIndex = loopStart;
			play(dst + numFrameBatches * numFramesToCopy, remainingFrames, frequencyToPlay);
		}
		else if (loopPlayback == LoopPlayback::PingPong) 
		{
			for (int i = 0; i < numFrameBatches; ++i)
			{
				if (i % 2 == 1)
				{
					m_reversed = !m_reversed;
					m_frameIndex = loopEnd - m_frameIndex;
				}
				else 
				{
					m_frameIndex = loopStart;
				}

				play(dst + i * numFramesToCopy, numFramesToCopy, frequencyToPlay);
			}

			m_frameIndex = loopStart;
			play(dst + numFrameBatches * numFramesToCopy, remainingFrames, frequencyToPlay);

			m_reversed = !m_reversed;
			m_frameIndex = loopEnd - m_frameIndex;
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
	void Sample::visualize(QPainter& painter, const QRect& drawingRect, int fromFrame, int toFrame, const float amplification)
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
			const float sqrtRmsData = std::sqrt(trueRmsData);
			const float maxRmsData = std::clamp(minData, sqrtRmsData, maxData);
			const float minRmsData = std::clamp(minData, -sqrtRmsData, maxData);

			// If nbFrames >= w, we can use curPixel to calculate X
			// but if nbFrames < w, we need to calculate it proportionally
			// to the total number of points
			auto x = nbFrames >= w ? xb + curPixel : xb + ((static_cast<double>(curPixel) / nbFrames) * w);
			// Partial Y calculation
			auto py = ySpace * amplification;
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

	QString Sample::toBase64() const
	{
		// TODO: Base64 encoding without the use of Qt
		const char* rawData = reinterpret_cast<const char*>(m_sampleBuffer.get());
		QByteArray data = QByteArray(rawData, sizeof(SampleBufferV2));
		return data.toBase64().constData();
	}

	bool Sample::rescaleMarkers(int& start, int& end) 
	{
		const auto audioEngineSampleRate = Engine::audioEngine()->processingSampleRate();
		if (m_sampleRate != audioEngineSampleRate) 
		{
			const auto sampleRateRatio = static_cast<float>(audioEngineSampleRate) / m_sampleRate;
			start = std::clamp(0, static_cast<int>(start * sampleRateRatio), numFrames());
			end = std::clamp(0, static_cast<int>(end * sampleRateRatio), numFrames());
			m_sampleRate = audioEngineSampleRate;

			return true;
		}

		return false;
	}

	QString Sample::sampleFile() const 
	{
		return m_sampleBuffer->filePath().value_or("");
	}

	std::shared_ptr<const SampleBufferV2> Sample::sampleBuffer() const 
	{
		return m_sampleBuffer;
	}
	
	bool Sample::reversed() const 
	{
		return m_reversed;
	}
	
	int Sample::startFrame() const 
	{
		return m_startFrame;
	}

	int Sample::endFrame() const 
	{
		return m_endFrame;
	}
	
	int Sample::frameIndex() const
	{
		return m_frameIndex;
	}

	int Sample::sampleRate() const 
	{
		return m_sampleRate;
	}

	int Sample::numFrames() const 
	{
		return m_sampleBuffer ? m_sampleBuffer->numFrames() : 0;
	}
	
	void Sample::setSampleBuffer(const SampleBufferV2* buffer)
	{
		m_sampleBuffer = std::shared_ptr<const SampleBufferV2>(buffer);
	}
	
	void Sample::setReversed(const bool reversed) 
	{
		m_reversed = reversed;
		m_frameIndex = m_endFrame - m_frameIndex;
	}
	
	void Sample::setStartFrame(const int start) 
	{
		m_startFrame = start;
	}
	
	void Sample::setEndFrame(const int end) 
	{
		m_endFrame = end;
	}

	void Sample::setFrameIndex(const int frameIndex)
	{
		m_frameIndex = frameIndex;
	}

	void Sample::setSampleRate(const int sampleRate) 
	{
		m_sampleRate = sampleRate;
	}
}; // namespace lmms
