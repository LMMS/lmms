/*
 * Memory.cpp
 *
 * Copyright (c) 2018 Lukas W <lukaswhl/at/gmail.com>
 * Copyright (c) 2014 Simon Symeonidis <lethaljellybean/at/gmail/com>
 * Copyright (c) 2004-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include "Memory.h"

#include <QtCore/QtGlobal>
#include "rpmalloc.h"

static thread_local MemoryManager::ThreadGuard local_mm_thread_guard{};

void* MemoryManager::alloc(size_t size)
{
	// Reference local thread guard to ensure it is initialized.
	// Compilers may optimize the instance away otherwise.
	Q_UNUSED(&local_mm_thread_guard);
	Q_ASSERT_X(rpmalloc_is_thread_initialized(), "MemoryManager::alloc", "Thread not initialized");
	return rpmalloc(size);
}


void MemoryManager::free(void * ptr)
{
	Q_UNUSED(&local_mm_thread_guard);
	Q_ASSERT_X(rpmalloc_is_thread_initialized(), "MemoryManager::free", "Thread not initialized");
	return rpfree(ptr);
}

void MemoryManager::initialize()
{
	rpmalloc_initialize();
}

void MemoryManager::deinitialize()
{
	rpmalloc_finalize();
}

void MemoryManager::thread_initialize()
{
	rpmalloc_thread_initialize();
}

void MemoryManager::thread_deinitialize()
{
	rpmalloc_thread_finalize();
}

void* _AlignedAllocator_Base::alloc_impl(size_t alignment, size_t size )
{
	return rpaligned_alloc(alignment, size);
}

void _AlignedAllocator_Base::dealloc_impl(void* p)
{
	rpfree(p);
}
