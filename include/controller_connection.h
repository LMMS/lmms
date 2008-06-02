/*
 * controller_connection.h - declaration of a controller connect, which
 *              provides a definition of the link between a controller and
 *              model, also handles deferred creation of links while
 *              loading project
 *
 * Copyright (c) 2008 Paul Giblock <pgllama/at/gmail.com>
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


#ifndef _CONTROLLER_CONNECTION_H
#define _CONTROLLER_CONNECTION_H

#include <QtCore/QObject>
#include <QtCore/QVector>

#include "engine.h"
#include "controller.h"
#include "mv_base.h"
#include "journalling_object.h"

class controllerConnection;

typedef QVector<controllerConnection *> controllerConnectionVector;

class controllerConnection : public QObject, public journallingObject
{
	Q_OBJECT
public:

	controllerConnection( controller * _controller );
	controllerConnection( int _controllerId );

	virtual ~controllerConnection();

	inline controller * getController( void )
	{
		return m_controller;
	}

	inline void setController( controller * _controller );

	inline void setController( int _controllerId );

	float currentValue( int _offset )
	{
		return m_controller->currentValue( _offset );
	}

	inline bool isFinalized( void )
	{
		return m_controllerId < 0;
	}

	static void finalizeConnections( void );

	virtual void saveSettings( QDomDocument & _doc, QDomElement & _this );
	virtual void loadSettings( const QDomElement & _this );
	virtual QString nodeName( void ) const;

protected:
	//virtual controllerDialog * createDialog( QWidget * _parent );

	controller * m_controller;
	int m_controllerId;
	bool m_ownsController;

	static controllerConnectionVector s_connections;

signals:
	// The value changed while the mixer isn't running (i.e: MIDI CC)
	void valueChanged( void );

	friend class controllerConnectionDialog;
};

#endif

