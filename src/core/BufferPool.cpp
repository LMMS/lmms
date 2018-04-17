/*
 * BufferPool.cpp
 *
 * Copyright (c) 2018 Lukas W <lukaswhl/at/gmail.com>
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

#include "BufferPool.h"

#include <cstring>
#include "MemoryPool.h"

static std::unique_ptr<_MemoryPool_Base> pool;
const int BM_INITIAL_BUFFERS = 256;

void BufferPool::init( fpp_t framesPerPeriod )
{
	pool.reset(new _MemoryPool_Base(framesPerPeriod * sizeof(sampleFrame), BM_INITIAL_BUFFERS));
}

sampleFrame * BufferPool::acquire()
{
	return reinterpret_cast<sampleFrame*>(pool->allocate());
}

void BufferPool::clear( sampleFrame *ab, const f_cnt_t frames, const f_cnt_t offset )
{
	memset( ab + offset, 0, sizeof( *ab ) * frames );
}

#ifndef LMMS_DISABLE_SURROUND
void BufferPool::clear( surroundSampleFrame * ab, const f_cnt_t frames,
							const f_cnt_t offset )
{
	memset( ab + offset, 0, sizeof( *ab ) * frames );
}
#endif


void BufferPool::release( sampleFrame * buf )
{
	pool->deallocate(buf);
}
