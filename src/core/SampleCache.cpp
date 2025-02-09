/*
 * SampleCache.cpp
 *
 * Copyright (c) 2025 Sotonye Atemie <sakertooth@gmail.com>
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

#include "SampleCache.h"

#include <filesystem>

#include "PathUtil.h"
#include "SampleBuffer.h"

namespace lmms {
auto SampleCache::fetch(const QString& path) -> std::shared_ptr<SampleBuffer>
{
	namespace fs = std::filesystem;

	const auto it = std::find_if(s_audioFileMap.begin(), s_audioFileMap.end(),
		[&](const auto& entry) { return entry.first.path == PathUtil::pathFromQString(path); });

	auto lastWriteTime = fs::last_write_time(PathUtil::pathFromQString(path));

	if (it == s_audioFileMap.end() || it->first.lastWriteTime != lastWriteTime)
	{
		const auto buffer = std::make_shared<SampleBuffer>(path);
		const auto key = AudioFileEntry{PathUtil::pathFromQString(path), lastWriteTime};
		s_audioFileMap[std::move(key)] = buffer;
		return buffer;
	}

	auto buffer = it->second.lock();
	if (buffer == nullptr)
	{
		buffer = std::make_shared<SampleBuffer>(path);
		it->second = buffer;
	}

	return buffer;
}

auto SampleCache::fetch(const QString& base64, int sampleRate) -> std::shared_ptr<SampleBuffer>
{
	const auto key = Base64Entry{base64.toStdString(), sampleRate};
	auto& value = s_base64Map[std::move(key)];
	auto buffer = value.lock();

	if (!buffer)
	{
		buffer = std::make_shared<SampleBuffer>(base64, sampleRate);
		value = buffer;
	}

	return buffer;
}
} // namespace lmms
