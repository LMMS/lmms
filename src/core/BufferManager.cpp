/*
 * BufferManager.cpp - A buffer caching/memory management system
 *
 * Copyright (c) 2017 Lukas W <lukaswhl/at/gmail.com>
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

#include "Engine.h"
#include "Mixer.h"
#include "MemoryManager.h"

#include <boost/lockfree/stack.hpp>


namespace {
using namespace boost::lockfree;

static const int BM_INITIAL_BUFFERS = 1024;
static const int BM_INCREMENT = 64;

static stack<sampleFrame*
	,allocator<MmAllocator<void>>
> s_available;

static fpp_t framesPerPeriod;

void extend( size_t c )
{
	s_available.reserve(c);

	size_t cc = c * framesPerPeriod;

	sampleFrame * b = MM_ALLOC( sampleFrame, cc );

	for( size_t i = 0; i < c; ++i )
	{
		s_available.push(b);
		b += framesPerPeriod;
	}
}

}



void BufferManager::init( fpp_t framesPerPeriod )
{
	::framesPerPeriod = framesPerPeriod;
	extend(BM_INITIAL_BUFFERS);
}


sampleFrame * BufferManager::acquire()
{
	sampleFrame* b;
	while (! s_available.pop(b))
	{
		qWarning( "BufferManager: out of buffers" );
		extend(BM_INCREMENT);
	}

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
	s_available.push(buf);
	//qDebug( "released buffer: %p - index %d", buf, i );
}

