/*
 * BufferManager.cpp - A buffer caching/memory management system
 *
 * Copyright (c) 2014 Vesa Kivimäki <contact/dot/diizy/at/nbl/dot/fi>
 * Copyright (c) 2006-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include "BufferManager.h"

#include "MemoryManager.h"

sampleFrame ** BufferManager::s_available;
AtomicInt BufferManager::s_availableIndex = 0;
sampleFrame ** BufferManager::s_released;
AtomicInt BufferManager::s_releasedIndex = 0;
//QReadWriteLock BufferManager::s_mutex;
int BufferManager::s_size;


void BufferManager::init( fpp_t framesPerPeriod )
{
	s_available = MM_ALLOC( sampleFrame*, BM_INITIAL_BUFFERS );
	s_released = MM_ALLOC( sampleFrame*, BM_INITIAL_BUFFERS );

	int c = framesPerPeriod * BM_INITIAL_BUFFERS;
	sampleFrame * b = MM_ALLOC( sampleFrame, c );

	for( int i = 0; i < BM_INITIAL_BUFFERS; ++i )
	{
		s_available[ i ] = b;
		b += framesPerPeriod;
	}
	s_availableIndex = BM_INITIAL_BUFFERS - 1;
	s_size = BM_INITIAL_BUFFERS;
}


sampleFrame * BufferManager::acquire()
{
	if( s_availableIndex < 0 )
	{
		qFatal( "BufferManager: out of buffers" );
	}

	int i = s_availableIndex.fetchAndAddOrdered( -1 );
	sampleFrame * b = s_available[ i ];

	//qDebug( "acquired buffer: %p - index %d", b, i );
	return b;
}


void BufferManager::clear( sampleFrame * ab, const f_cnt_t frames,
							const f_cnt_t offset )
{
	memset( ab + offset, 0, sizeof( *ab ) * frames );
}


#ifndef LMMS_DISABLE_SURROUND
void BufferManager::clear( surroundSampleFrame * ab, const f_cnt_t frames,
							const f_cnt_t offset )
{
	memset( ab + offset, 0, sizeof( *ab ) * frames );
}
#endif


void BufferManager::release( sampleFrame * buf )
{
	if (buf == nullptr) return;
	int i = s_releasedIndex.fetchAndAddOrdered( 1 );
	s_released[ i ] = buf;
	//qDebug( "released buffer: %p - index %d", buf, i );
}


void BufferManager::refresh() // non-threadsafe, hence it's called periodically from mixer at a time when no other threads can interfere
{
	if( s_releasedIndex == 0 ) return;
	//qDebug( "refresh: %d buffers", int( s_releasedIndex ) );

	int j = s_availableIndex;
	for( int i = 0; i < s_releasedIndex; ++i )
	{
		++j;
		s_available[ j ] = s_released[ i ];
	}
	s_availableIndex = j;
	s_releasedIndex = 0;
}


/* // non-extensible for now
void BufferManager::extend( int c )
{
	s_size += c;
	sampleFrame ** tmp = MM_ALLOC( sampleFrame*, s_size );
	MM_FREE( s_available );
	s_available = tmp;

	int cc = c * Engine::mixer()->framesPerPeriod();
	sampleFrame * b = MM_ALLOC( sampleFrame, cc );

	for( int i = 0; i < c; ++i )
	{
		s_available[ s_availableIndex.fetchAndAddOrdered( 1 ) + 1 ] = b;
		b += Engine::mixer()->framesPerPeriod();
	}
}*/
