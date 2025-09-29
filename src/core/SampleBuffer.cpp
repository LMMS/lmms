/*
 * SampleBuffer.cpp - container-class SampleBuffer
 *
 * Copyright (c) 2005-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include "SampleBuffer.h"

#include <cstring>
#include <fstream>

#include "AudioFile.h"
#include "DrumSynth.h"
#include "PathUtil.h"
#include "Song.h"

namespace {
using namespace lmms;

auto isDrumSynthPreset(const std::filesystem::path& path) -> bool
{
	auto f = std::ifstream{path};
	if (!f) { return false; }

	auto line = std::string{};

	std::getline(f, line);
	if (line != "[General]") { return false; }

	std::getline(f, line);
	return line.find("Version=DrumSynth") != std::string::npos;
}

auto loadFromDrumSynthPreset(const std::filesystem::path& path) -> std::unique_ptr<SampleBuffer>
{
	auto data = static_cast<int16_t*>(nullptr);
	const auto sampleRate = Engine::audioEngine()->outputSampleRate();
	const auto frames = DrumSynth{}.GetDSFileSamples(PathUtil::fsConvert(path), data, 2, sampleRate);

	auto buffer = std::vector<SampleFrame>(frames);
	for (auto frame = 0; frame < frames; ++frame)
	{
		const auto left = std::clamp(data[frame * 2] / OUTPUT_SAMPLE_MULTIPLIER, -1.0f, 1.0f);
		const auto right = std::clamp(data[frame * 2 + 1] / OUTPUT_SAMPLE_MULTIPLIER, -1.0f, 1.0f);
		buffer[frame] = {left, right};
	}

	delete data;
	return std::make_unique<SampleBuffer>(std::move(buffer), sampleRate);
}

auto loadFromAudioFile(const std::filesystem::path& path) -> std::unique_ptr<SampleBuffer>
{
	auto audioFile = AudioFile{path};

	if (audioFile.channels() == 2)
	{
		auto buffer = std::vector<SampleFrame>(audioFile.frames());
		audioFile.read({&buffer[0][0], 2, audioFile.frames()});
		return std::make_unique<SampleBuffer>(std::move(buffer), audioFile.sampleRate());
	}
	else if (audioFile.channels() == 1)
	{
		auto buffer = std::vector<SampleFrame>(audioFile.frames());
		auto monoBuffer = std::vector<float>(audioFile.frames());
		audioFile.read({&monoBuffer[0], 1, audioFile.frames()});
		std::transform(monoBuffer.begin(), monoBuffer.end(), buffer.begin(),
			[](auto& sample) { return SampleFrame{sample, sample}; });
		return std::make_unique<SampleBuffer>(std::move(buffer), audioFile.sampleRate());
	}
	else
	{
		auto dst = std::vector<SampleFrame>(audioFile.frames());
		auto src = std::vector<float>(audioFile.frames() * audioFile.channels());
		audioFile.read({&src[0], audioFile.channels(), audioFile.frames()});

		for (auto frame = 0; frame < audioFile.frames(); ++frame)
		{
			dst[frame][0] = src[frame * audioFile.channels()];
			dst[frame][1] = src[frame * audioFile.channels() + 1];
		}

		return std::make_unique<SampleBuffer>(std::move(dst), audioFile.sampleRate());
	}
}
} // namespace

namespace lmms {

SampleBuffer::SampleBuffer(std::vector<SampleFrame> data, int sampleRate)
	: m_data(std::move(data))
	, m_sampleRate(sampleRate)
{
}

SampleBuffer::SampleBuffer(const SampleFrame* data, size_t numFrames, int sampleRate)
	: m_data(data, data + numFrames)
	, m_sampleRate(sampleRate)
{
}

void swap(SampleBuffer& first, SampleBuffer& second) noexcept
{
	using std::swap;
	swap(first.m_data, second.m_data);
	swap(first.m_audioFile, second.m_audioFile);
	swap(first.m_sampleRate, second.m_sampleRate);
}

QString SampleBuffer::toBase64() const
{
	// TODO: Replace with non-Qt equivalent
	const auto data = reinterpret_cast<const char*>(m_data.data());
	const auto size = static_cast<int>(m_data.size() * sizeof(SampleFrame));
	const auto byteArray = QByteArray{data, size};
	return byteArray.toBase64();
}

auto SampleBuffer::emptyBuffer() -> std::shared_ptr<const SampleBuffer>
{
	static auto s_buffer = std::make_shared<const SampleBuffer>();
	return s_buffer;
}

auto SampleBuffer::createFromFile(const QString& path) -> std::shared_ptr<const SampleBuffer>
{
	try
	{
		auto fsPath = PathUtil::fsConvert(path);
		if (isDrumSynthPreset(fsPath)) { return loadFromDrumSynthPreset(fsPath); }
		return loadFromAudioFile(fsPath);
	}
	catch (const std::runtime_error& error)
	{
		auto message = Song::tr("Failed to load %1: %2").arg(path, error.what());
		Engine::getSong()->handleError(message);
		return SampleBuffer::emptyBuffer();
	}
}

auto SampleBuffer::createFromBase64(const QString& base64Str, int sampleRate) -> std::shared_ptr<const SampleBuffer>
{
	// TODO: Replace with non-Qt equivalent
	const auto byteArray = QByteArray::fromBase64(base64Str.toUtf8());
	auto buffer = std::vector<SampleFrame>(byteArray.size() / sizeof(SampleFrame));
	std::memcpy(buffer.data(), byteArray.data(), byteArray.size());
	return std::make_unique<SampleBuffer>(std::move(buffer), Engine::audioEngine()->outputSampleRate());
}

} // namespace lmms
