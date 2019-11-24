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
#include <QDesktopServices>
#include <QListView>

#include "ConfigManager.h"
#include "FileDialog.h"


FileDialog::FileDialog( QWidget *parent, const QString &caption,
					   const QString &directory, const QString &filter ) :
	QFileDialog( parent, caption, directory, filter )
{
#if QT_VERSION > 0x050200
	setOption( QFileDialog::DontUseCustomDirectoryIcons );
#endif

	setOption( QFileDialog::DontUseNativeDialog );

	// Add additional locations to the sidebar
	QList<QUrl> urls = sidebarUrls();
	urls << QUrl::fromLocalFile( QStandardPaths::writableLocation( QStandardPaths::DesktopLocation ) );
	// Find downloads directory
	QDir downloadDir( QDir::homePath() + "/Downloads" );
	if ( ! downloadDir.exists() )
		downloadDir.setPath(QStandardPaths::writableLocation( QStandardPaths::DownloadLocation ));
	if ( downloadDir.exists() )
		urls << QUrl::fromLocalFile( downloadDir.absolutePath() );

	urls << QUrl::fromLocalFile( QStandardPaths::writableLocation( QStandardPaths::MusicLocation ) );
	urls << QUrl::fromLocalFile( ConfigManager::inst()->workingDir() );

	// Add `/Volumes` directory on OS X systems, this allows the user to browse
	// external disk drives.
#ifdef LMMS_BUILD_APPLE
	QDir volumesDir( QDir("/Volumes") );
	if ( volumesDir.exists() )
		urls << QUrl::fromLocalFile( volumesDir.absolutePath() );
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
									QString *selectedFilter,
									QFileDialog::Options options)
{
	FileDialog dialog(parent, caption, directory, filter);
	dialog.setOptions(dialog.options() | options);
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
    QListView *view = findChild<QListView*>();
	Q_ASSERT( view );
	view->clearSelection();
}

