/*
 * MemoryManager.h - A lightweight, generic memory manager for LMMS
 *
 * Copyright (c) 2014 Vesa Kivimäki
 * Copyright (c) 2007-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 *
 */

#ifndef MEMORY_MANAGER_H
#define MEMORY_MANAGER_H

#include <new>
#include <QtCore/QVector>
#include <QtCore/QMutex>
#include <QtCore/QMap>
#include "MemoryHelper.h"


const int MM_CHUNK_SIZE = 64; // granularity of managed memory 
const int MM_INITIAL_CHUNKS = 1024 * 1024; // how many chunks to allocate at startup - TODO: make configurable
const int MM_INCREMENT_CHUNKS = 16 * 1024; // min. amount of chunks to increment at a time

struct MemoryPool
{
	void * m_pool;
	char * m_free;
	int m_chunks;
	QMutex m_mutex;
	
	MemoryPool() :
		m_pool( NULL ),
		m_free( NULL ),
		m_chunks( 0 )
	{}
	
	MemoryPool( int chunks ) : 
		m_chunks( chunks )
	{
		m_free = (char*) MemoryHelper::alignedMalloc( chunks );
		memset( m_free, 1, chunks );
	}
	
	MemoryPool( const MemoryPool & mp ) :
		m_pool( mp.m_pool ),
		m_free( mp.m_free ),
		m_chunks( mp.m_chunks ),
		m_mutex()
	{}
	
	MemoryPool & operator = ( const MemoryPool & mp )
	{
		m_pool = mp.m_pool;
		m_free = mp.m_free;
		m_chunks = mp.m_chunks;
		return *this;
	}
	
	void * getChunks( int chunksNeeded );
	void releaseChunks( void * ptr, int chunks );
};

struct PtrInfo
{
	int chunks;
	MemoryPool * memPool;
};

typedef QVector<MemoryPool> MemoryPoolVector;
typedef QMap<void*, PtrInfo> PointerInfoMap;

class MemoryManager 
{
public:
	static bool init();
	static void * alloc( size_t size );
	static void free( void * ptr );
	static int extend( int chunks ); // returns index of created pool (for use by alloc)
	static void cleanup();

private:
	static MemoryPoolVector s_memoryPools;
	static QMutex s_poolMutex;
	
	static PointerInfoMap s_pointerInfo;
	static QMutex s_pointerMutex;
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
	qDebug( "MM_OPERATORS_DEBUG: new called for %d bytes", size );		\
	return MemoryManager::alloc( size );								\
}																		\
static void * operator new[] ( size_t size )							\
{																		\
	qDebug( "MM_OPERATORS_DEBUG: new[] called for %d bytes", size );	\
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
