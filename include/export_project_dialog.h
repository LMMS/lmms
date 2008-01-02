/*
 * export_project_dialog.h - declaration of class exportProjectDialog which is
 *                           responsible for exporting project
 *
 * Copyright (c) 2004-2007 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * 
 * This file is part of Linux MultiMedia Studio - http://lmms.sourceforge.net
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


#ifndef _EXPORT_PROJECT_DIALOG_H
#define _EXPORT_PROJECT_DIALOG_H

#include <QtGui/QDialog>

#include "export.h"
#include "automatable_model.h"


class QLabel;
class QPushButton;
class QProgressBar;

class comboBox;
class comboBoxModel;
class ledCheckBox;
class pixmapButton;


class exportProjectDialog : public QDialog
{
	Q_OBJECT
public:
	exportProjectDialog( const QString & _file_name, QWidget * _parent );
	virtual ~exportProjectDialog();


public slots:
	void exportBtnClicked( void );


protected:
	virtual void keyPressEvent( QKeyEvent * _ke );
	virtual void closeEvent( QCloseEvent * _ce );


private slots:
	void changedType( const QString & );
	void cancelBtnClicked( void );


private:
	void finishProjectExport( void );
	void abortProjectExport( void );

	static fileTypes getFileTypeFromExtension( const QString & _ext );
	static Sint16 s_availableBitrates[];


	QLabel * m_typeLbl;
	comboBox * m_typeCombo;
	comboBoxModel * m_typeModel;
	QLabel * m_kbpsLbl;
	comboBox * m_kbpsCombo;
	comboBoxModel * m_kbpsModel;
	ledCheckBox * m_vbrCb;
	boolModel * m_vbrEnabledModel;
	ledCheckBox * m_hqmCb;
	boolModel * m_hqmEnabledModel;
	QLabel * m_hourglassLbl;
	QPushButton * m_exportBtn;
	QPushButton * m_cancelBtn;
	QProgressBar * m_exportProgressBar;

	QString m_fileName;
	fileTypes m_fileType;
	bool m_deleteFile;

} ;

#endif
