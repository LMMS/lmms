/*
 * Sample.h - a SampleBuffer with its own characteristics
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

#ifndef SAMPLE_H
#define SAMPLE_H

#include <QPainter>
#include <QRect>
#include <memory>
#include <samplerate.h>
#include <string>

#include "Note.h"
#include "SampleBufferV2.h"

namespace lmms 
{
	class Sample
	{
	public:
		enum class LoopPlayback 
		{
			LoopPoints,
			PingPong
		};

		Sample() = default;
		Sample(const QString& sampleData, bool isBase64 = false);
		Sample(const sampleFrame* data, const int numFrames);
		explicit Sample(const SampleBufferV2* buffer);
		explicit Sample(const int numFrames);
		Sample(const Sample& other);
		Sample(Sample&& other);
		~Sample();
		
		Sample& operator=(const Sample& other);
		Sample& operator=(Sample&& other);
		
		bool play(sampleFrame *dst, const int numFramesRequested, const float frequencyToPlay);
		bool play(sampleFrame *dst, const int numFramesRequested, const float frequencyToPlay, int loopStart, int loopEnd, const LoopPlayback loopPlayback);
		void visualize(QPainter& painter, const QRect& drawingRect, const int fromFrame = 0, const int toFrame = 0, const float amplification = 1.0f);
		
		QString toBase64() const;
		bool rescaleMarkers(int& start, int& end);
		
		QString sampleFile() const;
		std::shared_ptr<const SampleBufferV2> sampleBuffer() const;
		bool reversed() const;
		int startFrame() const;
		int endFrame() const;
		int frameIndex() const;
		int sampleRate() const;
		int numFrames() const;
		
		void setSampleBuffer(const SampleBufferV2* buffer);
		void setReversed(const bool reversed);
		void setStartFrame(const int start);
		void setEndFrame(const int end);
		void setFrameIndex(const int frameIndex);
		void setSampleRate(const int sampleRate);

	private:
		std::shared_ptr<const SampleBufferV2> m_sampleBuffer;
		bool m_reversed = false;
		int m_startFrame = 0;
		int m_endFrame = 0;
		int m_frameIndex = 0;
		int m_sampleRate = Engine::audioEngine()->processingSampleRate();
		SRC_STATE* m_resampleState = nullptr;
	};
}

#endif