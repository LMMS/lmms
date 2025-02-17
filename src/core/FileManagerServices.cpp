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

#include <QDesktopServices>
#include <QDir>
#include <QProcess>
#include <QUrl>

namespace lmms {
void FileManagerServices::openDir(QString& path)
{
	QString nativePath = QDir::toNativeSeparators(path);

	QProcess::startDetached(getDefaultFileManager(), {nativePath});
}
void FileManagerServices::reveal(const QFileInfo item)
{
	if (!canSelect()) { QDesktopServices::openUrl(QUrl::fromLocalFile(item.absolutePath())); }
	QString path = QDir::toNativeSeparators(item.canonicalFilePath());
	QStringList params;

#ifdef _WIN32
	// explorer /select,[object]: Selects the file in the new explorer window
	params += QLatin1String("/select,");
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

QString FileManagerServices::getDefaultFileManager()
{
	if (defaultFileManager.has_value()) { return defaultFileManager.value(); }
#if defined(_WIN32)
	defaultFileManager = "explorer";
#elif defined(__APPLE__)
	defaultFileManager = "open";
#else
	QProcess process;
	process.start("xdg-mime", {"query", "default", "inode/directory"});
	process.waitForFinished(3000);

	QString fileManager = QString::fromUtf8(process.readAllStandardOutput()).toLower().trimmed();

	if (fileManager.endsWith(".desktop")) { fileManager.chop(8); }

	// If the fileManager contains dots (e.g., "org.kde.dolphin"), extract only the last part
	fileManager = fileManager.section('.', -1);
	defaultFileManager = fileManager;
#endif
	return defaultFileManager.value();
}

bool FileManagerServices::canSelect()
{

	if (cachedCanSelect.has_value()) { return cachedCanSelect.value(); }
#if defined(_WIN32) || !defined(__APPLE__)
	cachedCanSelect = true;
#endif

	bool result = supportsSelectOption(getDefaultFileManager());
	cachedCanSelect = result;
	return result;
}

} // namespace lmms
