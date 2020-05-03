/*
 * RenameDialog.h - declaration of class renameDialog, a simple dialog for
 *                   changing the content of a string
 *
 * Copyright (c) 2004-2005 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifndef RENAME_DIALOG_H
#define RENAME_DIALOG_H

#include <QDialog>


class QLineEdit;


class RenameDialog : public QDialog
{
	Q_OBJECT
public:
	RenameDialog( QString & _string );
	~RenameDialog();


protected:
	void keyPressEvent( QKeyEvent * _ke ) override;
	void resizeEvent(QResizeEvent * event) override;


protected slots:
	void textChanged( const QString & _new_string );


private:
	QString & m_stringToEdit;
	QString m_originalString;
	QLineEdit * m_stringLE;

} ;


#endif
