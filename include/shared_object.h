/*
 * shared_object.h - class sharedObject for use among other objects
 *
 * Copyright (c) 2006-2007 Javier Serrano Polo <jasp00/at/users.sourceforge.net>
 * Copyright (c) 2008-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef SHARED_OBJECT_H
#define SHARED_OBJECT_H

#include <atomic>

class sharedObject
{
public:
	sharedObject() :
		m_referenceCount(1)
	{
	}

	virtual ~sharedObject()
	{
	}

	template<class T>
	static T* ref( T* object )
	{
		// Incrementing an atomic reference count can be relaxed since no action
		// is ever taken as a result of increasing the count.
		// Other loads and stores can be reordered around this without consequence.
		object->m_referenceCount.fetch_add(1, std::memory_order_relaxed);
		return object;
	}

	template<class T>
	static void unref( T* object )
	{
		// When decrementing an atomic reference count, we need to provide
		// two ordering guarantees:
		// 1. All reads and writes to the referenced object occur before
		//    the count reaches zero.
		// 2. Deletion occurs after the count reaches zero.
		//
		// To accomplish this, each decrement must be store-released,
		// and the final thread (which is deleting the referenced data)
		// must load-acquire those stores.
		// The simplest way to do this to give the decrement acquire-release
		// semantics.
		//
		// See https://www.boost.org/doc/libs/1_67_0/doc/html/atomic/usage_examples.html
		// for further discussion, along with a slightly more complicated
		// (but possibly more performant on weakly-ordered hardware like ARM)
		// approach.
		const bool deleteObject =
			object->m_referenceCount.fetch_sub(1, std::memory_order_acq_rel) == 1;

		if ( deleteObject )
		{
			object->deleteLater();
		}
	}

private:
	std::atomic_int m_referenceCount;
} ;

#endif
