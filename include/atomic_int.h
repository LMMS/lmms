/*
 * atomic_int.h - fallback AtomicInt class when Qt is too old
 *
 * Copyright (c) 2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef _ATOMIC_INT_H
#define _ATOMIC_INT_H

#include <QtCore/QMutex>

#if QT_VERSION >= 0x040400

typedef QAtomicInt AtomicInt;

#else
// implement our own (slow) QAtomicInt class when on old Qt
class AtomicInt
{
public:
	inline AtomicInt( int _value = 0 ) :
		m_value( _value ),
		m_lock()
	{
	}

	inline AtomicInt( const AtomicInt & _copy ) :
		m_value( _copy.m_value ),
		m_lock()
	{
	}

	inline int fetchAndStoreOrdered( int _newVal )
	{
		m_lock.lock();
		const int oldVal = m_value;
		m_value = _newVal;
		m_lock.unlock();

		return oldVal;
	}

	inline int fetchAndAddOrdered( int _add )
	{
		m_lock.lock();
		const int oldVal = m_value;
		m_value += _add;
		m_lock.unlock();

		return oldVal;
	}

	inline AtomicInt & operator=( const AtomicInt & _copy )
	{
		m_lock.lock();
		m_value = _copy.m_value;
		m_lock.unlock();

		return *this;
	}


	inline AtomicInt & operator=( int _value )
	{
		m_lock.lock();
		m_value = _value;
		m_lock.unlock();

		return *this;
	}

	inline operator int() const
	{
		return m_value;
	}

private:
	volatile int m_value;
	QMutex m_lock;
} ;

#endif

#endif

