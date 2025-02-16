/*
 * FileManagerServices.h - include file for FileManagerServices
 *
 * Copyright (c) 2025 Andrew Wiltshire <aw1lt / at/ proton/ dot/me >
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

#ifndef LMMS_FILEMANAGER_SERVICES_H
#define LMMS_FILEMANAGER_SERVICES_H

#include <QFileInfo>
#include <QString>
#include <optional>

namespace lmms {

class FileManagerServices
{
public:
	[[maybe_unused]] static void openDir(QString& path);
	[[maybe_unused]] static void select(QFileInfo item);

#if defined(_WIN32)
	[[maybe_unused]] static QString getDefaultFileManager() { return QString("explorer"); }
	[[maybe_unused]] static bool canSelect(bool useCache = true) { return true; }
#elif defined(__APPLE__)
	[[maybe_unused]] static QString getDefaultFileManager() { return QString("open"); };
	[[maybe_unused]] static bool canSelect(bool useCache = true) { return true; }
#else
	[[maybe_unused]] static QString getDefaultFileManager();
	[[maybe_unused]] static bool canSelect(bool useCache = true);
#endif

protected:
#if !defined(_WIN32) && !defined(__APPLE__)
	static bool supportsSelectOption(const QString& fileManager);
#else
	static bool supportsSelectOption(const QString& fileManager) { return true; };
#endif

private:
#if !defined(_WIN32) && !defined(__APPLE__)
	static inline std::optional<bool> cachedCanSelect;
#else
	static inline std::optional<bool> cachedCanSelect = true;
#endif
};

} // namespace lmms
#endif // LMMS_FILEMANAGER_SERVICES_H
