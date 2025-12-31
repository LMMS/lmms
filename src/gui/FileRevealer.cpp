/*
 * FileRevealer.cpp - Helper file for cross platform file revealing
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

#include "FileRevealer.h"

#include <QDesktopServices>
#include <QDebug>
#include <QDir>
#include <QProcess>
#include <QString>
#include <QRegularExpression>
#include <QUrl>
#include <optional>

#include "lmmsconfig.h"

namespace lmms::gui {
bool FileRevealer::s_canSelect = false;

const QString& FileRevealer::getDefaultFileManager()
{
	static std::optional<QString> fileManagerCache;
	if (fileManagerCache.has_value()) { return fileManagerCache.value(); }
#if defined(LMMS_BUILD_WIN32)
	fileManagerCache = "explorer";
#elif defined(LMMS_BUILD_APPLE)
	fileManagerCache = "open";
#else

	QString desktopEnv = qgetenv("XDG_CURRENT_DESKTOP").trimmed().toLower();

	if (desktopEnv.contains("xfce"))
	{
		fileManagerCache = "exo-open";
		return fileManagerCache.value();
	}

	QProcess process;
	if (desktopEnv.contains("gnome"))
	{
		process.start("gio", {"mime", "inode/directory"});
	}
	else
	{
		process.start("xdg-mime", {"query", "default", "inode/directory"});
	}

	process.waitForFinished(3000);

	QString fileManager = QString::fromUtf8(process.readAllStandardOutput()).toLower().trimmed();

	if (fileManager.contains("inode/directory"))
	{
		// gio format: split on ":" or "\n", take second element
		QStringList fileManagers = fileManager.split(QRegularExpression("[:\n]"), Qt::SkipEmptyParts);
		if (fileManagers.length() >= 2)
		{
			fileManagers.removeFirst();
			fileManager = fileManagers.first().trimmed();
		}
		else
		{
			// Fallback to something sane
			fileManager = "xdg-open";
		}
	}
	else
	{
		// xdg-mime format: split on ";", take the last non-empty element
		QStringList fileManagers = fileManager.split(';', Qt::SkipEmptyParts);
		if (!fileManagers.isEmpty())
		{
			// The highest priority file manager is last
			fileManager = fileManagers.last();
		}
	}

	if (fileManager.endsWith(".desktop")) { fileManager.chop(8); }

	// If the fileManager contains dots (e.g., "org.kde.dolphin"), extract only the last part
	fileManager = fileManager.section('.', -1);
	fileManagerCache = fileManager;
#endif
	qDebug() << "FileRevealer: Default app for inode/directory:" << fileManagerCache.value();
	return fileManagerCache.value();
}

void FileRevealer::openDir(const QFileInfo item)
{
	QString nativePath = QDir::toNativeSeparators(item.canonicalFilePath());

	QProcess::startDetached(getDefaultFileManager(), {nativePath});
}

const QStringList& FileRevealer::getSelectCommand()
{
	static std::optional<QStringList> selectCommandCache;

	if (selectCommandCache.has_value()) { return selectCommandCache.value(); }

	static const std::map<QString, QStringList> argMap = {
		{"open", {"-R"}},
		{"explorer", {"/select,"}},
		{"nemo", {}},
		{"thunar", {}},
		{"exo-open", {"--launch", "FileManager"}},
	};

	// Skip calling "--help" for file managers that we know
	for (const auto& [fileManager, arg] : argMap)
	{
		if (fileManager == getDefaultFileManager())
		{
			s_canSelect = true;
			selectCommandCache = arg;
			return selectCommandCache.value();
		}
	}

	// Parse "<command> --help" and look for the "--select" for file managers that we don't know
	if (supportsArg(getDefaultFileManager(), "--select"))
	{
		s_canSelect = true;
		selectCommandCache = {"--select"};
		return selectCommandCache.value();
	}

	// Fallback to empty list
	selectCommandCache = {};
	return selectCommandCache.value();
}

void FileRevealer::reveal(const QFileInfo item)
{
	// Sets selectCommandCache, canSelect
	const QStringList& selectCommand = getSelectCommand();
	if (!s_canSelect)
	{
		QDesktopServices::openUrl(QUrl::fromLocalFile(item.canonicalPath()));
		return;
	}

	QString path = QDir::toNativeSeparators(item.canonicalFilePath());
	QStringList params;

	if (!selectCommand.isEmpty()) { params << selectCommand; }

	params << path;
	QProcess::startDetached(getDefaultFileManager(), params);
}

bool FileRevealer::supportsArg(const QString& command, const QString& arg)
{
	QProcess process;
	process.start(command, {"--help"});
	process.waitForFinished(3000);

	QString output = process.readAllStandardOutput() + process.readAllStandardError();
	return output.contains(arg);
}

} // namespace lmms::gui
