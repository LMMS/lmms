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
#include "lmms_basics.h"

namespace lmms {

SampleBuffer::SampleBuffer(Access, const sampleFrame* data, int numFrames, int sampleRate)
	: m_data(data, data + numFrames)
	, m_sampleRate(sampleRate)
{
}

SampleBuffer::SampleBuffer(Access, const QString& audioFile)
	: m_sourceType(Source::AudioFile)
{
	if (audioFile.isEmpty()) { throw std::runtime_error{"Failure loading audio file: Audio file path is empty."}; }
	auto absolutePath = PathUtil::toAbsolute(audioFile);

	if (auto decodedResult = SampleDecoder::decode(absolutePath))
	{
		auto& [data, sampleRate] = *decodedResult;
		m_data = std::move(data);
		m_sampleRate = sampleRate;
		m_source = std::move(absolutePath);
		return;
	}

	throw std::runtime_error{
		"Failed to decode audio file: Either the audio codec is unsupported, or the file is corrupted."};
}

SampleBuffer::SampleBuffer(Access, const QString& base64, int sampleRate)
	: m_source(base64)
	, m_sourceType(Source::Base64)
	, m_sampleRate(sampleRate)
{
	// TODO: Replace with non-Qt equivalent
	const auto bytes = QByteArray::fromBase64(base64.toUtf8());
	m_data.resize(bytes.size() / sizeof(sampleFrame));
	std::memcpy(reinterpret_cast<char*>(m_data.data()), bytes, m_data.size() * sizeof(sampleFrame));
}

SampleBuffer::SampleBuffer(Access, std::vector<sampleFrame> data, int sampleRate)
	: m_data(std::move(data))
	, m_sampleRate(sampleRate)
{
}

auto SampleBuffer::create() -> std::shared_ptr<const SampleBuffer>
{
	return std::shared_ptr<SampleBuffer>(new SampleBuffer{Access{}});
}

auto SampleBuffer::create(const QString& audioFile) -> std::shared_ptr<const SampleBuffer>
{
	return std::shared_ptr<SampleBuffer>(new SampleBuffer{Access{}, audioFile});
}

auto SampleBuffer::create(const QString& base64, int sampleRate) -> std::shared_ptr<const SampleBuffer>
{
	return std::shared_ptr<SampleBuffer>(new SampleBuffer{Access{}, base64, sampleRate});
}

auto SampleBuffer::create(std::vector<sampleFrame> data, int sampleRate) -> std::shared_ptr<const SampleBuffer>
{
	return std::shared_ptr<SampleBuffer>(new SampleBuffer{Access{}, std::move(data), sampleRate});
}

auto SampleBuffer::create(const sampleFrame* data, int numFrames, int sampleRate)
	-> std::shared_ptr<const SampleBuffer>
{
	return std::shared_ptr<SampleBuffer>(new SampleBuffer{Access{}, data, numFrames, sampleRate});
}

void swap(SampleBuffer& first, SampleBuffer& second) noexcept
{
	using std::swap;
	swap(first.m_data, second.m_data);
	swap(first.m_source, second.m_source);
	swap(first.m_sourceType, second.m_sourceType);
	swap(first.m_sampleRate, second.m_sampleRate);
}

/*
auto SampleBuffer::get() const -> std::shared_ptr<const SampleBuffer>
{
	return shared_from_this();
}*/

auto SampleBuffer::toBase64() const -> QString
{
	if (m_sourceType == Source::Base64)
	{
		return m_source;
	}

	// TODO: Replace with non-Qt equivalent
	const auto data = reinterpret_cast<const char*>(m_data.data());
	const auto size = static_cast<int>(m_data.size() * sizeof(sampleFrame));
	const auto byteArray = QByteArray{data, size};
	return byteArray.toBase64();
}

auto SampleBuffer::audioFileAbsolute() const -> const QString&
{
	static const QString empty;
	return m_sourceType == Source::AudioFile ? m_source : empty;
}

auto SampleBuffer::audioFileRelative() const -> QString
{
	return m_sourceType == Source::AudioFile ? PathUtil::toShortestRelative(m_source) : QString{};
}

auto SampleBuffer::base64() const -> const QString&
{
	static const QString empty;
	return m_sourceType == Source::Base64 ? m_source : empty;
}

} // namespace lmms
