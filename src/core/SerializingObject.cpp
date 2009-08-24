/*
 * SerializingObject.cpp - implementation of SerializingObject
 *
 * Copyright (c) 2008-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include "SerializingObject.h"



SerializingObject::SerializingObject() :
	m_hook( NULL )
{
}




SerializingObject::~SerializingObject()
{
	if( m_hook )
	{
		m_hook->m_hookedIn = NULL;
	}
}




QDomElement SerializingObject::saveState( QDomDocument & _doc,
							QDomElement & _parent )
{
	QDomElement _this = _doc.createElement( nodeName() );
	_parent.appendChild( _this );
	saveSettings( _doc, _this );
	if( getHook() )
	{
		getHook()->saveSettings( _doc, _this );
	}
	return _this;
}




void SerializingObject::restoreState( const QDomElement & _this )
{
	loadSettings( _this );
	if( getHook() )
	{
		getHook()->loadSettings( _this );
	}
}




void SerializingObject::setHook( SerializingObjectHook * _hook )
{
	if( m_hook )
	{
		m_hook->m_hookedIn = NULL;
	}
	m_hook = _hook;
	if( m_hook )
	{
		m_hook->m_hookedIn = this;
	}
}




void SerializingObject::saveSettings( QDomDocument &/* _doc*/,
						QDomElement &/* _this*/ )
{
}




void SerializingObject::loadSettings( const QDomElement & /* _this*/ )
{
}


