/*
 * MemoryManager.cpp - A lightweight, generic memory manager for LMMS
 *
 * Copyright (c) 2014 Vesa Kivimäki
 * Copyright (c) 2007-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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
#include <QtDebug>

#include "tlsf.c"


MemoryPoolVector MemoryManager::s_memoryPools;
QMutex MemoryManager::s_mutex;
tlsf_t MemoryManager::s_tlsf = NULL;


bool MemoryManager::init()
{
	QMutexLocker lock(&s_mutex);
	void* mem = MemoryHelper::alignedMalloc(tlsf_size());
	if (!mem)
	{
		return false;
	}
	s_tlsf = tlsf_create(mem);
	if (!s_tlsf)
	{
		return false;
	}
	s_memoryPools.reserve(64);
	extend(MM_INITIAL_SIZE);
	return true;
}


void* MemoryManager::alloc(size_t size)
{
	QMutexLocker lock(&s_mutex);
	if (!size)
	{
		return NULL;
	}
	void* mem = tlsf_malloc(s_tlsf, size + tlsf_alloc_overhead());
	if (!mem)
	{
		extend(qMax(size, MM_INCREMENT_SIZE));
		mem = tlsf_malloc(s_tlsf, size + tlsf_alloc_overhead());
		if (!mem)
		{
			// still no luck? something is horribly wrong
			qCritical()
				<< "MemoryManager.cpp: Couldn't allocate memory:"
				<< size
				<< "bytes asked";
			return NULL;
		}
	}
	return mem;
}


void MemoryManager::free(void* ptr)
{
	QMutexLocker lock(&s_mutex);
	tlsf_free(s_tlsf, ptr);
}


void MemoryManager::extend(size_t required)
{
	qDebug()
		<< "MemoryManager::extend() called for"
		<< required
		<< "bytes";
	size_t size = required + tlsf_pool_overhead();
	pool_t pool = MemoryHelper::alignedMalloc(size);
	s_memoryPools.push_back(pool);
	tlsf_add_pool(s_tlsf, pool, size);
}


void MemoryManager::cleanup()
{
	QMutexLocker lock(&s_mutex);
	for (MemoryPoolVector::iterator it = s_memoryPools.begin(); it != s_memoryPools.end(); ++it)
	{
		MemoryHelper::alignedFree((*it));
	}
	tlsf_destroy(s_tlsf);
	MemoryHelper::alignedFree(s_tlsf);
}
