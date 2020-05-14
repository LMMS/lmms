/*
 * Memory.h
 *
 * Copyright (c) 2017 Lukas W <lukaswhl/at/gmail.com>
 * Copyright (c) 2014 Simon Symeonidis <lethaljellybean/at/gmail/com>
 * Copyright (c) 2014 Vesa Kivimäki
 * Copyright (c) 2004-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef MEMORY_MANAGER_H
#define MEMORY_MANAGER_H

#include <cstddef>
#include <vector>

#include "lmms_export.h"
#include "NiftyCounter.h"

class LMMS_EXPORT MemoryManager
{
public:
	static void * alloc( size_t size );
	static void free( void * ptr );

private:
	static void initialize();
	static void deinitialize();
	static void thread_initialize();
	static void thread_deinitialize();
public:
	typedef NiftyCounter<MemoryManager::initialize, MemoryManager::deinitialize> MmCounter;
	typedef NiftyCounterTL<MemoryManager::thread_initialize, MemoryManager::thread_deinitialize> ThreadGuard;
};

static MemoryManager::MmCounter _mm_counter;
static thread_local MemoryManager::MmCounter _mm_thread_counter;

template<typename T>
class MmAllocator
{
public:
	MmAllocator() = default;
	template< class U > MmAllocator( const MmAllocator<U>& other ) {}
	typedef T value_type;
	template<class U> struct rebind { typedef MmAllocator<U> other; };


	T* allocate( std::size_t n )
	{
		return reinterpret_cast<T*>( MemoryManager::alloc( sizeof(T) * n ) );
	}

	void deallocate( T* p, std::size_t )
	{
		MemoryManager::free( p );
	}

	typedef std::vector<T, MmAllocator<T> > vector;
};

class _AlignedAllocator_Base
{
protected:
	void* alloc_impl( size_t alignment, size_t size );
	void dealloc_impl( void* p );
};

template<typename T>
class AlignedAllocator : _AlignedAllocator_Base
{
public:
	typedef T value_type;
	template<class U>  struct rebind { typedef AlignedAllocator<U> other; };

	AlignedAllocator( size_t alignment = 16 )
		: alignment(alignment) {}

	T* allocate( std::size_t n )
	{
		return reinterpret_cast<T*>( alloc_impl( alignment, sizeof(T) * n ) );
	}

	void deallocate( T* p, std::size_t )
	{
		dealloc_impl( p );
	}

private:
	std::size_t alignment;
};

#define MM_OPERATORS								\
public: 											\
static void * operator new ( size_t size )		\
{													\
	return MemoryManager::alloc( size );			\
}													\
static void * operator new[] ( size_t size )		\
{													\
	return MemoryManager::alloc( size );			\
}													\
static void operator delete ( void * ptr )		\
{													\
	MemoryManager::free( ptr );					\
}													\
static void operator delete[] ( void * ptr )	\
{													\
	MemoryManager::free( ptr );					\
}

// for use in cases where overriding new/delete isn't a possibility
#define MM_ALLOC( type, count ) reinterpret_cast<type*>( MemoryManager::alloc( sizeof( type ) * count ) )
// and just for symmetry...
#define MM_FREE( ptr ) MemoryManager::free( ptr )

#endif
