/*
 * serializing_object.h - declaration of class serializingObject
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


#ifndef _SERIALIZING_OBJECT_H
#define _SERIALIZING_OBJECT_H

#include <QtCore/QString>

#include "export.h"


class QDomDocument;
class QDomElement;

class serializingObjectHook;


class EXPORT serializingObject
{
public:
	serializingObject( void );
	virtual ~serializingObject();

	virtual QDomElement saveState( QDomDocument & _doc,
							QDomElement & _parent );

	virtual void restoreState( const QDomElement & _this );


	// to be implemented by actual object
	virtual QString nodeName( void ) const = 0;

	void setHook( serializingObjectHook * _hook );

	serializingObjectHook * getHook( void )
	{
		return( m_hook );
	}


protected:
	// to be implemented by sub-objects
	virtual void saveSettings( QDomDocument & _doc, QDomElement & _this )
	{
	}

	virtual void loadSettings( const QDomElement & _this )
	{
	}


private:
	serializingObjectHook * m_hook;

} ;


class serializingObjectHook
{
public:
	serializingObjectHook() :
		m_hookedIn( NULL )
	{
	}
	virtual ~serializingObjectHook()
	{
		if( m_hookedIn != NULL )
		{
			m_hookedIn->setHook( NULL );
		}
	}

	virtual void saveSettings( QDomDocument & _doc, QDomElement & _this ) = 0;
	virtual void loadSettings( const QDomElement & _this ) = 0;

private:
	serializingObject * m_hookedIn;

	friend class serializingObject;

} ;


#endif

