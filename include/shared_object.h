/*
 * shared_object.h - class sharedObject for use among threads
 *
 * Copyright (c) 2006 Javier Serrano Polo <jasp00/at/users.sourceforge.net>
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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */


#ifndef _SHARED_OBJECT_H
#define _SHARED_OBJECT_H

#ifdef QT4

#include <QtCore/QMutex>

#else

#include <qmutex.h>

#endif




class sharedObject
{
public:
	sharedObject( void ) :
		m_reference_count( 1 )
	{
	}

	template<class T>
	static T * ref( T * _object )
	{
		_object->m_reference_mutex.lock();
		++_object->m_reference_count;
		_object->m_reference_mutex.unlock();
		return( _object );
	}

	template<class T>
	static void unref( T * _object )
	{
		_object->m_reference_mutex.lock();
		bool delete_object = --_object->m_reference_count == 0;
		_object->m_reference_mutex.unlock();
		if ( delete_object )
		{
			delete _object;
		}
	}


private:
	unsigned m_reference_count;
	QMutex m_reference_mutex;

} ;




#endif

