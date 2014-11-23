/*
 * ControllerConnection.h - declaration of a controller connect, which
 *              provides a definition of the link between a controller and
 *              model, also handles deferred creation of links while
 *              loading project
 *
 * Copyright (c) 2008 Paul Giblock <pgllama/at/gmail.com>
 * Copyright (c) 2010 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifndef _CONTROLLER_CONNECTION_H
#define _CONTROLLER_CONNECTION_H

#include <QtCore/QObject>
#include <QtCore/QVector>

#include "Controller.h"
#include "JournallingObject.h"

class ControllerConnection;

typedef QVector<ControllerConnection *> ControllerConnectionVector;


class EXPORT ControllerConnection : public QObject, public JournallingObject
{
	Q_OBJECT
public:

	ControllerConnection( Controller * _controller );
	ControllerConnection( int _controllerId );

	virtual ~ControllerConnection();

	inline Controller * getController()
	{
		return m_controller;
	}

	void setController( Controller * _controller );

	inline void setController( int _controllerId );

	float currentValue( int _offset )
	{
		return m_controller->currentValue( _offset );
	}

	inline void setTargetName( const QString & _name );

	inline QString targetName() const
	{
		return m_targetName;
	}

	inline bool isFinalized()
	{
		return m_controllerId < 0;
	}

	static void finalizeConnections();

	virtual void saveSettings( QDomDocument & _doc, QDomElement & _this );
	virtual void loadSettings( const QDomElement & _this );

	static inline const QString classNodeName()
	{
		return "connection";
	}

	virtual QString nodeName() const
	{
		return classNodeName();
	}


public slots:
	void deleteConnection();

protected:
	//virtual controllerDialog * createDialog( QWidget * _parent );
	Controller * m_controller;
	QString m_targetName;
	int m_controllerId;
	
	bool m_ownsController;

	static ControllerConnectionVector s_connections;

signals:
	// The value changed while the mixer isn't running (i.e: MIDI CC)
	void valueChanged();

	friend class ControllerConnectionDialog;
};

#endif

