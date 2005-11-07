/*
 * export_project_dialog.h - declaration of class exportProjectDialog which is
 *                           responsible for exporting project
 *
 * Linux MultiMedia Studio
 * Copyright (c) 2004-2005 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */


#ifndef _EXPORT_PROJECT_DIALOG_H
#define _EXPORT_PROJECT_DIALOG_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif


#include "qt3support.h"

#ifdef QT4

#include <QDialog>

#else

#include <qdialog.h>

#endif


#include "export.h"


class QLabel;
class QPushButton;
class QComboBox;
class QProgressBar;
class QCheckBox;
class pixmapButton;


class exportProjectDialog : public QDialog
{
	Q_OBJECT
public:
	exportProjectDialog( const QString & _file_name, QWidget * _parent );
	~exportProjectDialog();
	void FASTCALL updateProgressBar( int _new_val );


public slots:
	void exportBtnClicked( void );


protected:
	void keyPressEvent( QKeyEvent * _ke );
	void closeEvent( QCloseEvent * _ce );


private slots:
	void changedType( const QString & );
	void cancelBtnClicked( void );
	void redrawProgressBar( void );


private:
	QString m_fileName;
	QLabel * m_typeLbl;
	QComboBox * m_typeCombo;
	QLabel * m_kbpsLbl;
	QComboBox * m_kbpsCombo;
	QCheckBox * m_vbrCb;
	QCheckBox * m_hqmCb;
	QLabel * m_hourglassLbl;
	QPushButton * m_exportBtn;
	QPushButton * m_cancelBtn;
	QProgressBar * m_exportProgressBar;
	fileTypes m_fileType;
	bool m_deleteFile;
	int m_oldProgressVal;
	int m_progressVal;

	QTimer * m_progressBarUpdateTimer;

	static Sint16 s_availableBitrates[];

	void finishProjectExport( void );
	void abortProjectExport( void );

	static fileTypes FASTCALL getFileTypeFromExtension( const QString &
									_ext );

} ;

#endif
