/*
 * FileDialog.cpp - implementation of class FileDialog
 *
 * Copyright (c) 2014 Lukas W <lukaswhl/at/gmail.com>
 *
 * This file is part of LMMS - http://lmms.io
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

#include <QtCore/QList>
#include <QtCore/QUrl>
#include <QtGui/QDesktopServices>
#include <QtGui/QListView>

#include "config_mgr.h"
#include "FileDialog.h"


FileDialog::FileDialog( QWidget *parent, const QString &caption,
					   const QString &directory, const QString &filter ) :
	QFileDialog( parent, caption, directory, filter )
{
#if QT_VERSION >= 0x040806
	setOption( QFileDialog::DontUseCustomDirectoryIcons );
#endif

	// Add additional locations to the sidebar
	QList<QUrl> urls = sidebarUrls();
	urls << QUrl::fromLocalFile( QDesktopServices::storageLocation( QDesktopServices::DesktopLocation ) );
	// Find downloads directory
	QDir downloadDir( QDir::homePath() + "/Downloads" );
	if ( ! downloadDir.exists() )
		downloadDir = QDesktopServices::storageLocation( QDesktopServices::DocumentsLocation ) + "/Downloads";
	if ( downloadDir.exists() )
		urls << QUrl::fromLocalFile( downloadDir.absolutePath() );

	urls << QUrl::fromLocalFile( QDesktopServices::storageLocation( QDesktopServices::MusicLocation ) );
	urls << QUrl::fromLocalFile( configManager::inst()->workingDir() );

	// Add `/Volumes` directory on OS X systems, this allows the user to browse
	// external disk drives.
#ifdef LMMS_BUILD_APPLE
	QDir volumesDir( QDir("/Volumes") );
	if ( volumesDir.exists() )
		urls << QUrl::fromLocalFile( volumesDir.absolutePath() );
#endif

	setSidebarUrls(urls);
}



void FileDialog::clearSelection()
{
    QListView *view = findChild<QListView*>();
	Q_ASSERT( view );
	view->clearSelection();
}


#include "moc_FileDialog.cxx"
