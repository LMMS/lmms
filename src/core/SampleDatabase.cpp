/*
 * SampleDatabase.cpp
 *
 * Copyright (c) 2024 saker
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

#include "SampleDatabase.h"

#include <filesystem>

#include "FileSystemHelpers.h"
#include "SampleBuffer.h"

namespace lmms {
auto SampleDatabase::fetch(const QString& path) -> std::shared_ptr<SampleBuffer>
{
	const auto fsPath = FileSystemHelpers::pathFromQString(path);
	const auto entry = AudioFileEntry{fsPath, std::filesystem::last_write_time(fsPath)};
	return get(entry, s_audioFileMap, path);
}

auto SampleDatabase::fetch(const QString& base64, int sampleRate) -> std::shared_ptr<SampleBuffer>
{
	const auto entry = Base64Entry{base64.toStdString(), sampleRate};
	return get(entry, s_base64Map, base64, sampleRate);
}
} // namespace lmms
