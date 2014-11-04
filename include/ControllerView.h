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

#ifndef _CONTROLLER_VIEW_H
#define _CONTROLLER_VIEW_H

#include <QtGui/QWidget>

#include "AutomatableModel.h"
#include "Controller.h"
#include "ModelView.h"

class QGroupBox;
class QLabel;
class QPushButton;
class QMdiSubWindow;

class ledCheckBox;


class ControllerView : public QWidget, public ModelView
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
	void editControls();
	void deleteController();
	void displayHelp();
	void closeControls();

	
signals:
	void deleteController( ControllerView * _view );


protected:
	virtual void contextMenuEvent( QContextMenuEvent * _me );
	virtual void paintEvent( QPaintEvent * _pe );
	virtual void modelChanged();
	virtual void mouseDoubleClickEvent( QMouseEvent * event );


private:
	QPixmap m_bg;
	QMdiSubWindow * m_subWindow;
	ControllerDialog * m_controllerDlg;
	bool m_show;

} ;

#endif
