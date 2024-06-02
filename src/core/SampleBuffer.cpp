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

#include <QHash>
#include <cstring>

#include "PathUtil.h"
#include "SampleDecoder.h"
#include "lmms_basics.h"

namespace lmms {

SampleBuffer::Source::Source(const QString& filePath)
	: m_type(Type::AudioFile)
{
	if (filePath.isEmpty()) { throw std::runtime_error{"Audio file path is empty."}; }

	m_identifier = PathUtil::toShortestRelative(filePath);
	m_hash = qHash(m_identifier, static_cast<unsigned int>(m_type));
}

SampleBuffer::Source::Source(const QString& base64, sample_rate_t sampleRate)
	: m_type(Type::Base64)
{
	if (base64.isEmpty()) { throw std::runtime_error{"Base64 string is empty."}; }

	m_identifier = base64 + " " + QString::number(sampleRate);
	m_hash = qHash(base64, sampleRate);
}

auto SampleBuffer::Source::audioFileRelative() const -> const QString&
{
	static const QString empty;
	return m_type == Type::AudioFile ? m_identifier : empty;
}

auto SampleBuffer::Source::audioFileAbsolute() const -> QString
{
	return m_type == Type::AudioFile ? PathUtil::toAbsolute(m_identifier) : QString{};
}

SampleBuffer::SampleBuffer(Access, const QString& audioFile)
	: m_source(audioFile)
{
	if (auto decodedResult = SampleDecoder::decode(m_source.audioFileAbsolute()))
	{
		auto& [data, sampleRate] = *decodedResult;
		m_data = std::move(data);
		m_sampleRate = sampleRate;
		return;
	}

	throw std::runtime_error{
		"Failed to decode audio file: Either the audio codec is unsupported, or the file is corrupted."};
}

SampleBuffer::SampleBuffer(Access, const QString& base64, sample_rate_t sampleRate)
	: m_sampleRate(sampleRate)
	, m_source(base64, sampleRate)
{
	// TODO: Replace with non-Qt equivalent
	const auto bytes = QByteArray::fromBase64(base64.toUtf8());
	m_data.resize(bytes.size() / sizeof(sampleFrame));
	std::memcpy(reinterpret_cast<char*>(m_data.data()), bytes, m_data.size() * sizeof(sampleFrame));
}

SampleBuffer::SampleBuffer(Access, std::vector<sampleFrame> data, sample_rate_t sampleRate)
	: m_data(std::move(data))
	, m_sampleRate(sampleRate)
	, m_source()
{
}

SampleBuffer::SampleBuffer(Access, const sampleFrame* data, size_t numFrames, sample_rate_t sampleRate)
	: m_data(data, data + numFrames)
	, m_sampleRate(sampleRate)
{
}

auto SampleBuffer::create() -> std::shared_ptr<const SampleBuffer>
{
	return std::make_shared<SampleBuffer>(Access{});
}

auto SampleBuffer::create(const QString& audioFile) -> std::shared_ptr<const SampleBuffer>
{
	return std::make_shared<SampleBuffer>(Access{}, audioFile);
}

auto SampleBuffer::create(const QString& base64, sample_rate_t sampleRate) -> std::shared_ptr<const SampleBuffer>
{
	return std::make_shared<SampleBuffer>(Access{}, base64, sampleRate);
}

auto SampleBuffer::create(std::vector<sampleFrame> data, sample_rate_t sampleRate)
	-> std::shared_ptr<const SampleBuffer>
{
	return std::make_shared<SampleBuffer>(Access{}, std::move(data), sampleRate);
}

auto SampleBuffer::create(const sampleFrame* data, size_t numFrames, sample_rate_t sampleRate)
	-> std::shared_ptr<const SampleBuffer>
{
	return std::make_shared<SampleBuffer>(Access{}, data, numFrames, sampleRate);
}

void swap(SampleBuffer& first, SampleBuffer& second) noexcept
{
	using std::swap;
	swap(first.m_data, second.m_data);
	swap(first.m_sampleRate, second.m_sampleRate);
	swap(first.m_source, second.m_source);
}

auto SampleBuffer::toBase64() const -> QString
{
	// TODO: Replace with non-Qt equivalent
	const auto data = reinterpret_cast<const char*>(m_data.data());
	const auto size = static_cast<int>(m_data.size() * sizeof(sampleFrame));
	const auto byteArray = QByteArray{data, size};
	return byteArray.toBase64();
}

} // namespace lmms
