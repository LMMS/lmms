/*
 * export_project_dialog.h - declaration of class exportProjectDialog which is
 *                           responsible for exporting project
 *
 * Copyright (c) 2004-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include "ui_export_project.h"

class ProjectRenderer;


class exportProjectDialog : public QDialog, public Ui::ExportProjectDialog
{
	Q_OBJECT
public:
	exportProjectDialog( const QString & _file_name, QWidget * _parent );
	virtual ~exportProjectDialog();


protected:
	virtual void reject( void );
	virtual void closeEvent( QCloseEvent * _ce );


private slots:
	void startBtnClicked( void );
	void updateTitleBar( int );


private:
	QString m_fileName;
	ProjectRenderer * m_renderer;

} ;

#endif
