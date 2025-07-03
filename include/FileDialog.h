/*
 * FileDialog.h - declaration of class FileDialog
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

#ifndef LMMS_GUI_FILE_DIALOG_H
#define LMMS_GUI_FILE_DIALOG_H

#include <QMap>
#include <QFileDialog>

#include "lmms_export.h"

namespace lmms::gui
{


class LMMS_EXPORT FileDialog : public QFileDialog
{
	Q_OBJECT
public:
	enum class DirType
	{
		Generic,
		Project,
		Midi,
		Preset,
		Plugin,
		Sample,
		Soundfont,
		Song,
	};

	explicit FileDialog(QWidget *parent = 0, const QString &caption = QString(),
						const DirType operation = DirType::Generic,
						const QString &directory = QString(),
						const QString &filter = QString());

	~FileDialog() override;

private:
	DirType m_dirType;

	static QMap<FileDialog::DirType, QString> s_lastUsedPaths;
	static QString getDefaultPath(const DirType ty);
	static QString getPath(const enum DirType ty);
};


} // namespace lmms::gui

#endif // LMMS_GUI_FILE_DIALOG_H
