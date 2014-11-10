/*
 * shared_object.h - class sharedObject for use among other objects
 *
 * Copyright (c) 2006-2007 Javier Serrano Polo <jasp00/at/users.sourceforge.net>
 * Copyright (c) 2008-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef SHARED_OBJECT_H
#define SHARED_OBJECT_H

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
	static T* ref( T* object )
	{
		object->m_lock.lock();
		// TODO: Use QShared
		++object->m_referenceCount;
		object->m_lock.unlock();
		return object;
	}

	template<class T>
	static void unref( T* object )
	{
		object->m_lock.lock();
		bool deleteObject = --object->m_referenceCount <= 0;
		object->m_lock.unlock();

		if ( deleteObject )
		{
			delete object;
		}
	}

	// keep clang happy which complaines about unused member variable
	void dummy()
	{
		m_referenceCount = 0;
	}

private:
	int m_referenceCount;
	QMutex m_lock;

} ;




#endif

