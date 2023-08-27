/*
 * SampleBuffer2.cpp - container-class SampleBuffer2
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

#include "SampleBuffer2.h"

#include <QFile>
#include <QFileInfo>
#include <QIODevice>
#include <algorithm>
#include <array>
#include <iostream>
#include <memory>
#include <samplerate.h>
#include <shared_mutex>
#include <sndfile.h>
#include <stdexcept>
#include <string>

#include "AudioEngine.h"
#include "DrumSynth.h"
#include "Engine.h"
#include "PathUtil.h"

namespace lmms {

SampleBuffer2::SampleBuffer2(const sampleFrame* data, int numFrames, int sampleRate)
	: m_data(data, data + numFrames)
	, m_sampleRate(sampleRate)
{
}

SampleBuffer2::SampleBuffer2(const QString& audioFile)
{
	if (audioFile.isEmpty()) { throw std::runtime_error{"Failure loading audio file: Audio file path is empty."}; }

	auto resolvedFileName = PathUtil::toAbsolute(PathUtil::toShortestRelative(audioFile));
	QFileInfo{resolvedFileName}.suffix() == "ds" ? decodeSampleDS(resolvedFileName) : decodeSampleSF(resolvedFileName);
}

SampleBuffer2::SampleBuffer2(const QByteArray& base64Data, int sampleRate)
	: m_data(reinterpret_cast<const sampleFrame*>(base64Data.data()),
		reinterpret_cast<const sampleFrame*>(base64Data.data()) + base64Data.size() / sizeof(sampleFrame))
	, m_sampleRate(sampleRate)
{
}

void swap(SampleBuffer2& first, SampleBuffer2& second) noexcept
{
	using std::swap;
	swap(first.m_data, second.m_data);
	swap(first.m_audioFile, second.m_audioFile);
	swap(first.m_sampleRate, second.m_sampleRate);
}

void SampleBuffer2::decodeSampleSF(const QString& audioFile)
{
	SNDFILE* sndFile = nullptr;
	auto sfInfo = SF_INFO{};

	// Use QFile to handle unicode file names on Windows
	auto file = QFile{audioFile};
	if (!file.open(QIODevice::ReadOnly))
	{
		throw std::runtime_error{
			"Failed to open sample " + audioFile.toStdString() + ": " + file.errorString().toStdString()};
	}

	sndFile = sf_open_fd(file.handle(), SFM_READ, &sfInfo, false);
	if (sf_error(sndFile) != 0)
	{
		throw std::runtime_error{"Failure opening audio handle: " + std::string{sf_strerror(sndFile)}};
	}

	auto buf = std::vector<sample_t>(sfInfo.channels * sfInfo.frames);
	sf_read_float(sndFile, buf.data(), buf.size());

	sf_close(sndFile);
	file.close();

	auto result = std::vector<sampleFrame>(sfInfo.frames);
	for (int i = 0; i < static_cast<int>(result.size()); ++i)
	{
		if (sfInfo.channels == 1)
		{
			// Upmix from mono to stereo
			result[i] = {buf[i], buf[i]};
		}
		else if (sfInfo.channels > 1)
		{
			// TODO: Add support for higher number of channels (i.e., 5.1 channel systems)
			// The current behavior assumes stereo in all cases excluding mono.
			// This may not be the expected behavior, given some audio files with a higher number of channels.
			result[i] = {buf[i * sfInfo.channels], buf[i * sfInfo.channels + 1]};
		}
	}

	m_data = result;
	m_audioFile = audioFile;
	m_sampleRate = sfInfo.samplerate;
}

void SampleBuffer2::decodeSampleDS(const QString& audioFile)
{
	auto data = std::unique_ptr<int_sample_t>{};
	int_sample_t* dataPtr = nullptr;

	auto ds = DrumSynth{};
	const auto engineRate = Engine::audioEngine()->processingSampleRate();
	const auto frames = ds.GetDSFileSamples(audioFile, dataPtr, DEFAULT_CHANNELS, engineRate);
	data.reset(dataPtr);

	auto result = std::vector<sampleFrame>(frames);
	if (frames > 0 && data != nullptr)
	{
		src_short_to_float_array(data.get(), &result[0][0], frames * DEFAULT_CHANNELS);
	}
	else { throw std::runtime_error{"Decoding failure: failed to decode DrumSynth file."}; }

	m_data = result;
	m_audioFile = audioFile;
	m_sampleRate = engineRate;
}

QString SampleBuffer2::toBase64() const
{
	// TODO: Replace with non-Qt equivalent
	const auto data = reinterpret_cast<const char*>(m_data.data());
	const auto size = static_cast<int>(m_data.size() * sizeof(sampleFrame));
	const auto byteArray = QByteArray{data, size};
	return byteArray.toBase64();
}

auto SampleBuffer2::audioFile() const -> QString
{
	return m_audioFile.value_or("");
}

auto SampleBuffer2::sampleRate() const -> sample_rate_t
{
	return m_sampleRate;
}

auto SampleBuffer2::begin() const -> const_iterator
{
	return m_data.begin();
}

auto SampleBuffer2::end() const -> const_iterator
{
	return m_data.end();
}

auto SampleBuffer2::rbegin() const -> const_reverse_iterator
{
	return m_data.rbegin();
}

auto SampleBuffer2::rend() const -> const_reverse_iterator
{
	return m_data.rend();
}

auto SampleBuffer2::data() const -> const sampleFrame*
{
	return m_data.data();
}

auto SampleBuffer2::size() const -> size_type
{
	return m_data.size();
}

auto SampleBuffer2::empty() const -> bool
{
	return m_data.empty();
}

} // namespace lmms