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

#include "GuiApplication.h"
#include "PathUtil.h"
#include "SampleDecoder.h"

namespace lmms {

SampleBuffer::SampleBuffer(AudioBuffer data, sample_rate_t sampleRate, const QString& audioFile)
	: m_data(std::move(data))
	, m_audioFile(audioFile)
	, m_sampleRate(sampleRate)
{
}

SampleBuffer::SampleBuffer(PlanarBufferSpan<const float> data, sample_rate_t sampleRate, const QString& audioFile)
	: m_data(data.frames(), data.channels())
	, m_audioFile(audioFile)
	, m_sampleRate(sampleRate)
{
	MixHelpers::copy(PlanarBufferSpan{m_data.allBuffers()}, data);
}

SampleBuffer::SampleBuffer(std::span<const SampleFrame> data, sample_rate_t sampleRate, const QString& audioFile)
	: m_data(data.size(), 2)
	, m_audioFile(audioFile)
	, m_sampleRate(sampleRate)
{
	toPlanar(InterleavedBufferSpan{data}, m_data.allBuffers());
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

	// Planar data is serialized as:
	//     [B64FrameCount, B64ChannelCount, float_ch0_f0, float_ch0_f1, ..., float_ch0_fN,
	//                                      float_ch1_f0, float_ch1_f1, ..., float_ch1_fN, ...
	//                                      float_chN_f0, float_chN_f1, ..., float_chN_fN]
	auto byteArray = QByteArray{};

	const auto frames = static_cast<B64FrameCount>(m_data.frames());
	const auto channels = static_cast<B64ChannelCount>(m_data.totalChannels());

	byteArray.append(reinterpret_cast<const char*>(frames), sizeof(B64FrameCount));
	byteArray.append(reinterpret_cast<const char*>(channels), sizeof(B64ChannelCount));

	for (ch_cnt_t ch = 0; ch < channels; ++ch)
	{
		byteArray.append(reinterpret_cast<const char*>(m_data.buffer(ch).data()), frames * sizeof(float));
	}

	return byteArray.toBase64();
}

auto SampleBuffer::emptyBuffer() -> std::shared_ptr<const SampleBuffer>
{
	static auto s_buffer = std::make_shared<const SampleBuffer>();
	return s_buffer;
}

std::shared_ptr<const SampleBuffer> SampleBuffer::fromFile(const QString& path)
{
	if (path.isEmpty()) { return SampleBuffer::emptyBuffer(); }

	const auto absolutePath = PathUtil::toAbsolute(path);
	const auto storedPath = PathUtil::toShortestRelative(path);

	auto result = SampleDecoder::decode(absolutePath);

	if (!result)
	{
		// TODO: Improve error handling. We dont always want to show a message box on failure when there is a GUI (e.g.
		// when loading the project), and this function also shouldn't be concerned with handling the error.
		if (gui::getGUI())
		{
			QMessageBox::warning(nullptr, QObject::tr("Failed to load sample"),
				QObject::tr("The sample may be corrupted or unsupported."));
		}
		else
		{
			qWarning() << QObject::tr(
				"Failed to load sample at path %1, the file may not exist, be corrupted, or is unsupported.")
							  .arg(absolutePath);
		}

		return SampleBuffer::emptyBuffer();
	}

	auto& [data, sampleRate] = *result;
	return std::make_shared<SampleBuffer>(std::move(data), sampleRate, storedPath);
}

std::shared_ptr<const SampleBuffer> SampleBuffer::fromBase64(const QString& str, sample_rate_t sampleRate)
{
	return fromBase64(false, str, sampleRate);
}

std::shared_ptr<const SampleBuffer> SampleBuffer::fromLegacyBase64(const QString& str, sample_rate_t sampleRate)
{
	return fromBase64(true, str, sampleRate);
}

std::shared_ptr<const SampleBuffer> SampleBuffer::fromBase64(bool legacyInterleaved,
	const QString& str, sample_rate_t sampleRate)
{
	if (str.isEmpty()) { return SampleBuffer::emptyBuffer(); }

	const auto bytes = QByteArray::fromBase64(str.toUtf8());
	const auto bytesSize = static_cast<std::size_t>(bytes.size());

	// NOTE: Interleaved and planar data is serialized differently.
	//
	// Interleaved data is serialized as:
	//     [SampleFrame_f0, SampleFrame_f1, ..., SampleFrame_fN]
	//
	// Planar data is serialized as:
	//     [B64FrameCount, B64ChannelCount, float_ch0_f0, float_ch0_f1, ..., float_ch0_fN,
	//                                      float_ch1_f0, float_ch1_f1, ..., float_ch1_fN, ...
	//                                      float_chM_f0, float_chM_f1, ..., float_chM_fN]

	const auto dataSize = legacyInterleaved
		? bytesSize
		: bytesSize - sizeof(B64FrameCount) - sizeof(B64ChannelCount);

	const bool invalid = legacyInterleaved
		? dataSize % sizeof(SampleFrame) != 0
		: (bytesSize < sizeof(B64FrameCount) + sizeof(B64ChannelCount) || dataSize % sizeof(float) != 0);

	if (invalid)
	{
		// TODO: Improve error handling. We dont always want to show a message box on failure when there is a GUI (e.g.
		// when loading the project), and this function also shouldn't be concerned with handling the error.
		if (gui::getGUI())
		{
			QMessageBox::warning(
				nullptr, QObject::tr("Failed to load sample"), QObject::tr("The sample size is invalid."));
		}
		else
		{
			qWarning() << QObject::tr("Failed to load Base64 sample, invalid size");
		}

		return SampleBuffer::emptyBuffer();
	}

	const auto frames = legacyInterleaved
		? static_cast<f_cnt_t>(bytes.size() / sizeof(SampleFrame))
		: *reinterpret_cast<const B64FrameCount*>(bytes.data());

	const auto channels = legacyInterleaved
		? static_cast<B64ChannelCount>(2)
		: *reinterpret_cast<const B64ChannelCount*>(bytes.data() + sizeof(B64FrameCount));

	const char* dataStart = legacyInterleaved
		? bytes.data()
		: bytes.data() + sizeof(B64FrameCount) + sizeof(B64ChannelCount);

	if (!legacyInterleaved && dataSize != frames * channels * sizeof(float))
	{
		// TODO: Improve error handling. We dont always want to show a message box on failure when there is a GUI (e.g.
		// when loading the project), and this function also shouldn't be concerned with handling the error.
		if (gui::getGUI())
		{
			QMessageBox::warning(
				nullptr, QObject::tr("Failed to load sample"), QObject::tr("Checksum failed."));
		}
		else
		{
			qWarning() << QObject::tr("Failed to load Base64 sample, checksum failed");
		}

		return SampleBuffer::emptyBuffer();
	}

	auto data = AudioBuffer{frames, static_cast<ch_cnt_t>(channels)};
	if (legacyInterleaved)
	{
		const auto decoded = InterleavedBufferSpan{reinterpret_cast<const SampleFrame*>(dataStart), frames};
		toPlanar(decoded, data.allBuffers());
	}
	else
	{
		const auto dataBuffers = data.allBuffers();
		for (ch_cnt_t ch = 0; ch < channels; ++ch)
		{
			const auto channelBufferOffset = ch * frames * sizeof(float);
			const auto channelBuffer = std::span {
				reinterpret_cast<const float*>(dataStart + channelBufferOffset),
				frames
			};
			std::ranges::copy(channelBuffer, dataBuffers.bufferPtr(ch));
		}
	}

	return std::make_shared<SampleBuffer>(std::move(data), sampleRate);
}

} // namespace lmms
