/*
 * MemoryPool.cpp
 *
 * Copyright (c) 2018 Lukas W <lukaswhl/at/gmail.com>
 *
 * This file is part of LMMS - https://lmms.io
 *
 * This file is licensed under the MIT license. See LICENSE.MIT.txt file in the
 * project root for details.
 *
 */

#include "MemoryPool.h"

#include <QtCore/QDebug>

#include "libcds.h"
#include <cds/container/vyukov_mpmc_cycle_queue.h>

#include "Memory.h"

class _MemoryPool_Private : MmAllocator<char>
{
	using Alloc = MmAllocator<char>;
public:
	_MemoryPool_Private(size_t size, size_t nmemb)
		: m_elementSize(size)
		, m_numElms(nmemb)
		, m_freelist(nmemb)
	{
		CDS_THREAD_GUARD();

		m_buffer = new char[m_elementSize * m_numElms];
		for (size_t i = 0; i < m_numElms; i++) {
			m_freelist.push(m_buffer + (i * m_elementSize));
		}
	}

	~_MemoryPool_Private()
	{
		char* ptr = nullptr;
		while (m_freelist.pop(ptr)) {
			if (! is_from_pool(ptr)) {
				Alloc::deallocate(ptr, m_elementSize);
			}
		}
		delete[] m_buffer;
	}

	void * allocate()
	{
		void* ptr = allocate_bounded();
		if (ptr) {
			return ptr;
		} else {
			qWarning() << "MemoryPool exhausted";
			return Alloc::allocate(m_elementSize);
		}
	}

	void * allocate_bounded()
	{
		char* ptr = nullptr;
		m_freelist.pop(ptr);
		return ptr;
	}

	void deallocate(void * ptr)
	{
		if (is_from_pool(ptr)) {
			bool pushed = m_freelist.push(reinterpret_cast<char*>(ptr));
			assert(pushed); Q_UNUSED(pushed);
		} else {
			do_deallocate(ptr);
		}
	}

private:
	void* do_allocate()
	{
		return Alloc::allocate(m_elementSize);
	}
	void do_deallocate(void* ptr)
	{
		Alloc::deallocate(reinterpret_cast<char*>(ptr), m_elementSize);
	}

	bool is_from_pool(void* ptr)
	{
		auto buff = reinterpret_cast<uintptr_t>(m_buffer);
		auto p = reinterpret_cast<uintptr_t>(ptr);
		return p >= buff && p < (buff + (m_elementSize * m_numElms));
	}

	const size_t m_elementSize;
	const size_t m_numElms;

	char* m_buffer;
	cds::container::VyukovMPMCCycleQueue<char*> m_freelist;
};

_MemoryPool_Base::_MemoryPool_Base( size_t size, size_t nmemb )
	: _imp(new _MemoryPool_Private(size, nmemb))
{}

_MemoryPool_Base::~_MemoryPool_Base()
{}

void * _MemoryPool_Base::allocate()
{
	return _imp->allocate();
}

void *_MemoryPool_Base::allocate_bounded()
{
	return _imp->allocate_bounded();
}

void _MemoryPool_Base::deallocate(void * ptr)
{
	return _imp->deallocate(ptr);
}

