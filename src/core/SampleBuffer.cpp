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

// Use aliasing constructor for std::shared_ptr to share ownership of a newly allocated vector without double
// indirection when accessing samples (in the case of a vector in a shared pointer)
SampleBuffer::SampleBuffer(std::vector<SampleFrame> data, sample_rate_t sampleRate, const QString& path)
	: m_frames(data.size())
	, m_sampleRate(sampleRate)
	, m_path(path)
	, m_data(std::shared_ptr<SampleFrame[]>(data.data(), [v = std::move(data)](auto) {}))
{
}

QString SampleBuffer::toBase64() const
{
	// TODO: Replace with non-Qt equivalent
	const auto data = reinterpret_cast<const char*>(m_data.get());
	const auto size = static_cast<int>(m_frames * sizeof(SampleFrame));
	const auto byteArray = QByteArray::fromRawData(data, size);
	return byteArray.toBase64();
}

std::optional<SampleBuffer> SampleBuffer::fromFile(const QString& filePath)
{
	if (filePath.isEmpty()) { return std::nullopt; }

	const auto absolutePath = PathUtil::toAbsolute(filePath);
	const auto storedPath = PathUtil::toShortestRelative(filePath);

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

		return std::nullopt;
	}

	auto& [data, sampleRate] = *result;
	return SampleBuffer{std::move(data), sampleRate, storedPath};
}

std::optional<SampleBuffer> SampleBuffer::fromBase64(const QString& str, sample_rate_t sampleRate)
{
	if (str.isEmpty()) { return std::nullopt; }

	const auto bytes = QByteArray::fromBase64(str.toUtf8());

	if (bytes.size() % sizeof(SampleFrame) != 0)
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

		return std::nullopt;
	}

	auto data = std::vector<SampleFrame>(bytes.size() / sizeof(SampleFrame));
	std::memcpy(reinterpret_cast<char*>(data.data()), bytes, bytes.size());
	return SampleBuffer{std::move(data), sampleRate};
}

} // namespace lmms
