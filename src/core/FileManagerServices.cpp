/*
 * FileManagerServices.cpp - Helper file for cross platform file management
 *
 * Copyright (c) 2025 Andrew Wiltshire
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

#include "FileManagerServices.h"

#include <QDir>
#include <QProcess>

namespace lmms {
void FileManagerServices::select(const QFileInfo item)
{
	QString path = QDir::toNativeSeparators(item.canonicalFilePath());
	QStringList params;

#ifdef _WIN32
	if (!item.isDir()) {
		// explorer /select,[object]: Selects the file in the new explorer window
		params += QLatin1String("/select,");
	}
#elif __APPLE__
	// Finder -R, --reveal: Selects in finder
	params += "-R";
#else
	// --select: Linux, BSD and other *nix systems
	params += "--select";
#endif
	params += path;
	QProcess::startDetached(getDefaultFileManager(), params);
}

#if !defined(_WIN32) && !defined(__APPLE__)
bool FileManagerServices::supportsSelectOption(const QString& fileManager)
{
	QProcess process;
	process.start(fileManager, {"--help"});
	process.waitForFinished(3000);

	QString output = process.readAllStandardOutput() + process.readAllStandardError();
	return output.contains("--select");
}

QString FileManagerServices::getDefaultFileManager()
{
	QProcess process;
	process.start("xdg-mime", {"query", "default", "inode/directory"});
	process.waitForFinished(3000);

	QString fileManager = QString::fromUtf8(process.readAllStandardOutput()).toLower().trimmed();

	if (fileManager.endsWith(".desktop")) {
		fileManager.chop(8);
	}

	if (fileManager.isEmpty())
	{
		fileManager = qgetenv("FILE_MANAGER");
		if (fileManager.isEmpty()) { fileManager = qgetenv("XDG_FILE_MANAGER"); }
	}

	return fileManager;
}

static bool canSelect()
{
	return supportsSelectOption(getDefaultFileManager());
}
#endif

} // namespace lmms