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

SampleBuffer::SampleBuffer(const QString& audioFile)
{
	if (audioFile.isEmpty()) { throw std::runtime_error{"Failure loading audio file: Audio file path is empty."}; }
	const auto absolutePath = PathUtil::toAbsolute(audioFile);

	if (auto decodedResult = SampleDecoder::decode(absolutePath))
	{
		auto& [data, sampleRate] = *decodedResult;
		m_data = std::make_shared<const SampleBufferData>(
			std::move(data), PathUtil::toShortestRelative(audioFile), sampleRate);
		return;
	}

	throw std::runtime_error{
		"Failed to decode audio file: Either the audio codec is unsupported, or the file is corrupted."};
}

SampleBuffer::SampleBuffer(const QString& base64, sample_rate_t sampleRate)
{
	// TODO: Replace with non-Qt equivalent
	const auto bytes = QByteArray::fromBase64(base64.toUtf8());
	auto data = std::vector<SampleFrame>(bytes.size() / sizeof(SampleFrame)); 
	std::memcpy(reinterpret_cast<char*>(data.data()), bytes, m_data->data.size() * sizeof(SampleFrame));
	m_data = std::make_shared<const SampleBufferData>(std::move(data), std::nullopt, sampleRate);
}

SampleBuffer::SampleBuffer(const SampleFrame* data, f_cnt_t numFrames, sample_rate_t sampleRate)
	: m_data(std::make_shared<const SampleBufferData>(std::vector<SampleFrame>(data, data + numFrames), std::nullopt, sampleRate))
{
}

SampleBuffer::SampleBuffer(f_cnt_t numFrames, sample_rate_t sampleRate)
	: m_data(std::make_shared<const SampleBufferData>(std::vector<SampleFrame>(numFrames), std::nullopt, sampleRate))
{
}

auto SampleBuffer::toBase64() const -> QString
{
	// TODO: Replace with non-Qt equivalent
	const auto data = reinterpret_cast<const char*>(m_data->data.data());
	const auto size = static_cast<int>(m_data->data.size() * sizeof(SampleFrame));
	const auto byteArray = QByteArray{data, size};
	return byteArray.toBase64();
}

auto SampleBuffer::emptyData() -> std::shared_ptr<const SampleBufferData>
{
	static auto s_buffer = std::make_shared<const SampleBufferData>();
	return s_buffer;
}

} // namespace lmms
