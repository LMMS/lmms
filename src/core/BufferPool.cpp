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

#include "SampleFrame.h"

#include <cstring>
#include "MemoryPool.h"

namespace lmms
{

static std::unique_ptr<_MemoryPool_Base> pool;
const int BM_INITIAL_BUFFERS = 256;

void BufferPool::init( fpp_t framesPerPeriod )
{
	pool.reset(new _MemoryPool_Base(framesPerPeriod * sizeof(SampleFrame), BM_INITIAL_BUFFERS));
}

SampleFrame * BufferPool::acquire()
{
	return reinterpret_cast<SampleFrame*>(pool->allocate());
}

void BufferPool::release( SampleFrame * buf )
{
	pool->deallocate(buf);
}

} // namespace lmms
