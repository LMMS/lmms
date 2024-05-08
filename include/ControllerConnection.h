/*
 * ControllerConnection.h - declaration of a controller connect, which
 *              provides a definition of the link between a controller and
 *              model, also handles deferred creation of links while
 *              loading project
 *
 * Copyright (c) 2008 Paul Giblock <pgllama/at/gmail.com>
 * Copyright (c) 2010 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 *
 * This file is part of LMMS - https://lmms.io
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

#ifndef LMMS_CONTROLLER_CONNECTION_H
#define LMMS_CONTROLLER_CONNECTION_H

#include <QObject>

#include "Controller.h"
#include "JournallingObject.h"
#include "ValueBuffer.h"

#include <vector>

namespace lmms
{

class ControllerConnection;

namespace gui
{
class ControllerConnectionDialog;
}

using ControllerConnectionVector = std::vector<ControllerConnection*>;

class LMMS_EXPORT ControllerConnection : public QObject, public JournallingObject
{
	Q_OBJECT
public:

	ControllerConnection(Controller * _controller);
	ControllerConnection( int _controllerId );

	~ControllerConnection() override;

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
	
	ValueBuffer * valueBuffer()
	{
		return m_controller->valueBuffer();
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

	void saveSettings( QDomDocument & _doc, QDomElement & _this ) override;
	void loadSettings( const QDomElement & _this ) override;

	static inline const QString classNodeName()
	{
		return "connection";
	}

	QString nodeName() const override
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
	// The value changed while the audio engine isn't running (i.e: MIDI CC)
	void valueChanged();

	friend class gui::ControllerConnectionDialog;
};


} // namespace lmms

#endif // LMMS_CONTROLLER_CONNECTION_H
