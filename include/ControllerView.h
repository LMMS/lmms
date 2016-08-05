/*
 * ControllerView.h - view-component for an control
 *
 * Copyright (c) 2008 Paul Giblock <drfaygo/at/gmail.com>
 *
 * This file is part of LMMS - http://lmms.io
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

#ifndef CONTROLLER_VIEW_H
#define CONTROLLER_VIEW_H

#include <QFrame>

#include "AutomatableModel.h"
#include "Controller.h"
#include "ModelView.h"

class QGroupBox;
class QLineEdit;
class QPushButton;
class QMdiSubWindow;

class LedCheckBox;


class ControllerView : public QFrame, public ModelView
{
	Q_OBJECT
public:
	ControllerView( Controller * _controller, QWidget * _parent );
	virtual ~ControllerView();

	inline Controller * getController()
	{
		return( castModel<Controller>() );
	}

	inline const Controller * getController() const
	{
		return( castModel<Controller>() );
	}


public slots:

	void deleteController();
	void displayHelp();
	void collapseController();
	void renameFinished();
	void rename();


signals:
	void deleteController( ControllerView * _view );
	void controllerCollapsed();


protected:
	virtual void paintEvent( QPaintEvent * event );
	virtual void contextMenuEvent( QContextMenuEvent * _me );
	virtual void modelChanged();
	virtual void mouseDoubleClickEvent( QMouseEvent * event );
	virtual void dragEnterEvent( QDragEnterEvent * dee );
	virtual void dropEvent( QDropEvent * de );


private:
	ControllerDialog * m_controllerDlg;
	const int m_titleBarHeight;
	bool m_show;
	QLineEdit * m_nameLineEdit;
	QPushButton * m_collapse;
	Controller * m_modelC;
} ;

#endif
