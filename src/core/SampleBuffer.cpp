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

#include "PathUtil.h"
#include "SampleDecoder.h"

namespace lmms {

SampleBuffer::SampleBuffer(const SampleFrame* data, size_t numFrames, int sampleRate)
	: m_data(data, data + numFrames)
	, m_sampleRate(sampleRate)
{
}

SampleBuffer::SampleBuffer(const QString& audioFile)
{
	if (audioFile.isEmpty()) { throw std::runtime_error{"Failure loading audio file: Audio file path is empty."}; }
	const auto absolutePath = PathUtil::toAbsolute(audioFile);

	if (auto decodedResult = SampleDecoder::decode(absolutePath))
	{
		auto& [data, sampleRate] = *decodedResult;
		m_data = std::move(data);
		m_sampleRate = sampleRate;
		m_audioFile = PathUtil::toShortestRelative(audioFile);
		return;
	}

	throw std::runtime_error{
		"Failed to decode audio file: Either the audio codec is unsupported, or the file is corrupted."};
}

SampleBuffer::SampleBuffer(const QString& base64, int sampleRate)
	: m_sampleRate(sampleRate)
{
	// TODO: Replace with non-Qt equivalent
	const auto bytes = QByteArray::fromBase64(base64.toUtf8());
	m_data.resize(bytes.size() / sizeof(SampleFrame));
	std::memcpy(reinterpret_cast<char*>(m_data.data()), bytes, m_data.size() * sizeof(SampleFrame));
}

SampleBuffer::SampleBuffer(std::vector<SampleFrame> data, int sampleRate)
	: m_data(std::move(data))
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

} // namespace lmms
