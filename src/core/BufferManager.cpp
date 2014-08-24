/*
 * BufferManager.cpp - A buffer caching/memory management system
 *
 * Copyright (c) 2014 Vesa Kivimäki <contact/dot/diizy/at/nbl/dot/fi>
 * Copyright (c) 2006-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include "BufferManager.h"


sampleFrame ** BufferManager::s_available;
QAtomicInt BufferManager::s_availableIndex = 0;
QReadWriteLock BufferManager::s_mutex;
int BufferManager::s_size;


void BufferManager::init()
{
	s_available = MM_ALLOC( sampleFrame*, BM_INITIAL_BUFFERS );

	int c = engine::mixer()->framesPerPeriod() * BM_INITIAL_BUFFERS;
	sampleFrame * b = MM_ALLOC( sampleFrame, c );
	
	for( int i = 0; i < BM_INITIAL_BUFFERS; ++i )
	{
		s_available[ i ] = b;
		b += engine::mixer()->framesPerPeriod();
	}
	s_availableIndex = BM_INITIAL_BUFFERS - 1;
	s_size = BM_INITIAL_BUFFERS;
}


sampleFrame * BufferManager::acquire()
{
	if( s_availableIndex < 0 )
	{
		s_mutex.lockForWrite();
		if( s_availableIndex < 0 ) extend( BM_INCREMENT );
		s_mutex.unlock();
	}
	s_mutex.lockForRead();
	
	sampleFrame * b = s_available[ s_availableIndex.fetchAndAddOrdered( -1 ) ];
	
	//qDebug( "acquired buffer: %p - index %d", b, int(s_availableIndex) );
	s_mutex.unlock();
	return b;
}


void BufferManager::release( sampleFrame * buf )
{
	s_mutex.lockForRead();
	s_available[ s_availableIndex.fetchAndAddOrdered( 1 ) + 1 ] = buf;
	//qDebug( "released buffer: %p - index %d", buf, int(s_availableIndex) );
	s_mutex.unlock();
}


void BufferManager::extend( int c )
{
	s_size += c;
	sampleFrame ** tmp = MM_ALLOC( sampleFrame*, s_size );
	MM_FREE( s_available );
	s_available = tmp;

	int cc = c * engine::mixer()->framesPerPeriod();
	sampleFrame * b = MM_ALLOC( sampleFrame, cc );
	
	for( int i = 0; i < c; ++i )
	{
		s_available[ s_availableIndex.fetchAndAddOrdered( 1 ) + 1 ] = b;
		b += engine::mixer()->framesPerPeriod();
	}
}
