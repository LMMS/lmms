/*
 * LocklessList.h - list with lockless push and pop
 *
 * Copyright (c) 2016 Javier Serrano Polo <javier@jasp.net>
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

#ifndef LOCKLESS_LIST_H
#define LOCKLESS_LIST_H

#include <QAtomicPointer>

template<typename T>
class LocklessList
{
public:
	struct Element
	{
		T value;
		Element * next;
	} ;

	void push( T value )
	{
		Element * e = new Element;
		e->value = value;

		do
		{
#if QT_VERSION >= 0x050000
			e->next = m_first.loadAcquire();
#else
			e->next = m_first;
#endif
		}
		while( !m_first.testAndSetOrdered( e->next, e ) );
	}

	Element * popList()
	{
		return m_first.fetchAndStoreOrdered( NULL );
	}

	Element * first()
	{
#if QT_VERSION >= 0x050000
		return m_first.loadAcquire();
#else
		return m_first;
#endif
	}

	void setFirst( Element * e )
	{
#if QT_VERSION >= 0x050000
		m_first.storeRelease( e );
#else
		m_first = e;
#endif
	}


private:
	QAtomicPointer<Element> m_first;

} ;


#endif
