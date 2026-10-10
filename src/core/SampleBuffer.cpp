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

#include <QDebug>
#include <QMessageBox>
#include <cstring>

#include "PathUtil.h"
#include "SampleDecoder.h"

namespace lmms {

SampleBuffer::SampleBuffer(ch_cnt_t channels, f_cnt_t frames, sample_rate_t sampleRate, const QString& audioFile)
	: m_data(frames * channels)
	, m_channels{channels}
	, m_sampleRate{sampleRate}
	, m_audioFile(audioFile)
{
	assert(channels > 0 && "channel count must be greater than 0");
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

auto SampleBuffer::fromFile(const QString& filePath) -> std::optional<SampleBuffer>
{
	const auto absolutePath = PathUtil::toAbsolute(filePath);
	const auto storedPath = PathUtil::toShortestRelative(filePath);
	return SampleDecoder::decode(absolutePath);
}

auto SampleBuffer::fromBase64(const QString& str, ch_cnt_t channels, sample_rate_t sampleRate) -> std::optional<SampleBuffer>
{
	const auto result = QByteArray::fromBase64Encoding(str.toUtf8(), QByteArray::AbortOnBase64DecodingErrors);
	if (!result || result.decoded.size() % sizeof(SampleFrame) != 0) { return std::nullopt; }

	auto buffer = SampleBuffer{channels, result.decoded.size() / sizeof(SampleFrame), sampleRate};
	std::memcpy(buffer.data(), result.decoded.data(), result.decoded.size());
	return buffer;
}

} // namespace lmms
