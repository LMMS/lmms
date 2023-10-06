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

#include "PathUtil.h"
#include "SampleDecoder.h"

namespace lmms {

SampleBuffer::SampleBuffer(const sampleFrame* data, int numFrames, int sampleRate)
	: m_data(data, data + numFrames)
	, m_sampleRate(sampleRate)
{
}

SampleBuffer::SampleBuffer(const QString& audioFile)
{
	if (audioFile.isEmpty()) { throw std::runtime_error{"Failure loading audio file: Audio file path is empty."}; }
	auto resolvedFileName = PathUtil::toAbsolute(PathUtil::toShortestRelative(audioFile));

	auto [data, sampleRate] = SampleDecoder::decode(resolvedFileName);
	m_data = std::move(data);
	m_sampleRate = sampleRate;
	m_audioFile = audioFile;
}

SampleBuffer::SampleBuffer(const QByteArray& base64Data, int sampleRate)
	: m_data(reinterpret_cast<const sampleFrame*>(base64Data.data()),
		reinterpret_cast<const sampleFrame*>(base64Data.data()) + base64Data.size() / sizeof(sampleFrame))
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
	const auto size = static_cast<int>(m_data.size() * sizeof(sampleFrame));
	const auto byteArray = QByteArray{data, size};
	return byteArray.toBase64();
}

auto SampleBuffer::audioFile() const -> const QString&
{
	return m_audioFile;
}

auto SampleBuffer::sampleRate() const -> sample_rate_t
{
	return m_sampleRate;
}

auto SampleBuffer::begin() const -> const_iterator
{
	return m_data.begin();
}

auto SampleBuffer::end() const -> const_iterator
{
	return m_data.end();
}

auto SampleBuffer::rbegin() const -> const_reverse_iterator
{
	return m_data.rbegin();
}

auto SampleBuffer::rend() const -> const_reverse_iterator
{
	return m_data.rend();
}

auto SampleBuffer::data() const -> const sampleFrame*
{
	return m_data.data();
}

auto SampleBuffer::size() const -> size_type
{
	return m_data.size();
}

auto SampleBuffer::empty() const -> bool
{
	return m_data.empty();
}

} // namespace lmms
