/*
 * SampleBufferV2.cpp - container class for immutable sample data
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

#include "SampleBufferV2.h"
#include "SampleBufferCache.h"
#include "PathUtil.h"
#include "DrumSynth.h"

#include <QFileInfo>
#include <sndfile.h>
#include <stdexcept>
#include <iostream>

namespace lmms
{
	SampleBufferV2::SampleBufferV2(const QString& sampleData, bool isBase64) 
	{
		if (!isBase64) 
		{
			const auto absoluteSampleFilePath = PathUtil::toAbsolute(sampleData);
			const auto sampleFileInfo = QFileInfo{absoluteSampleFilePath};
			if (!sampleFileInfo.exists())
			{
				throw std::runtime_error{"SampleBufferV2.cpp: Sample file does not exist"};
			}

			sampleFileInfo.completeSuffix() == "ds" ? 
				loadFromDrumSynthFile(absoluteSampleFilePath) : 
				loadFromSampleFile(absoluteSampleFilePath);
		}
		else 
		{
			loadFromBase64(sampleData);
		}

		const auto audioEngineSampleRate = Engine::audioEngine()->processingSampleRate();
		if (m_currentSampleRate != audioEngineSampleRate) 
		{
			resample(audioEngineSampleRate);
		}

		connect(Engine::audioEngine(), &AudioEngine::sampleRateChanged, [this]()
		{
			resample(Engine::audioEngine()->processingSampleRate());
		});
	}

	SampleBufferV2::SampleBufferV2(const sampleFrame* data, const int numFrames)
		: m_sampleData(data, data + numFrames)
		, m_filePath("")
		, m_originalSampleRate(Engine::audioEngine()->processingSampleRate())
		, m_currentSampleRate(m_originalSampleRate)
	{
	}

	SampleBufferV2::SampleBufferV2(const int numFrames)
		: m_sampleData(numFrames)
		, m_filePath("")
		, m_originalSampleRate(Engine::audioEngine()->processingSampleRate())
		, m_currentSampleRate(m_originalSampleRate)
	{
	}

	SampleBufferV2::SampleBufferV2(const SampleBufferV2& other)
		: m_sampleData(other.m_sampleData)
		, m_filePath(other.m_filePath)
		, m_originalSampleRate(other.m_originalSampleRate)
		, m_currentSampleRate(other.m_currentSampleRate)
	{
	}

	SampleBufferV2::SampleBufferV2(SampleBufferV2&& other)
		: m_sampleData(std::move(other.m_sampleData))
		, m_filePath(std::exchange(other.m_filePath, std::nullopt))
		, m_originalSampleRate(std::exchange(other.m_originalSampleRate, 0))
		, m_currentSampleRate(std::exchange(other.m_currentSampleRate, 0))
	{
		other.m_sampleData.clear();
	}

	SampleBufferV2& SampleBufferV2::operator=(const SampleBufferV2& other) 
	{
		if (this == &other) { return *this; }

		m_sampleData = other.m_sampleData;
		m_filePath = other.m_filePath;
		m_originalSampleRate = other.m_originalSampleRate;
		m_currentSampleRate = other.m_currentSampleRate;

		return *this;
	}

	SampleBufferV2& SampleBufferV2::operator=(SampleBufferV2&& other)
	{
		if (this == &other) { return *this; }

		m_sampleData = std::move(other.m_sampleData);
		m_filePath = std::exchange(other.m_filePath, std::nullopt);
		m_originalSampleRate = std::exchange(other.m_originalSampleRate, 0);
		m_currentSampleRate = std::exchange(other.m_currentSampleRate, 0);
		other.m_sampleData.clear();

		return *this;
	}

	const std::vector<sampleFrame>& SampleBufferV2::sampleData() const
	{
		return m_sampleData;
	}

	const std::optional<QString>& SampleBufferV2::filePath() const
	{
		return m_filePath;
	}

	sample_rate_t SampleBufferV2::originalSampleRate() const
	{
		return m_originalSampleRate;
	}

	int SampleBufferV2::numFrames() const
	{
		return m_sampleData.size();
	}

	void SampleBufferV2::loadFromSampleFile(const QString& sampleFilePath)
	{
		SF_INFO sfInfo;
		sfInfo.format = 0;

		auto sampleFile = QFile{PathUtil::toAbsolute(sampleFilePath)};
		auto sndFileDeleter = [&](SNDFILE* ptr) 
		{ 
			sf_close(ptr);
			sampleFile.close();
		};

		if (!sampleFile.open(QIODevice::ReadOnly)) 
		{
			throw std::runtime_error{"SampleBufferV2.cpp: Could not open sample file."};
		}
		
		auto sndFile = std::unique_ptr<SNDFILE, decltype(sndFileDeleter)>(
				sf_open_fd(sampleFile.handle(), SFM_READ, &sfInfo, false), sndFileDeleter);

		if (!sndFile)
		{ 
			throw std::runtime_error{std::string{"SampleBufferV2.cpp: Failed to open sample file - "} 
				+ sf_strerror(sndFile.get())};
		}

		auto numSamples = sfInfo.frames * sfInfo.channels;
		auto samples = std::vector<float>(numSamples);
		auto samplesRead = sf_read_float(sndFile.get(), samples.data(), numSamples);

		if (samplesRead != numSamples)
		{
			throw std::runtime_error{"SampleBufferV2.cpp: samplesRead != numSamples"};
		}

		m_originalSampleRate = sfInfo.samplerate;
		m_currentSampleRate = sfInfo.samplerate;
		m_filePath = sampleFilePath;

		if (sfInfo.channels > 2) 
		{
			m_sampleData = mixDownToStereo(samples, sfInfo.channels);
		}
		else if (sfInfo.channels < 2) 
		{
			m_sampleData = mixMonoToStereo(samples);
		}
		else 
		{
			m_sampleData = std::vector<sampleFrame>(sfInfo.frames);
			std::copy(samples.begin(), samples.end(), m_sampleData.begin()->data());
		}
	}

	void SampleBufferV2::loadFromDrumSynthFile(const QString& drumSynthFilePath)
	{
		auto ds = DrumSynth();
		auto samples = std::make_unique<int16_t>();
		auto samplesRawPtr = samples.get();
		const auto audioEngineSampleRate = Engine::audioEngine()->processingSampleRate();

		int numSamples = ds.GetDSFileSamples(drumSynthFilePath, samplesRawPtr, DEFAULT_CHANNELS, audioEngineSampleRate);

		if (numSamples == 0 || !samples)
		{
			throw std::runtime_error{"SampleBufferV2.cpp: Could not read DrumSynth file."};
		}

		m_sampleData.resize(numSamples / DEFAULT_CHANNELS);
		m_filePath = drumSynthFilePath;
		m_originalSampleRate = audioEngineSampleRate;
		m_currentSampleRate = audioEngineSampleRate;

		src_short_to_float_array(samplesRawPtr, m_sampleData.data()->data(), m_sampleData.size());
	}

	void SampleBufferV2::loadFromBase64(const QString& base64)
	{
		// TODO: Base64 decoding without the use of Qt
		QByteArray base64Data = QByteArray::fromBase64(base64.toUtf8());
		*this = *reinterpret_cast<SampleBufferV2*>(base64Data.data());
	}

	void SampleBufferV2::resample(const int newSampleRate) 
	{
		const auto numOutputFrames = static_cast<f_cnt_t>(
			(numFrames() / static_cast<float>(m_currentSampleRate)) * 
			static_cast<float>(newSampleRate));
		auto outputFrames = std::vector<sampleFrame>(numOutputFrames);

		int error;
		SRC_STATE * state;
		if ((state = src_new(SRC_SINC_MEDIUM_QUALITY, DEFAULT_CHANNELS, &error)) != nullptr)
		{
			SRC_DATA srcData;
			srcData.end_of_input = 1;
			srcData.data_in = m_sampleData.data()->data();
			srcData.data_out = outputFrames.data()->data();
			srcData.input_frames = m_sampleData.size();
			srcData.output_frames = numOutputFrames;
			srcData.src_ratio = static_cast<double>(newSampleRate) / m_currentSampleRate;

			if ((error = src_process(state, &srcData)))
			{
				std::cerr << "Error in SampleBuffer.cpp::resample: Error while resampling: " << src_strerror(error) << '\n';
				src_delete(state);
				return;
			}

			src_delete(state);
		}
		else
		{
			std::cerr << "Error in SampleBuffer.cpp::resample: src_new() failed\n";
			return;
		}

		m_sampleData = outputFrames;
		m_currentSampleRate = newSampleRate;
	}

	std::vector<sampleFrame> SampleBufferV2::mixMonoToStereo(const std::vector<float>& data) 
	{
		auto result = std::vector<sampleFrame>(data.size());
		for (const auto sample : data)
		{
			result.push_back(sampleFrame{sample, sample});
		}

		return result;
	}
	
	std::vector<sampleFrame> SampleBufferV2::mixDownToStereo(const std::vector<float>& data, int numChannels)
	{
		if (numChannels < 2) { throw std::runtime_error{"Error in SampleBuffer.cpp::mixDownToStereo: numChannels is < 2"}; }

		auto result = std::vector<sampleFrame>(data.size() / numChannels);
		for (int i = 0; i < data.size(); i += numChannels)
		{
			const auto averageSample = std::accumulate(data.begin() + i, data.begin() + i + numChannels, 0.0f) / numChannels;
			result.push_back(sampleFrame{averageSample, averageSample});
		}

		return result;
	}

} // namespace lmms
