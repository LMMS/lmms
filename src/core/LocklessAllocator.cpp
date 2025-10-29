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

#include <algorithm>
#include <cstdio>

#include "lmmsconfig.h"

#ifndef LMMS_BUILD_WIN32
#include <strings.h>
#endif

namespace lmms
{

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
	m_freeState = new std::atomic_int[m_freeStateSets];
	std::fill(m_freeState, m_freeState + m_freeStateSets, 0);

	m_available = m_capacity;
	m_startIndex = 0;
}




LocklessAllocator::~LocklessAllocator()
{
	if (m_available != m_capacity)
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
	// Some of these CAS loops could probably use relaxed atomics, as discussed
	// in http://en.cppreference.com/w/cpp/atomic/atomic/compare_exchange.
	// Let's use sequentially-consistent ops to be safe for now.
	auto available = m_available.load();
	do
	{
		if( !available )
		{
			fprintf( stderr, "LocklessAllocator: No free space\n" );
			return nullptr;
		}
	}
	while (!m_available.compare_exchange_weak(available, available - 1));

	const size_t startIndex = m_startIndex++ % m_freeStateSets;
	for (size_t set = startIndex;; set = ( set + 1 ) % m_freeStateSets)
	{
		for (int freeState = m_freeState[set]; freeState != -1;)
		{
			int bit = ffs( ~freeState ) - 1;
			if (m_freeState[set].compare_exchange_weak(freeState,
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
	int prevState = m_freeState[set].fetch_and(~mask);
	if ( !( prevState & mask ) )
	{
		fprintf( stderr, "LocklessAllocator: Block not in use\n" );
		return;
	}
	++m_available;
}


} // namespace lmms