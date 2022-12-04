/*
 * SampleBufferV2.h - container class for immutable sample data
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

#ifndef SAMPLE_BUFFER_V2_H
#define SAMPLE_BUFFER_V2_H

#include <memory>
#include <optional>
#include <vector>

#include "AudioEngine.h"
#include "Engine.h"

namespace lmms 
{
	class SampleBufferV2 : public QObject
	{
	public:
		SampleBufferV2(const QString& sampleData, bool isBase64);
		SampleBufferV2(const sampleFrame* data, const int numFrames);
		explicit SampleBufferV2(const int numFrames);
		SampleBufferV2(const SampleBufferV2& other);
		SampleBufferV2(SampleBufferV2&& other);

		SampleBufferV2& operator=(const SampleBufferV2& other);
		SampleBufferV2& operator=(SampleBufferV2&& other);

		const std::vector<sampleFrame>& sampleData() const;
		const std::optional<QString>& filePath() const;
		sample_rate_t originalSampleRate() const;
		sample_rate_t currentSampleRate() const;
		int numFrames() const;

	private:
		void loadFromSampleFile(const QString& sampleFilePath);
		void loadFromDrumSynthFile(const QString& drumSynthFilePath);
		void loadFromBase64(const QString& base64);
		void resample(const int newSampleRate);
		
	private:
		std::vector<sampleFrame> m_sampleData;
		std::optional<QString> m_filePath;
		sample_rate_t m_originalSampleRate = 0;
		sample_rate_t m_currentSampleRate = 0;
	};
}

#endif