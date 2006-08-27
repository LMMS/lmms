/*
 * about_dialog.h - declaration of class aboutDialog
 *
 * Copyright (c) 2004-2005 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifndef _ABOUT_DIALOG_H
#define _ABOUT_DIALOG_H

#include "qt3support.h"

#ifdef QT4

#include <QtGui/QDialog>

#else

#include <qdialog.h>

#endif


class QPushButton;
class QLabel;
class QTabWidget;


class aboutDialog : public QDialog
{
	Q_OBJECT
public:
	aboutDialog( void );
	~aboutDialog();


protected:
	virtual void keyPressEvent( QKeyEvent * _ke );
	virtual void resizeEvent( QResizeEvent * _re );


private:
	QPushButton * m_okBtn;
	QLabel * m_iconLbl;
	QLabel * m_appNameLbl;
	QTabWidget * m_aboutTabs;

} ;


#endif

