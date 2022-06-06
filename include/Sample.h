/*
 * Sample.h - a SampleBuffer with its own characteristics
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

#ifndef SAMPLE_H
#define SAMPLE_H

#include <QPainter>
#include <QRect>
#include <memory>
#include <samplerate.h>
#include <string>

#include "Note.h"
#include "SampleBufferCache.h"
#include "SampleBufferV2.h"
#include "lmms_basics.h"

namespace lmms 
{
	class Sample
	{
	public:
		enum class PlaybackType
		{
			Regular,
			LoopPoints,
			PingPong
		};

		Sample() = default;
		Sample(const std::string& strData, const SampleBufferV2::StrDataType dataType);
		Sample(const sampleFrame* data, const int numFrames);
		explicit Sample(const SampleBufferV2* buffer);
		explicit Sample(const int numFrames);
		Sample(const Sample& other);
		Sample(Sample&& other);

		Sample& operator=(Sample other);
		friend void swap(Sample& first, Sample& second);

		bool play(sampleFrame* dst, const int numFrames, const float freq);
		void visualize(QPainter& painter, const QRect& drawingRect, const int fromFrame = 0, const int toFrame = 0);

		std::string sampleFile() const;
		std::shared_ptr<const SampleBufferV2> sampleBuffer() const;
		int sampleRate() const;
		float amplification() const;
		float frequency() const;
		bool reversed() const;
		bool varyingPitch() const;
		int interpolationMode() const;
		int startFrame() const;
		int endFrame() const;
		int loopStartFrame() const;
		int loopEndFrame() const;
		int frameIndex() const;
		int numFrames() const;
		PlaybackType playback() const;

		void setSampleData(const std::string& str, const SampleBufferV2::StrDataType dataType);
		void setSampleBuffer(const SampleBufferV2* buffer);
		void setAmplification(const float amplification);
		void setFrequency(const float frequency);
		void setReversed(const bool reversed);
		void setVaryingPitch(const bool varyingPitch);
		void setInterpolationMode(const int interpolationMode);
		void setStartFrame(const int start);
		void setEndFrame(const int end);
		void setLoopStartFrame(const int loopStart);
		void setLoopEndFrame(const int loopEnd);
		void setFrameIndex(const int frameIndex);
		void setPlayback(const PlaybackType playback);

		void loadAudioFile(const std::string& audioFile);
		void loadBase64(const std::string& base64);
		void resetMarkers();
		int calculateTickLength() const;

	private:
		std::shared_ptr<const SampleBufferV2> m_sampleBuffer;
		float m_amplification = 1.0f;
		float m_frequency = DefaultBaseFreq;
		bool m_reversed = false;
		bool m_varyingPitch = false;
		bool m_pingPongBackwards = false;
		int m_interpolationMode = SRC_LINEAR;
		int m_startFrame = 0;
		int m_endFrame = 0;
		int m_loopStartFrame = 0;
		int m_loopEndFrame = 0;
		int m_frameIndex = 0;
		PlaybackType m_playback = PlaybackType::Regular;
		SRC_STATE* m_resampleState = nullptr;
	};
}

#endif