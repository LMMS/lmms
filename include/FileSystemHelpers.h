/*
 * FileSystemHelpers.h
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

#ifndef LMMS_FILE_SYSTEM_HELPERS_H
#define LMMS_FILE_SYSTEM_HELPERS_H

#include <QString>
#include <filesystem>

namespace lmms {
class FileSystemHelpers
{
public:
	static std::filesystem::path pathFromQString(const QString& path)
	{
#ifdef _WIN32
		return std::filesystem::path{path.toStdWString()};
#else
		return std::filesystem::path{path.toStdString()};
#endif
	}

	static QString qStringFromPath(const std::filesystem::path& path)
	{
#ifdef _WIN32
		return QString::fromStdWString(path.generic_wstring());
#else
		return QString::fromStdString(path.native());
#endif
	}
};
} // namespace lmms

#endif // LMMS_FILE_SYSTEM_HELPERS_H
