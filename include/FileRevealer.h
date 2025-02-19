/*
 * FileRevealer.h - include file for FileRevealer
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

#ifndef LMMS_FILE_REVEALER_H
#define LMMS_FILE_REVEALER_H

#include <QFileInfo>
#include <QString>
#include <optional>

#include "lmmsconfig.h"

namespace lmms {

/**
 * @class FileRevealer
 * @brief A utility class for revealing files and directories in the system's file manager.
 */
class FileRevealer
{
public:
	/**
	 * @brief Retrieves the default file manager for the current platform.
	 * @return The name or command of the default file manager.
	 */
	static const QString& getDefaultFileManager();

	/**
	 * @brief Opens the directory containing the specified file or folder in the file manager.
	 * @param item The QFileInfo object representing the file or folder.
	 */
	static void openDir(QFileInfo item);

	/**
	 * @brief Checks whether the file manager supports selecting a specific file.
	 * @return True if selection is supported, otherwise false.
	 */
	static bool canSelect();

	/**
	 * @brief Opens the file manager and selects the specified file if supported.
	 * @param item The QFileInfo object representing the file to reveal.
	 */
	static void reveal(QFileInfo item);

protected:
	/**
	 * @brief Determines if the given file manager supports the "--select" option.
	 * @param fileManager The name of the file manager to check.
	 * @return True if the file manager supports "--select", otherwise false.
	 */
	static bool supportsSelectOption(const QString& fileManager);
};

} // namespace lmms
#endif // LMMS_FILE_REVEALER_H
