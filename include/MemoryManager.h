/*
 * MemoryManager.h
 *
 * Copyright (c) 2017 Lukas W <lukaswhl/at/gmail.com>
 * Copyright (c) 2014 Vesa Kivimäki
 * Copyright (c) 2007-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef LMMS_MEMORY_MANAGER_H
#define LMMS_MEMORY_MANAGER_H

#include <cstddef>
#include <vector>

#include "lmms_export.h"

namespace lmms
{


class LMMS_EXPORT MemoryManager
{
public:
	//struct ThreadGuard
	//{
	//	ThreadGuard();
	//	~ThreadGuard();
	//};

	static void * alloc( size_t size );
	static void free( void * ptr );
};

template<typename T>
struct MmAllocator
{
	using value_type = T;
	template<class U>  struct rebind {
		using other = MmAllocator<U>;
	};

	T* allocate( std::size_t n )
	{
		return reinterpret_cast<T*>( MemoryManager::alloc( sizeof(T) * n ) );
	}

	void deallocate( T* p, std::size_t )
	{
		MemoryManager::free( p );
	}

	using vector = std::vector<T, MmAllocator<T>>;
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
template<typename T>
T* MM_ALLOC(size_t count)
{
	return reinterpret_cast<T*>(
		MemoryManager::alloc(sizeof(T) * count));
}

// and just for symmetry...
template<typename T>
void MM_FREE(T* ptr)
{
	MemoryManager::free(ptr);
}


} // namespace lmms

#endif // LMMS_MEMORY_MANAGER_H
