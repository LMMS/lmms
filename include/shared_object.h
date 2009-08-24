/*
 * shared_object.h - class sharedObject for use among other objects
 *
 * Copyright (c) 2006-2007 Javier Serrano Polo <jasp00/at/users.sourceforge.net>
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


#ifndef _SHARED_OBJECT_H
#define _SHARED_OBJECT_H


#include <QtCore/QMutex>


class sharedObject
{
public:
	sharedObject() :
		m_referenceCount( 1 ),
		m_lock()
	{
	}

	virtual ~sharedObject()
	{
	}

	template<class T>
	static T * ref( T * _object )
	{
		_object->m_lock.lock();
		// TODO: Use QShared
		++_object->m_referenceCount;
		_object->m_lock.unlock();
		return( _object );
	}

	template<class T>
	static void unref( T * _object )
	{
		_object->m_lock.lock();
		bool delete_object = --_object->m_referenceCount <= 0;
		_object->m_lock.unlock();
		if ( delete_object )
		{
			delete _object;
		}
	}


private:
	int m_referenceCount;
	QMutex m_lock;

} ;




#endif

