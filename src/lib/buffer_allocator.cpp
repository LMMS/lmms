/*
 * buffer_allocator.cpp - namespace bufferAllocator providing routines for own
 *                        optimized memory-management for audio-buffers
 *
 * Linux MultiMedia Studio
 * Copyright (c) 2004-2005 Tobias Doerffel <tobydox@users.sourceforge.net>
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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */


#include "qt3support.h"

#ifdef QT4

#include <QList>
#include <QMutex>

#else

#include <qpair.h>
#include <qvaluelist.h>
#include <qmutex.h>

#define qSort qHeapSort

#endif

#include <math.h>
#include <cstring>

#include "buffer_allocator.h"
#include "templates.h"
#include "mixer.h"
#include "debug.h"


struct bufDesc
{
	bool free;
	char * origPtr;
	void * buf;
	Uint32 bytes;
	Uint32 timesUsed;

} ;


inline bool operator<( const bufDesc & _bd1, const bufDesc & _bd2 )
{
	return( _bd1.timesUsed < _bd2.timesUsed );
}

#ifdef QT4

inline bool operator==( const bufDesc & _bd1, const bufDesc & _bd2 )
{
	return( memcmp( &_bd1, &_bd2, sizeof( bufDesc ) ) == 0 );
}

#else

inline bool operator!=( const bufDesc & _bd1, const bufDesc & _bd2 )
{
	return( memcmp( &_bd1, &_bd2, sizeof( bufDesc ) ) != 0 );
}

#endif


static vlist<bufDesc> s_buffers;
typedef vlist<bufDesc>::iterator bufIt;

QMutex s_buffersMutex;


const int BUFFER_ALIGN = 16;
const int BUFFER_ALIGN_MASK = BUFFER_ALIGN - 1;



void bufferAllocator::cleanUp( Uint16 _level )
{
	// first insert all unused bufs into an array
	vvector<bufDesc> bufsToRemove;
	for( bufIt it = s_buffers.begin(); it != s_buffers.end(); ++it )
	{
		if( ( *it ).free )
		{
			bufsToRemove.push_back( *it );
		}
	}

	// sort array by usage of each buffer
	// ( operator<(...) compares bufDesc::timesUsed )
	qSort( bufsToRemove );

	const Uint16 todo = tMin<Uint16>( s_buffers.size() - _level,
						bufsToRemove.size() );

	// now cleanup the first n elements of sorted array
	for( Uint16 i = 0; i < todo ; ++i )
	{
		delete[] bufsToRemove[i].origPtr;
		s_buffers.erase( qFind( s_buffers.begin(), s_buffers.end(),
							bufsToRemove[i] ) );
	}
#ifdef LMMS_DEBUG
	//printf( "cleaned up %d buffers\n", todo );
#endif
}




void bufferAllocator::free( void * _buf )
{
	s_buffersMutex.lock();

	// look for buffer
	for( bufIt it = s_buffers.begin(); it != s_buffers.end(); ++it )
	{
		if( !( *it ).free && ( *it ).buf == _buf )
		{
			++( *it ).timesUsed;
			( *it ).free = TRUE;
			break;
		}
	}

	// do clean-up if neccessary
	static Uint16 CLEANUP_LEVEL = static_cast<Uint16>( 512 / ( logf(
				mixer::inst()->framesPerAudioBuffer() ) /
								logf( 2 ) ) );
	if( s_buffers.size() > CLEANUP_LEVEL )
	{
		cleanUp( CLEANUP_LEVEL );
	}

	s_buffersMutex.unlock();
}




void * bufferAllocator::allocBytes( Uint32 _bytes )
{
	QMutexLocker ml( &s_buffersMutex );

	bufIt free_buf = s_buffers.end();

	// look whether there's a buffer matching to the one wanted and
	// find out the most used one (higher chances for being in CPU-cache)
	for( bufIt it = s_buffers.begin(); it != s_buffers.end(); ++it )
	{
		if( ( *it ).free && ( *it ).bytes == _bytes )
		{
			if( free_buf == s_buffers.end() ||
				( *it ).timesUsed > ( *free_buf ).timesUsed )
			{
				free_buf = it;
			}
		}
	}

	if( free_buf != s_buffers.end() )
	{
		( *free_buf ).free = FALSE;
		return( ( *free_buf ).buf );
	}


	// nothing so far, so we'll alloc a new (aligned) buf
	bufDesc d = { FALSE, new char[_bytes + BUFFER_ALIGN], NULL, _bytes, 0 };
	d.buf = (void *)( (size_t) d.origPtr + ( BUFFER_ALIGN -
						( (size_t) d.origPtr &
							BUFFER_ALIGN_MASK ) ) );
	s_buffers.push_back( d );
	return( d.buf );
}



