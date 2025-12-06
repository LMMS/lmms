/*
 * FileDialog.cpp - implementation of class FileDialog
 *
 * Copyright (c) 2014 Lukas W <lukaswhl/at/gmail.com>
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

#include <QList>
#include <QUrl>
#include <QListView>
#include <QStandardPaths>
#include <QStorageInfo>
#include <QStringList>

#include "ConfigManager.h"
#include "FileDialog.h"

namespace lmms::gui
{


FileDialog::FileDialog( QWidget *parent, const QString &caption,
					   const QString &directory, const QString &filter ) :
	QFileDialog( parent, caption, directory, filter )
{
	setOption( QFileDialog::DontUseCustomDirectoryIcons );
	setOption( QFileDialog::DontUseNativeDialog );

#ifdef LMMS_BUILD_LINUX
	QList<QUrl> urls;
#else
	QList<QUrl> urls = sidebarUrls();
#endif

	QDir desktopDir;
	desktopDir.setPath(QStandardPaths::writableLocation(QStandardPaths::DesktopLocation));
	if (desktopDir.exists())
	{
		urls << QUrl::fromLocalFile(desktopDir.absolutePath());
	}
	
	QDir downloadDir(QDir::homePath() + "/Downloads");
	if (!downloadDir.exists())
	{
		downloadDir.setPath(QStandardPaths::writableLocation(QStandardPaths::DownloadLocation));
	}
	if (downloadDir.exists())
	{
		urls << QUrl::fromLocalFile(downloadDir.absolutePath());
	}

	QDir musicDir;
	musicDir.setPath(QStandardPaths::writableLocation(QStandardPaths::MusicLocation));
	if (musicDir.exists())
	{
		urls << QUrl::fromLocalFile(musicDir.absolutePath());
	}

	urls << QUrl::fromLocalFile(ConfigManager::inst()->workingDir());
	
	// Add `/Volumes` directory on OS X systems, this allows the user to browse
	// external disk drives.
#ifdef LMMS_BUILD_APPLE
	QDir volumesDir( QDir("/Volumes") );
	if ( volumesDir.exists() )
		urls << QUrl::fromLocalFile( volumesDir.absolutePath() );
#endif

#ifdef LMMS_BUILD_LINUX

	// FileSystem types : https://www.javatpoint.com/linux-file-system
	QStringList usableFileSystems = {"ext", "ext2", "ext3", "ext4", "jfs", "reiserfs", "ntfs3", "fuse.sshfs", "fuseblk"};

	for(QStorageInfo storage : QStorageInfo::mountedVolumes())
	{
		storage.refresh();

		if (usableFileSystems.contains(QString(storage.fileSystemType()), Qt::CaseInsensitive) && storage.isValid() && storage.isReady())
		{			
			urls << QUrl::fromLocalFile(storage.rootPath());	
		}
	}
#endif

	setSidebarUrls(urls);
}


QString FileDialog::getExistingDirectory(QWidget *parent,
										const QString &caption,
										const QString &directory,
										QFileDialog::Options options)
{
	FileDialog dialog(parent, caption, directory, QString());
	dialog.setFileMode(QFileDialog::Directory);
	dialog.setOptions(dialog.options() | options);
	if (dialog.exec() == QDialog::Accepted) {
		return dialog.selectedFiles().value(0);
	}
	return QString();
}

QString FileDialog::getOpenFileName(QWidget *parent,
									const QString &caption,
									const QString &directory,
									const QString &filter,
									QString *selectedFilter)
{
	FileDialog dialog(parent, caption, directory, filter);
	if (selectedFilter && !selectedFilter->isEmpty())
		dialog.selectNameFilter(*selectedFilter);
	if (dialog.exec() == QDialog::Accepted) {
		if (selectedFilter)
			*selectedFilter = dialog.selectedNameFilter();
		return dialog.selectedFiles().value(0);
	}
	return QString();
}


void FileDialog::clearSelection()
{
	auto view = findChild<QListView*>();
	Q_ASSERT( view );
	view->clearSelection();
}


} // namespace lmms::gui