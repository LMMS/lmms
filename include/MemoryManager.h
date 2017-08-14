/*
 * MemoryManager.h - A lightweight, generic memory manager for LMMS
 *
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

#ifndef MEMORY_MANAGER_H
#define MEMORY_MANAGER_H

#include <QtCore/QVector>
#include <QtCore/QHash>
#include "MemoryHelper.h"
#include "SpinLock.h"
#include "export.h"
#include "tlsf.h"

const size_t MM_CHUNK_SIZE = 64; // granularity of managed memory
const size_t MM_INITIAL_SIZE = 1024 * 1024 * MM_CHUNK_SIZE; // how many bytes to allocate at startup - TODO: make configurable
const size_t MM_INCREMENT_SIZE = 16 * 1024 * MM_CHUNK_SIZE; // min. amount of bytes to increment at a time

typedef QVector<pool_t> MemoryPoolVector;

class EXPORT MemoryManager
{
public:
	static bool init();
	static void * alloc( size_t size );
	static void free( void * ptr );
	static void cleanup();

private:
	static tlsf_t s_tlsf;
	static MemoryPoolVector s_memoryPools;
	static SpinLock s_lock;

	static void extend( size_t required );
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
#define MM_ALLOC( type, count ) (type*) MemoryManager::alloc( sizeof( type ) * count )
// and just for symmetry...
#define MM_FREE( ptr ) MemoryManager::free( ptr )



// for debugging purposes

#define MM_OPERATORS_DEBUG												\
public: 																\
static void * operator new ( size_t size )							\
{																		\
	qDebug( "MM_OPERATORS_DEBUG: new called for %d bytes", ( int )size );		\
	return MemoryManager::alloc( size );								\
}																		\
static void * operator new[] ( size_t size )							\
{																		\
	qDebug( "MM_OPERATORS_DEBUG: new[] called for %d bytes", ( int )size );	\
	return MemoryManager::alloc( size );								\
}																		\
static void operator delete ( void * ptr )							\
{																		\
	qDebug( "MM_OPERATORS_DEBUG: delete called for %p", ptr );			\
	MemoryManager::free( ptr );										\
}																		\
static void operator delete[] ( void * ptr )						\
{																		\
	qDebug( "MM_OPERATORS_DEBUG: delete[] called for %p", ptr );		\
	MemoryManager::free( ptr );										\
}


#endif
