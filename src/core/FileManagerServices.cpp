/*
 * FileManagerServices.cpp - Helper file for cross platform file management
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

#include "FileManagerServices.h"

#include <QDir>
#include <QProcess>

namespace lmms {
void FileManagerServices::openDir(QString& path)
{
	QString nativePath = QDir::toNativeSeparators(path);

#if defined(_WIN32)
	QProcess::startDetached("explorer", {nativePath});
#elif defined(__APPLE__)
	QProcess::startDetached("open", {nativePath});
#else
	QProcess::startDetached("xdg-open", {nativePath});
#endif
}
void FileManagerServices::select(const QFileInfo item)
{
	QString path = QDir::toNativeSeparators(item.canonicalFilePath());
	QStringList params;

#ifdef _WIN32
	if (!item.isDir())
	{
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

bool FileManagerServices::supportsSelectOption(const QString& fileManager)
{
#if !defined(_WIN32) && !defined(__APPLE__)

	QProcess process;
	process.start(fileManager, {"--help"});
	process.waitForFinished(3000);

	QString output = process.readAllStandardOutput() + process.readAllStandardError();
	return output.contains("--select");
#else
	return true;
#endif
}

QString FileManagerServices::getDefaultFileManager(bool useCache)
{
#if defined(_WIN32)
	defaultFileManager = "explorer";
	return defaultFileManager.value();
#elif defined(__APPLE__)
	defaultFileManager = "open";
	return defaultFileManager.value();
#else

	if (useCache && defaultFileManager.has_value()) { return defaultFileManager.value(); }

	QProcess process;
	process.start("xdg-mime", {"query", "default", "inode/directory"});
	process.waitForFinished(3000);

	QString fileManager = QString::fromUtf8(process.readAllStandardOutput()).toLower().trimmed();

	if (fileManager.endsWith(".desktop")) { fileManager.chop(8); }

	// If the fileManager contains dots (e.g., "org.kde.dolphin"), extract only the last part
	fileManager = fileManager.section('.', -1);

	return fileManager;
#endif
}

bool FileManagerServices::canSelect(bool useCache)
{
#if defined(_WIN32) || !defined(__APPLE__)
	cachedCanSelect = true;
#endif
	if (useCache && cachedCanSelect.has_value()) { return cachedCanSelect.value(); }

	bool result = supportsSelectOption(getDefaultFileManager());
	cachedCanSelect = result;
	return result;
}

} // namespace lmms
