/*
 * MemoryManager.cpp - A lightweight, generic memory manager for LMMS
 *
 * Copyright (c) 2014 Vesa Kivimäki
 * Copyright (c) 2007-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include "MemoryManager.h"
#include <QtGlobal>
#include <QReadWriteLock>
#include <stdint.h>


MemoryPoolVector MemoryManager::s_memoryPools;
QReadWriteLock MemoryManager::s_poolMutex;
PointerInfoMap MemoryManager::s_pointerInfo;
QMutex MemoryManager::s_pointerMutex;


bool MemoryManager::init()
{
	s_memoryPools.reserve( 64 );
	s_pointerInfo.reserve( 4096 );
	// construct first MemoryPool and allocate memory
	MemoryPool m ( MM_INITIAL_CHUNKS );
	m.m_pool = MemoryHelper::alignedMalloc( MM_INITIAL_CHUNKS * MM_CHUNK_SIZE );
	s_memoryPools.append( m );
	return true;
}


void * MemoryManager::alloc( size_t size )
{
	if( !size )
	{
		return NULL;
	}

	int requiredChunks = size / MM_CHUNK_SIZE + ( size % MM_CHUNK_SIZE > 0 ? 1 : 0 );

	MemoryPool * mp = NULL;
	void * ptr = NULL;

	MemoryPoolVector::iterator it = s_memoryPools.begin();

	s_poolMutex.lockForRead();
	while( it != s_memoryPools.end() && !ptr )
	{
		ptr = ( *it ).getChunks( requiredChunks );
		if( ptr )
		{
			mp = &( *it );
		}
		++it;
	}
	s_poolMutex.unlock();

	if( ptr )
	{
		s_pointerMutex.lock();
		PtrInfo p;
			p.chunks = requiredChunks;
			p.memPool = mp;
		s_pointerInfo[ptr] = p;
		s_pointerMutex.unlock();
		return ptr;
	}

	// can't find enough chunks in existing pools, so
	// create a new pool that is guaranteed to have enough chunks
	int moreChunks = qMax( requiredChunks, MM_INCREMENT_CHUNKS );
	int i = MemoryManager::extend( moreChunks );

	mp = &s_memoryPools[i];
	ptr = s_memoryPools[i].getChunks( requiredChunks );
	if( ptr )
	{
		s_pointerMutex.lock();
		PtrInfo p;
			p.chunks = requiredChunks;
			p.memPool = mp;
		s_pointerInfo[ptr] = p;
		s_pointerMutex.unlock();
		return ptr;
	}
	// still no luck? something is horribly wrong
	qFatal( "MemoryManager.cpp: Couldn't allocate memory: %d chunks asked", requiredChunks );
	return NULL;
}


void MemoryManager::free( void * ptr )
{
	if( !ptr )
	{
		return; // Null pointer deallocations are OK but do not need to be handled
	}

	// fetch info on the ptr and remove
	s_pointerMutex.lock();
	if( ! s_pointerInfo.contains( ptr ) ) // if we have no info on ptr, fail loudly
	{
		qFatal( "MemoryManager: Couldn't find pointer info for pointer: %p", ptr );
	}
	PtrInfo p = s_pointerInfo[ptr];
	s_pointerInfo.remove( ptr );
	s_pointerMutex.unlock();

	p.memPool->releaseChunks( ptr, p.chunks );
}


int MemoryManager::extend( int chunks )
{
	MemoryPool m ( chunks );
	m.m_pool = MemoryHelper::alignedMalloc( chunks * MM_CHUNK_SIZE );

	s_poolMutex.lockForWrite();
	s_memoryPools.append( m );
	int i = s_memoryPools.size() - 1;
	s_poolMutex.unlock();

	return i;
}


void MemoryManager::cleanup()
{
	for( MemoryPoolVector::iterator it = s_memoryPools.begin(); it != s_memoryPools.end(); ++it )
	{
		MemoryHelper::alignedFree( ( *it ).m_pool );
		MemoryHelper::alignedFree( ( *it ).m_free );
	}
}


void * MemoryPool::getChunks( int chunksNeeded )
{
	if( chunksNeeded > m_chunks ) // not enough chunks in this pool?
	{
		return NULL;
	}

	m_mutex.lock();

	// now find out if we have a long enough sequence of chunks in this pool
	char last = 0;
	intptr_t n = 0;
	intptr_t index = -1;
	bool found = false;

	for( int i = 0; i < m_chunks; ++i )
	{
		if( m_free[i] )
		{
			if( !last )
			{
				index = i;
			}

			++n;
			if( n >= chunksNeeded )
			{
				found = true;
				break;
			}
		}
		else
		{
			n = 0;
		}

		last = m_free[i];
	}

	if( found ) // if enough chunks found, return pointer to chunks
	{
		// set chunk flags to false so we know the chunks are in use
		for( intptr_t i = 0; i < chunksNeeded; ++i )
		{
			m_free[ index + i ] = 0;
		}
		m_mutex.unlock();
		return (char*)m_pool + ( index * MM_CHUNK_SIZE );
	}
	m_mutex.unlock();
	return NULL; // out of stock, come again tomorrow!
}


void MemoryPool::releaseChunks( void * ptr, int chunks )
{
	m_mutex.lock();

	intptr_t start = ( (intptr_t)ptr - (intptr_t)m_pool ) / MM_CHUNK_SIZE;
	if( start < 0 )
	{
		qFatal( "MemoryManager: error at releaseChunks() - corrupt pointer info?" );
	}

	memset( &m_free[ start ], 1, chunks );

	m_mutex.unlock();
}
