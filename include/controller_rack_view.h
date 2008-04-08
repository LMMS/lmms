/*
 * controller_rack_view.h - view for song's controllers
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

#ifndef _CONTROLLER_RACK_VIEW_H
#define _CONTROLLER_RACK_VIEW_H

#include <QtGui/QWidget>

#include "types.h"

#include "mv_base.h"

class QPushButton;
class QScrollArea;
class QVBoxLayout;

class controllerView;


class controllerRackView : public QWidget, public modelView
{
	Q_OBJECT
public:
	controllerRackView();
	virtual ~controllerRackView();

	void clear( void );


public slots:
	//void moveUp( effectView * _view );
	//void moveDown( effectView * _view );
	//void deletePlugin( effectView * _view );


private slots:
	virtual void update( void );
	void addController( void );


private:
	/*virtual void modelChanged( void );

	inline effectChain * fxChain( void )
	{
		return( castModel<effectChain>() );
	}

	inline const effectChain * fxChain( void ) const
	{
		return( castModel<effectChain>() );
	}
*/

	QVector<controllerView *> m_controllerViews;

	QVBoxLayout * m_mainLayout;
	//groupBox * m_effectsGroupBox;
	QScrollArea * m_scrollArea;
	QPushButton * m_addButton;
	
	Uint32 m_lastY;

} ;

#endif
