/*
 * MemoryManager.cpp
 *
 * Copyright (c) 2017 Lukas W <lukaswhl/at/gmail.com>
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


#include "MemoryManager.h"

#include <QtGlobal>
#include "rpmalloc.h"

namespace lmms
{


/// Global static object handling rpmalloc intializing and finalizing
struct MemoryManagerGlobalGuard {
	MemoryManagerGlobalGuard() {
		rpmalloc_initialize();
	}
	~MemoryManagerGlobalGuard() {
		rpmalloc_finalize();
	}
} static mm_global_guard;


namespace {
static thread_local size_t thread_guard_depth;
}

MemoryManager::ThreadGuard::ThreadGuard()
{
	if (thread_guard_depth++ == 0) {
		rpmalloc_thread_initialize();
	}
}

MemoryManager::ThreadGuard::~ThreadGuard()
{
	if (--thread_guard_depth == 0) {
		rpmalloc_thread_finalize(true);
	}
}

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


} // namespace lmms
