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


#ifndef FILEDIALOG_H
#define FILEDIALOG_H

#include <QFileDialog>

#include "lmms_export.h"

class LMMS_EXPORT FileDialog : public QFileDialog
{
	Q_OBJECT
public:
	explicit FileDialog( QWidget *parent = 0, const QString &caption = QString(),
						const QString &directory = QString(),
						const QString &filter = QString() );

	static QString getExistingDirectory(QWidget *parent,
										const QString &caption,
										const QString &directory,
										QFileDialog::Options options = QFileDialog::ShowDirsOnly);
    static QString getOpenFileName(QWidget *parent = 0,
									const QString &caption = QString(),
									const QString &directory = QString(),
									const QString &filter = QString(),
									QString *selectedFilter = 0,
									QFileDialog::Options options = 0);
	void clearSelection();
};

#endif // FILEDIALOG_HPP
