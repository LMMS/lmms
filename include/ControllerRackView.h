/*
 * ControllerRackView.h - view for song's controllers
 *
 * Copyright (c) 2008-2009 Paul Giblock <drfaygo/at/gmail.com>
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

#ifndef _CONTROLLER_RACK_VIEW_H
#define _CONTROLLER_RACK_VIEW_H

#include <QtGui/QWidget>

#include "SerializingObject.h"
#include "lmms_basics.h"


class QPushButton;
class QScrollArea;

class ControllerView;


class ControllerRackView : public QWidget, public SerializingObject
{
	Q_OBJECT
public:
	ControllerRackView();
	virtual ~ControllerRackView();

	virtual void saveSettings( QDomDocument & _doc, QDomElement & _parent );
	virtual void loadSettings( const QDomElement & _this );

	inline virtual QString nodeName() const
	{
		return "ControllerRackView";
	}


public slots:
	void deleteController( ControllerView * _view );


private slots:
	virtual void update();
	void addController();


private:
	QVector<ControllerView *> m_controllerViews;

	QScrollArea * m_scrollArea;
	QPushButton * m_addButton;

} ;

#endif
