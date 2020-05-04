/*
 * MemoryPool.h
 *
 * Copyright (c) 2018 Lukas W <lukaswhl/at/gmail.com>
 *
 * This file is part of LMMS - https://lmms.io
 *
 * This file is licensed under the MIT license. See LICENSE.MIT.txt file in the
 * project root for details.
 *
 */

#pragma once

#include <cstddef>
#include <memory>

class _MemoryPool_Private;

class _MemoryPool_Base
{
public:
	_MemoryPool_Base(size_t size, size_t nmemb);
	virtual ~_MemoryPool_Base();
	void * allocate();
	void * allocate_bounded();
	void deallocate(void * ptr);

private:
	const std::unique_ptr<_MemoryPool_Private> _imp;
};

/// Thread-safe, lockless memory pool. Only supports allocate(n) with n=1. When
/// the pool is exhausted, MmAllocator is used.
template<typename T>
class MemoryPool : private _MemoryPool_Base
{
public:
	using value_type = T;
	template<class U> struct rebind { typedef MemoryPool<U> other; };

	MemoryPool(size_t nmemb) : _MemoryPool_Base(sizeof(T), nmemb) {}

	T * allocate(size_t n = 1)
	{
		if (n != 1) { throw std::bad_alloc{}; }
		return reinterpret_cast<T*>(_MemoryPool_Base::allocate());
	}

	T * allocate_bounded()
	{
		return reinterpret_cast<T*>(_MemoryPool_Base::allocate_bounded());
	}

	void deallocate(T * ptr, size_t n = 1)
	{
		_MemoryPool_Base::deallocate(ptr);
	}

	template<class... Args>
	T * construct(Args&&... args)
	{
		T* buffer = allocate();
		return ::new ((void*)buffer) T(std::forward<Args>(args)...);
	}

	template<class... Args >
	T * construct_bounded(Args&&... args)
	{
		T* buffer = allocate_bounded();
		if (buffer) {
			::new ((void*)buffer) T(std::forward<Args>(args)...);
		}
		return buffer;
	}

	void destroy(T* ptr)
	{
		ptr->~T();
		deallocate(ptr);
	}
} ;
