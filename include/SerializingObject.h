/*
 * SerializingObject.h - declaration of class SerializingObject
 *
 * Copyright (c) 2008-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef _SERIALIZING_OBJECT_H
#define _SERIALIZING_OBJECT_H

#include <QtCore/QString>

#include "export.h"


class QDomDocument;
class QDomElement;

class SerializingObjectHook;


class EXPORT SerializingObject
{
public:
	SerializingObject();
	virtual ~SerializingObject();

	virtual QDomElement saveState( QDomDocument & _doc, QDomElement & _parent );

	virtual void restoreState( const QDomElement & _this );


	// to be implemented by actual object
	virtual QString nodeName() const = 0;

	void setHook( SerializingObjectHook * _hook );

	SerializingObjectHook* hook()
	{
		return m_hook;
	}


protected:
	// to be implemented by sub-objects
	virtual void saveSettings( QDomDocument& doc, QDomElement& element ) = 0;
	virtual void loadSettings( const QDomElement& element ) = 0;


private:
	SerializingObjectHook * m_hook;

} ;


class SerializingObjectHook
{
public:
	SerializingObjectHook() :
		m_hookedIn( NULL )
	{
	}
	virtual ~SerializingObjectHook()
	{
		if( m_hookedIn != NULL )
		{
			m_hookedIn->setHook( NULL );
		}
	}

	virtual void saveSettings( QDomDocument & _doc, QDomElement & _this ) = 0;
	virtual void loadSettings( const QDomElement & _this ) = 0;

private:
	SerializingObject * m_hookedIn;

	friend class SerializingObject;

} ;


#endif

