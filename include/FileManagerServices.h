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
	static QString getDefaultFileManager();
	static void openDir(QString& path);
	static bool canSelect();
	static void reveal(QFileInfo item);

protected:
	static bool supportsSelectOption(const QString& fileManager);

private:
	static inline std::optional<QString> fileManagerCache;
	static inline std::optional<bool> canSelectCache;
};

} // namespace lmms
#endif // LMMS_FILEMANAGER_SERVICES_H
