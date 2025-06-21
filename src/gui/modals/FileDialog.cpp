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
#include <qchar.h>

#include "ConfigManager.h"
#include "FileBrowser.h"
#include "volume.h"
#include "FileDialog.h"

namespace lmms::gui
{

std::vector<QString> FileDialog::OperationPaths = std::vector<QString>();
bool FileDialog::OperationPathsReady = false;

FileDialog::FileDialog(QWidget *parent, const QString &caption,
					   const QString &directory, const QString &filter,
					   const Operation operation) :
	QFileDialog( parent, caption, getOperationPath(operation, directory), filter),
	operation(operation)
{
#if QT_VERSION > 0x050200
	setOption( QFileDialog::DontUseCustomDirectoryIcons );
#endif
	setOption( QFileDialog::DontUseNativeDialog );
}

FileDialog::~FileDialog()
{
	setOperationPath(operation, directory().absolutePath());
}

QString FileDialog::getExistingDirectory(QWidget *parent,
										const QString &caption,
										const QString &directory,
										QFileDialog::Options options,
										const Operation operation)
{
	FileDialog dialog(parent, caption, directory, QString(), operation);
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
									const Operation operation)
{
	FileDialog dialog(parent, caption, directory, QString(), operation);
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

void FileDialog::prepareOperationPaths()
{
	if (OperationPathsReady)
	{
		return;
	}

	auto* config = ConfigManager::inst();

	OperationPaths = {
		config->workingDir(),
		config->userProjectsDir(),
		config->workingDir(),
		config->userPresetsDir(),
		config->userVstDir(),
		config->userSamplesDir(),
		config->workingDir()
	};

	OperationPathsReady = true;
}

QString FileDialog::getOperationPath(const FileDialog::Operation op, const QString& existing)
{
	if (!existing.isEmpty())
	{
		return existing;
	}

	prepareOperationPaths();

	if (op != Operation::End)
	{
		return OperationPaths[static_cast<size_t>(op)];
	}

	return OperationPaths[static_cast<size_t>(Operation::Generic)];
}

void FileDialog::setOperationPath(const FileDialog::Operation op, const QString& path)
{
	prepareOperationPaths();

	if (op != Operation::End)
	{
		OperationPaths[static_cast<size_t>(op)] = path;
	}
}

} // namespace lmms::gui
