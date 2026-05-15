/*
 * SfzSamplePool.cpp - Helper class for handling loading of sample files
 *
 * Copyright (c) 2026 Keratin
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

#include "SfzSamplePool.h"
#include "SampleBuffer.h"
#include "PathUtil.h"
#include "SampleDecoder.h"

#include <QDebug>

namespace lmms
{


const SfzSampleBuffer* SfzSamplePool::loadSample(const QString& path)
{
	// If the sample has already been loaded before, just return a pointer to it
	if (m_samplePool.contains(path))
	{
		return m_samplePool.at(path).get();
	}

	// Copied from SampleBuffer.cpp
	// This is done instead of calling SampleBuffer::fromFile, since that function creates a warning box/window which the user needs to close, every single time a sample fails to load.
	// For an SFZ with thousands of samples, if one fails to load due to an invalid path, it's likely all will fail to load, and the user needs to close thousands or windows, or forcefully close lmms.
	// Doing it manually here bypasses the gui.
	const auto absolutePath = PathUtil::toAbsolute(path);
	auto result = SampleDecoder::decode(absolutePath);

	if (!result)
	{
		qWarning() << QObject::tr("Failed to load sample at path %1, the file may not exist, be corrupted, or is unsupported.").arg(absolutePath);
		return nullptr;
	}

	auto& [data, sampleRate] = *result;

	m_samplePool.insert({path, std::make_unique<SfzSampleBuffer>(std::move(data), sampleRate)});
	return m_samplePool.at(path).get();
}


} // namespace lmms

