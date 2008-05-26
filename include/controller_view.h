/*
 * controller_view.h - view-component for an control
 *
 * Copyright (c) 2008 Paul Giblock <drfaygo/at/gmail.com>
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

#ifndef _CONTROLLER_VIEW_H
#define _CONTROLLER_VIEW_H

#include <QtGui/QWidget>

#include "automatable_model.h"
#include "controller.h"
#include "mv_base.h"

class QGroupBox;
class QLabel;
class QPushButton;
class QMdiSubWindow;

//class controllerControlDialog;
//class knob;
class ledCheckBox;
//class tempoSyncKnob;
//class track;


class controllerView : public QWidget, public modelView
{
	Q_OBJECT
public:
	controllerView( controller * _controller, QWidget * _parent );
	virtual ~controllerView();
	
    inline controller * getController( void )
	{
		return( castModel<controller>() );
	}

	inline const controller * getController( void ) const
	{
		return( castModel<controller>() );
	}


public slots:
	void editControls( void );
	//void deletePlugin( void );
	//void displayHelp( void );
	void closeControls( void );

	
signals:
	//void moveUp( effectView * _plugin );
	//void moveDown( effectView * _plugin );
	//void deletePlugin( effectView * _plugin );


protected:
	virtual void contextMenuEvent( QContextMenuEvent * _me );
	virtual void paintEvent( QPaintEvent * _pe );
	virtual void modelChanged( void );


private:
	QPixmap m_bg;
	ledCheckBox * m_bypass;
	QMdiSubWindow * m_subWindow;
	controllerDialog * m_controllerDlg;
	bool m_show;

} ;

#endif
