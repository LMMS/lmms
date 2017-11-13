/*
 * LocklessAllocator.cpp - allocator with lockless alloc and free
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

#include "LocklessAllocator.h"

#include <stdio.h>
#include <strings.h>

#include "lmmsconfig.h"


static const size_t SIZEOF_SET = sizeof( int ) * 8;


static size_t align( size_t size, size_t alignment )
{
	size_t misalignment = size % alignment;
	if( misalignment )
	{
		size += alignment - misalignment;
	}
	return size;
}




LocklessAllocator::LocklessAllocator( size_t nmemb, size_t size )
{
	m_capacity = align( nmemb, SIZEOF_SET );
	m_elementSize = align( size, sizeof( void * ) );
	m_pool = new char[m_capacity * m_elementSize];

	m_freeStateSets = m_capacity / SIZEOF_SET;
	m_freeState = new AtomicInt[m_freeStateSets];

	m_available = m_capacity;
}




LocklessAllocator::~LocklessAllocator()
{
	int available = m_available;
	if( available != m_capacity )
	{
		fprintf( stderr, "LocklessAllocator: "
				"Destroying with elements still allocated\n" );
	}

	delete[] m_pool;
	delete[] m_freeState;
}




#ifdef LMMS_BUILD_WIN32
static int ffs( int i )
{
	if( !i )
	{
		return 0;
	}
	for( int j = 0;; )
	{
		if( i & 1 << j++ )
		{
			return j;
		}
	}
}
#endif




void * LocklessAllocator::alloc()
{
	int available;
	do
	{
		available = m_available;
		if( !available )
		{
			fprintf( stderr, "LocklessAllocator: No free space\n" );
			return NULL;
		}
	}
	while( !m_available.testAndSetOrdered( available, available - 1 ) );

	size_t startIndex = m_startIndex.fetchAndAddOrdered( 1 )
							% m_freeStateSets;
	for( size_t set = startIndex;; set = ( set + 1 ) % m_freeStateSets )
	{
		for( int freeState = m_freeState[set]; freeState != -1;
						freeState = m_freeState[set] )
		{
			int bit = ffs( ~freeState ) - 1;
			if( m_freeState[set].testAndSetOrdered( freeState,
							freeState | 1 << bit ) )
			{
				return m_pool + ( SIZEOF_SET * set + bit )
								* m_elementSize;
			}
		}
	}
}




void LocklessAllocator::free( void * ptr )
{
	ptrdiff_t diff = (char *)ptr - m_pool;
	if( diff < 0 || diff % m_elementSize )
	{
invalid:
		fprintf( stderr, "LocklessAllocator: Invalid pointer\n" );
		return;
	}
	size_t offset = diff / m_elementSize;
	if( offset >= m_capacity )
	{
		goto invalid;
	}
	size_t set = offset / SIZEOF_SET;
	int bit = offset % SIZEOF_SET;
	int mask = 1 << bit;
	int prevState = m_freeState[set].fetchAndAndOrdered( ~mask );
	if ( !( prevState & mask ) )
	{
		fprintf( stderr, "LocklessAllocator: Block not in use\n" );
		return;
	}
	m_available.fetchAndAddOrdered( 1 );
}
