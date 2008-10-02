#ifndef SINGLE_SOURCE_COMPILE

/*
 * controller_connection.cpp - implementation of class controller connection 
 *            which handles the link between automatableModels and controllers
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

#include <QtXml/QDomElement>
#include <QtCore/QObject>
#include <QtCore/QVector>


#include "song.h"
#include "engine.h"
#include "mixer.h"
#include "controller_connection.h"


controllerConnectionVector controllerConnection::s_connections;



controllerConnection::controllerConnection( controller * _controller ) :
    m_controllerId( -1 ),
	m_ownsController( FALSE )
{
    if( _controller != NULL )
    {
        setController( _controller );
    }
    else
    {
        m_controller = controller::create( controller::DummyController, NULL );
    }
	s_connections.append( this );
}




controllerConnection::controllerConnection( int _controllerId ) :
	m_controller( controller::create( controller::DummyController, NULL ) ),
    m_controllerId( _controllerId ),
	m_ownsController( FALSE )
{
	s_connections.append( this );
}




controllerConnection::~controllerConnection()
{
	s_connections.remove( s_connections.indexOf( this ) );
	if( m_ownsController )
	{
		delete m_controller;
	}
}




void controllerConnection::setController( int /*_controllerId*/ )
{
}




void controllerConnection::setController( controller * _controller )
{
	if( m_ownsController && m_controller )
	{
		delete m_controller;
	}

	if( !_controller )
	{
		m_controller = controller::create( controller::DummyController, NULL );
	}
	else
	{
		m_controller = _controller;
	}
	m_controllerId = -1;

	if( _controller->type() != controller::DummyController )
	{
		QObject::connect( _controller, SIGNAL( valueChanged() ),
				this, SIGNAL( valueChanged() ) );
	}

	m_ownsController = 
			( _controller->type() == controller::MidiController );

	// If we don't own the controller, allow deletion of controller
	// to delete the connection
	if( !m_ownsController ) {
		QObject::connect( _controller, SIGNAL( destroyed() ),
				this, SLOT( deleteConnection() ) );
	}
}



inline void controllerConnection::setTargetName( const QString & _name )
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
void controllerConnection::finalizeConnections( void )
{
	for( int i = 0; i < s_connections.size(); ++i )
    {
        controllerConnection * c = s_connections[i];
        if ( !c->isFinalized() )
        {
            c->setController( engine::getSong()->controllers().at( c->m_controllerId ) );
        }
    }
}




void controllerConnection::saveSettings( QDomDocument & _doc, QDomElement & _this )
{
    if( engine::getSong() ) {
		if( m_ownsController )
		{
			m_controller->saveState( _doc, _this );
		}
		else
		{
			int id = engine::getSong()->controllers().indexOf( m_controller );
			if(id >= 0 )
			{
				_this.setAttribute( "id", id );
			}
		}
    }
}




void controllerConnection::loadSettings( const QDomElement & _this )
{
	QDomNode node = _this.firstChild();
	if( !node.isNull() )
	{
		setController( controller::create( node.toElement(), engine::getSong() ) );
	}
	else
	{
		if( _this.attribute( "id" ).toInt() >= 0 )
		{
			m_controllerId = _this.attribute( "id" ).toInt();
		}
		else
		{
			qWarning( "controller index invalid\n" );
			m_controllerId = -1;
		}
		m_controller = controller::create( controller::DummyController, NULL );
	}
}


void controllerConnection::deleteConnection( void )
{
	delete this;
}

QString controllerConnection::nodeName( void ) const
{
	return( "connection" );
}


/*
controllerDialog * controller::createDialog( QWidget * _parent )
{
	controllerDialog * d = new controllerDialog( this, _parent );

	return d;
}
*/

#include "moc_controller_connection.cxx"


#endif
