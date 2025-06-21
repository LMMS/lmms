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
#include <QMap>
#include <QUrl>
#include <QListView>
#include <QStandardPaths>
#include <QStorageInfo>
#include <QStringList>
#include <qchar.h>

#include "ConfigManager.h"
#include "FileBrowser.h"
#include "volume.h"
#include "FileDialog.h"

namespace lmms::gui
{

QMap<FileDialog::Operation, QString> FileDialog::OperationPaths = {};
bool FileDialog::OperationPathsReady = false;

FileDialog::FileDialog(QWidget *parent, const QString &caption,
					   const QString &directory, const QString &filter,
					   const Operation operation) :
	QFileDialog(parent, caption, getOperationPath(operation, directory), filter),
	operation(operation),
	m_status(QDialog::Rejected)
{
#if QT_VERSION > 0x050200
	setOption( QFileDialog::DontUseCustomDirectoryIcons );
#endif
	setOption( QFileDialog::DontUseNativeDialog );
}

int FileDialog::exec()
{
	m_status = this->QFileDialog::exec();
	return m_status;
}

FileDialog::~FileDialog()
{
	if (m_status == QDialog::Accepted)
	{
		setOperationPath(operation, directory().absolutePath());
	}
}

void FileDialog::clearSelection()
{
	auto view = findChild<QListView*>();
	Q_ASSERT( view );
	view->clearSelection();
}

void FileDialog::prepareOperationPaths()
{
	if (OperationPathsReady)
	{
		return;
	}

	auto* config = ConfigManager::inst();

	OperationPaths[Operation::Generic]		= config->workingDir();
	OperationPaths[Operation::Project]  	= config->userProjectsDir();
	OperationPaths[Operation::Midi]  		= config->workingDir();
	OperationPaths[Operation::Preset]		= config->userPresetsDir();
	OperationPaths[Operation::Plugin]		= config->userVstDir();
	OperationPaths[Operation::Sample]		= config->userSamplesDir();
	OperationPaths[Operation::Soundfont]	= config->userSf2Dir();
	OperationPaths[Operation::Song]  		= config->workingDir();

	OperationPathsReady = true;
}

QString FileDialog::getOperationPath(const FileDialog::Operation op, const QString& existing)
{
	if (!existing.isEmpty())
	{
		return existing;
	}

	prepareOperationPaths();

	return OperationPaths[op];
}

void FileDialog::setOperationPath(const FileDialog::Operation op, const QString& path)
{
	OperationPaths[op] = path;
}

} // namespace lmms::gui
