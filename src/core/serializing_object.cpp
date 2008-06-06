/*
 * serializing_object.cpp - implementation of serializingObject
 *
 * Copyright (c) 2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include <Qt/QtXml>

#include "serializing_object.h"



serializingObject::serializingObject( void ) :
	m_hook( NULL )
{
}




serializingObject::~serializingObject()
{
	if( m_hook )
	{
		m_hook->m_hookedIn = NULL;
	}
}




QDomElement serializingObject::saveState( QDomDocument & _doc,
							QDomElement & _parent )
{
	QDomElement _this = _doc.createElement( nodeName() );
	_parent.appendChild( _this );
	saveSettings( _doc, _this );
	return( _this );
}




void serializingObject::restoreState( const QDomElement & _this )
{
	// load actual settings
	loadSettings( _this );
}




void serializingObject::setHook( serializingObjectHook * _hook )
{
	m_hook = _hook;
	m_hook->m_hookedIn = this;
}


