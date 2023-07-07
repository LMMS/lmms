/*
 * ControllerConnection.cpp - implementation of class controller connection
 *            which handles the link between AutomatableModels and controllers
 *
 * Copyright (c) 2008 Paul Giblock <drfaygo/at/gmail.com>
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

#include <QDomElement>
#include <QObject>


#include "Song.h"
#include "ControllerConnection.h"

namespace lmms
{


ControllerConnectionVector ControllerConnection::s_connections;



ControllerConnection::ControllerConnection(Controller * _controller) :
	m_controller( nullptr ),
	m_controllerId( -1 ),
	m_ownsController(false)
{
	if( _controller != nullptr )
	{
		setController( _controller );
	}
	else
	{
		m_controller = Controller::create( Controller::DummyController,
									nullptr );
	}
	s_connections.append( this );
}




ControllerConnection::ControllerConnection( int _controllerId ) :
	m_controller( Controller::create( Controller::DummyController, nullptr ) ),
	m_controllerId( _controllerId ),
	m_ownsController( false )
{
	s_connections.append( this );
}




ControllerConnection::~ControllerConnection()
{
	if( m_controller && m_controller->type() != Controller::DummyController )
	{
		m_controller->removeConnection( this );
	}
	s_connections.remove( s_connections.indexOf( this ) );
	if( m_ownsController )
	{
		delete m_controller;
	}
}




void ControllerConnection::setController( int /*_controllerId*/ )
{
}




void ControllerConnection::setController( Controller * _controller )
{
	if( m_ownsController && m_controller )
	{
		delete m_controller;
		m_controller = nullptr;
	}

	if( m_controller && m_controller->type() != Controller::DummyController )
	{
		m_controller->removeConnection( this );
	}

	if( !_controller )
	{
		m_controller = Controller::create( Controller::DummyController, nullptr );
	}
	else
	{
		m_controller = _controller;
	}
	m_controllerId = -1;

	if( _controller->type() != Controller::DummyController )
	{
		_controller->addConnection( this );
		QObject::connect( _controller, SIGNAL(valueChanged()),
				this, SIGNAL(valueChanged()), Qt::DirectConnection );
	}

	m_ownsController =
		(_controller->type() == Controller::MidiController);

	// If we don't own the controller, allow deletion of controller
	// to delete the connection
	if( !m_ownsController ) {
		QObject::connect( _controller, SIGNAL(destroyed()),
				this, SLOT(deleteConnection()));
	}
}



inline void ControllerConnection::setTargetName( const QString & _name )
{
	m_targetName = _name;
	if( m_controller )
	{
	//	m_controller->getMidiPort()->setName( _name );
	}
}



/*
 * A connection may not be finalized.  This means, the connection should exist,
 * but the controller does not yet exist.  This happens when loading.  Even
 * loading connections last won't help, since there can be connections BETWEEN
 * controllers. So, we remember the controller-ID and use a dummyController
 * instead.  Once the song is loaded, finalizeConnections() connects to the proper controllers
 */
void ControllerConnection::finalizeConnections()
{
	for( int i = 0; i < s_connections.size(); ++i )
	{
		ControllerConnection * c = s_connections[i];
		if ( !c->isFinalized() && c->m_controllerId <
				Engine::getSong()->controllers().size() )
		{
			c->setController( Engine::getSong()->
					controllers().at( c->m_controllerId ) );
		}
		else if (c->getController()->type() == Controller::DummyController)
		{
			delete c;
			--i;
		}
	}
}




void ControllerConnection::saveSettings( QDomDocument & _doc, QDomElement & _this )
{
	if( Engine::getSong() )
	{
		if( m_ownsController )
		{
			m_controller->saveState( _doc, _this );
		}
		else
		{
			int id = Engine::getSong()->controllers().indexOf( m_controller );
			if( id >= 0 )
			{
				_this.setAttribute( "id", id );
			}
		}
	}
}




void ControllerConnection::loadSettings( const QDomElement & _this )
{
	QDomNode node = _this.firstChild();
	if( !node.isNull() )
	{
		setController( Controller::create( node.toElement(), Engine::getSong() ) );
	}
	else
	{
		m_controllerId = _this.attribute( "id", "-1" ).toInt();
		if( m_controllerId < 0 )
		{
			qWarning( "controller index invalid\n" );
			m_controllerId = -1;
		}

		if (!Engine::getSong()->isLoadingProject()
			&& m_controllerId != -1
			&& m_controllerId < Engine::getSong()->controllers().size())
		{
			setController( Engine::getSong()->
					controllers().at( m_controllerId ) );
		}
		else
		{
			m_controller = Controller::create( Controller::DummyController, nullptr );
		}
	}
}


void ControllerConnection::deleteConnection()
{
	delete this;
}



} // namespace lmms
