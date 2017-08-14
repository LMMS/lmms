/*
 * SpinLock.h
 *
 * This file is part of LMMS - https://lmms.io
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

#ifndef SPIN_LOCK_H
#define SPIN_LOCK_H

#include <QAtomicInt>


class SpinLock : private QAtomicInt
{
	enum {
		Unlocked,
		Locked,
	};

public:
	SpinLock() : QAtomicInt(Unlocked)
	{
	}

	void lock()
	{
		while (!testAndSetOrdered(Unlocked, Locked))
		{
		}
	}

	void unlock()
	{
		while (!testAndSetOrdered(Locked, Unlocked))
		{
		}
	}

	bool tryLock()
	{
		return testAndSetOrdered(Unlocked, Locked);
	}
};


class SpinLockGuard
{
	SpinLock* m_lock;

public:
	SpinLockGuard(SpinLock* lock) : m_lock(lock)
	{
		m_lock->lock();
	}

	~SpinLockGuard()
	{
		m_lock->unlock();
	}
};


#endif
