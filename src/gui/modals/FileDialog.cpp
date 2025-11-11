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

#include "ConfigManager.h"

#include "FileDialog.h"

namespace lmms::gui
{

QMap<FileDialog::DirType, QString> FileDialog::s_lastUsedPaths = {};

FileDialog::FileDialog(QWidget *parent, const QString &caption,
					   const DirType dirType, 
					   const QString &filter,
					   const QString &directory):
	QFileDialog(parent, caption, directory.isEmpty() ? getPath(dirType) : directory, filter),
	m_dirType(dirType)
{
#if QT_VERSION > 0x050200
	setOption( QFileDialog::DontUseCustomDirectoryIcons );
#endif
	setOption( QFileDialog::DontUseNativeDialog );
}

FileDialog::~FileDialog()
{
	if (result() == QDialog::Accepted)
	{
		s_lastUsedPaths[m_dirType] = directory().absolutePath();
	}
}

QString FileDialog::getDefaultPath(const DirType dirType)
{
	auto* config = ConfigManager::inst();

	switch (dirType)
	{
		case DirType::Project:
			return config->userProjectsDir();
		case DirType::Midi:
			return config->workingDir();
		case DirType::Preset:
			return config->userPresetsDir();
		case DirType::Plugin:
			return config->userVstDir();
		case DirType::Sample:
			return config->userSamplesDir();
		case DirType::Soundfont:
			return config->userSf2Dir();
		case DirType::Song:
			return config->workingDir();
		default:
			return config->workingDir();
	}
}

QString FileDialog::getPath(const FileDialog::DirType dirType)
{
	auto& path = s_lastUsedPaths[dirType];

	if (path.isEmpty())
	{
		path = getDefaultPath(dirType);
	}

	return path;
}


} // namespace lmms::gui
