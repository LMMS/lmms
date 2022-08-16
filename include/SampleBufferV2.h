/*
 * SampleBufferV2.h - container class for immutable sample data
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

#ifndef SAMPLE_BUFFER_V2_H
#define SAMPLE_BUFFER_V2_H

// TODO: Replace with #include <filesystem> when GHA fully supports it
#include <experimental/filesystem>

#include <memory>
#include <optional>
#include <vector>

#include "AudioEngine.h"
#include "Engine.h"
#include "lmms_basics.h"

namespace lmms 
{
	class SampleBufferV2
	{
	public:
		SampleBufferV2(const std::experimental::filesystem::path& sampleFile);
		SampleBufferV2(const sampleFrame* data, const int numFrames);
		explicit SampleBufferV2(const int numFrames);
		SampleBufferV2(const SampleBufferV2& other) = delete;
		SampleBufferV2(SampleBufferV2&& other);

		SampleBufferV2& operator=(SampleBufferV2& other) = delete;
		SampleBufferV2& operator=(SampleBufferV2&& other);

		const std::vector<sampleFrame>& sampleData() const;
		const std::optional<std::experimental::filesystem::path>& filePath() const;
		sample_rate_t sampleRate() const;
		int numFrames() const;

		static std::shared_ptr<const SampleBufferV2> loadFromBase64(const std::string& base64);

		/**
		 * @brief Convert a STL file path to a QString portably.
		 * 
		 * @param filePath 
		 * @return QString 
		 */
		static QString qStringFromFilePath(const std::experimental::filesystem::path& filePath);
		
		/**
		 * @brief Convert a QString to a STL file path portably.
		 * 
		 * @param str 
		 * @return std::experimental::filesystem::path 
		 */
		static std::experimental::filesystem::path qStringToFilePath(const QString& str);

	private:
		void loadFromSampleFile(const std::experimental::filesystem::path& sampleFilePath);
		void loadFromDrumSynthFile(const std::experimental::filesystem::path& drumSynthFilePath);
		
	private:
		std::vector<sampleFrame> m_sampleData;
		std::optional<std::experimental::filesystem::path> m_filePath;
		sample_rate_t m_sampleRate = Engine::audioEngine()->processingSampleRate();
	};
}

#endif