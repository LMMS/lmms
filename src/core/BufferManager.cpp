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

static fpp_t framesPerPeriod;

void BufferManager::init( fpp_t framesPerPeriod )
{
	::framesPerPeriod = framesPerPeriod;
}


sampleFrame * BufferManager::acquire()
{
	return MM_ALLOC( sampleFrame, ::framesPerPeriod );
}

void BufferManager::clear( sampleFrame *ab, const f_cnt_t frames, const f_cnt_t offset )
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
	MM_FREE( buf );
}

