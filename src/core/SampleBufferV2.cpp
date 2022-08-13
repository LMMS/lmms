/*
 * SampleBufferV2.cpp - container class for immutable sample data
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

#include "SampleBufferV2.h"

#include <DrumSynth.h>
#include <sndfile.h>
#include <stdexcept>

namespace lmms
{
	SampleBufferV2::SampleBufferV2(const std::string& strData, const StrDataType dataType)
	{
		if (strData.empty()) { throw std::runtime_error("SampleBufferV2.cpp: strData is empty."); }

		if (dataType == StrDataType::AudioFile)
		{
			auto audioFilePath = std::filesystem::path(strData);
			if (!std::filesystem::exists(audioFilePath))
			{
				throw std::runtime_error("SampleBufferV2.cpp: non existing file " + strData);
			}

			if (audioFilePath.extension() == ".ds") { loadFromDrumSynthFile(audioFilePath); }
			else
			{
				loadFromAudioFile(audioFilePath);
			}
		}
		else if (dataType == StrDataType::Base64)
		{
			loadFromBase64(strData);
		}
	}

	SampleBufferV2::SampleBufferV2(const sampleFrame* data, const int numFrames)
		: m_sampleData(data, data + numFrames)
		, m_filePath("")
	{
	}

	SampleBufferV2::SampleBufferV2(const int numFrames)
		: m_sampleData(numFrames)
		, m_filePath("")
	{
	}

	SampleBufferV2::SampleBufferV2(SampleBufferV2&& other)
		: m_sampleData(std::move(other.m_sampleData))
		, m_filePath(std::exchange(other.m_filePath, std::nullopt))
		, m_sampleRate(std::exchange(other.m_sampleRate, 0))
	{
		other.m_sampleData.clear();
	}

	SampleBufferV2& SampleBufferV2::operator=(SampleBufferV2&& other)
	{
		if (this == &other) { return *this; }

		m_sampleData = std::move(other.m_sampleData);
		m_filePath = std::exchange(other.m_filePath, std::nullopt);
		m_sampleRate = std::exchange(other.m_sampleRate, 0);
		other.m_sampleData.clear();

		return *this;
	}

	const std::vector<sampleFrame>& SampleBufferV2::sampleData() const
	{
		return m_sampleData;
	}

	const std::optional<std::filesystem::path>& SampleBufferV2::filePath() const
	{
		return m_filePath;
	}

	sample_rate_t SampleBufferV2::sampleRate() const
	{
		return m_sampleRate;
	}

	std::string SampleBufferV2::toBase64() const
	{
		const char* rawData = reinterpret_cast<const char*>(m_sampleData.data());
		QByteArray data = QByteArray(rawData, m_sampleData.size() * sizeof(sampleFrame));
		return data.toBase64().constData();
	}

	int SampleBufferV2::numFrames() const
	{
		return m_sampleData.size();
	}

	void SampleBufferV2::loadFromAudioFile(const std::filesystem::path& audioFilePath)
	{
		SF_INFO sfInfo;
		sfInfo.format = 0;

		auto sndFileDeleter = [](SNDFILE* ptr) { sf_close(ptr); };
		#ifdef LMMS_BUILD_WIN32
			auto sndFile = std::unique_ptr<SNDFILE, decltype(sndFileDeleter)>(
				sf_wchar_open(audioFilePath.c_str(), SFM_READ, &sfInfo), sndFileDeleter);
		#else
			auto sndFile = std::unique_ptr<SNDFILE, decltype(sndFileDeleter)>(
				sf_open(audioFilePath.c_str(), SFM_READ, &sfInfo), sndFileDeleter);
		#endif

		if (!sndFile) { throw std::runtime_error("Failed to open audio file: " + std::string{sf_strerror(sndFile.get())}); }

		auto numSamples = sfInfo.frames * sfInfo.channels;
		auto samples = std::vector<float>(numSamples);
		auto samplesRead = sf_read_float(sndFile.get(), samples.data(), numSamples);

		if (samplesRead != numSamples)
		{
			throw std::runtime_error("Failed to read audio samples: samplesRead != numSamples");
		}

		m_sampleData = std::vector<sampleFrame>(sfInfo.frames);
		m_sampleRate = sfInfo.samplerate;
		m_filePath = audioFilePath;

		for (sf_count_t frameIndex = 0; frameIndex < sfInfo.frames; ++frameIndex)
		{
			m_sampleData[frameIndex][0] = samples[frameIndex * sfInfo.channels];
			m_sampleData[frameIndex][1] = samples[frameIndex * sfInfo.channels + (sfInfo.channels > 1 ? 1 : 0)];
		}
	}

	void SampleBufferV2::loadFromDrumSynthFile(const std::filesystem::path& drumSynthFilePath)
	{
		auto dsFilePathStr = drumSynthFilePath.native();
		auto ds = DrumSynth();
		auto samples = std::make_unique<int16_t>();
		auto samplesRawPtr = samples.get();

		// TODO: Remove QString::fromStdString when Qt is removed from DrumSynth
		int numSamples = ds.GetDSFileSamples(QString::fromStdString(dsFilePathStr), samplesRawPtr,
			DEFAULT_CHANNELS, Engine::audioEngine()->processingSampleRate());

		if (numSamples == 0 || !samples)
		{
			throw std::runtime_error("Could not read DrumSynth file " + dsFilePathStr);
		}

		m_sampleData.resize(numSamples / DEFAULT_CHANNELS);
		m_filePath = drumSynthFilePath;

		for (int sampleIndex = 0; sampleIndex < numSamples; ++sampleIndex)
		{
			int frameIndex = sampleIndex / DEFAULT_CHANNELS;
			m_sampleData[frameIndex][sampleIndex % DEFAULT_CHANNELS]
				= samplesRawPtr[sampleIndex] * (1 / OUTPUT_SAMPLE_MULTIPLIER);
		}
	}

	void SampleBufferV2::loadFromBase64(const std::string& base64)
	{
		// TODO: Base64 decoding without the use of Qt
		QByteArray base64Data = QByteArray::fromBase64(QString::fromStdString(base64).toUtf8());
		sampleFrame* dataAsSampleFrame = reinterpret_cast<sampleFrame*>(base64Data.data());
		m_sampleData.assign(dataAsSampleFrame, dataAsSampleFrame + base64Data.size());
	}
}