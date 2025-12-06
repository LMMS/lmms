/*
 * LocklessAllocator.h - allocator with lockless alloc and free
 *
 * Copyright (c) 2016 Javier Serrano Polo <javier@jasp.net>
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

#ifndef LMMS_LOCKLESS_ALLOCATOR_H
#define LMMS_LOCKLESS_ALLOCATOR_H

#include <atomic>
#include <cstddef>


namespace lmms
{


class LocklessAllocator
{
public:
	LocklessAllocator( size_t nmemb, size_t size );
	virtual ~LocklessAllocator();
	void * alloc();
	void free( void * ptr );


private:
	char * m_pool;
	size_t m_capacity;
	size_t m_elementSize;

	std::atomic_int * m_freeState;
	size_t m_freeStateSets;

	std::atomic_size_t m_available;
	std::atomic_size_t m_startIndex;

} ;




template<typename T>
class LocklessAllocatorT : private LocklessAllocator
{
public:
	LocklessAllocatorT( size_t nmemb ) :
		LocklessAllocator( nmemb, sizeof( T ) )
	{
	}

	~LocklessAllocatorT() override = default;

	T * alloc()
	{
		return (T *)LocklessAllocator::alloc();
	}

	void free( T * ptr )
	{
		LocklessAllocator::free( ptr );
	}

} ;


} // namespace lmms

#endif // LMMS_LOCKLESS_ALLOCATOR_H
