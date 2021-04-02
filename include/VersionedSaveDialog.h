/*
 * VersionedSaveDialog.h - declaration of class VersionedSaveDialog, a file save
 * dialog that provides buttons to increment or decrement a version which is
 * appended to the file name. (e.g. "MyProject-01.mmpz")
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


#ifndef VERSIONEDSAVEDIALOG_H
#define VERSIONEDSAVEDIALOG_H

#include "FileDialog.h"
#include "Song.h"

class QLineEdit;
class LedCheckBox;

class SaveOptionsWidget : public QWidget {
public:
	SaveOptionsWidget(Song::SaveOptions &saveOptions);

private:
	LedCheckBox *m_discardMIDIConnectionsCheckbox;
	LedCheckBox *m_saveAsProjectBundleCheckbox;
};

class VersionedSaveDialog : public FileDialog
{
	Q_OBJECT
public:
	explicit VersionedSaveDialog( QWidget *parent = 0,
								  QWidget *saveOptionsWidget = nullptr,
								  const QString &caption = QString(),
								  const QString &directory = QString(),
								  const QString &filter = QString() );

	// Returns true if file name was changed, returns false if it wasn't
	static bool changeFileNameVersion( QString &fileName, bool increment );
	static bool fileExistsQuery( QString FileName, QString WindowTitle );

public slots:
	void incrementVersion();
	void decrementVersion();
};

#endif // VERSIONEDSAVEDIALOG_H
