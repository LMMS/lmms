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

SampleBuffer::SampleBuffer(f_cnt_t frames, sample_rate_t sampleRate, const QString& audioFile)
	: m_data(frames)
	, m_audioFile(audioFile)
	, m_sampleRate(sampleRate)
{
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

std::shared_ptr<const SampleBuffer> SampleBuffer::fromFile(const QString& filePath)
{
	const auto absolutePath = PathUtil::toAbsolute(filePath);
	const auto storedPath = PathUtil::toShortestRelative(filePath);

	auto result = SampleDecoder::decode(absolutePath);
	if (!result) { return nullptr; }

	return std::make_shared<const SampleBuffer>(std::move(*result));
}

std::shared_ptr<const SampleBuffer> SampleBuffer::fromBase64(const QString& str, sample_rate_t sampleRate)
{
	const auto result = QByteArray::fromBase64Encoding(str.toUtf8(), QByteArray::AbortOnBase64DecodingErrors);
	if (!result || result.decoded.size() % sizeof(SampleFrame) != 0) { return nullptr; }

	auto buffer = SampleBuffer{result.decoded.size() / sizeof(SampleFrame), sampleRate};
	std::memcpy(buffer.data(), result.decoded.data(), result.decoded.size());
	return std::make_shared<const SampleBuffer>(std::move(buffer));
}

} // namespace lmms
