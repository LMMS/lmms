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
#include <qdialog.h>
#include "lmms_export.h"

namespace lmms::gui
{


class LMMS_EXPORT FileDialog : public QFileDialog
{
	Q_OBJECT
public:
	enum class Operation
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

	explicit FileDialog( QWidget *parent = 0, const QString &caption = QString(),
						const QString &directory = QString(),
						const QString &filter = QString(),
						const Operation operation = Operation::Generic);

	~FileDialog() override;

	int exec() override;

	void clearSelection();

private:
	Operation operation;
	int m_status;

	static QMap<FileDialog::Operation, QString> OperationPaths;
	static bool OperationPathsReady;
	static void prepareOperationPaths();
	// If existing path is not empty, getOperationPath returns it instead.
	static QString getOperationPath(const enum Operation op, const QString& existing);
	static void setOperationPath(const enum Operation op, const QString& path);
};


} // namespace lmms::gui

#endif // LMMS_GUI_FILE_DIALOG_H
