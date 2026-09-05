/*
 * SharedMemory.h
 *
 * Copyright (c) 2022 Dominic Clark <mrdomclark/at/gmail.com>
 * Copyright (c) 2025-2026 Dalton Messmer <messmer.dalton/at/gmail.com>
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

#ifndef LMMS_SHARED_MEMORY_H
#define LMMS_SHARED_MEMORY_H

#include <memory>
#include <memory_resource>
#include <new>
#include <string>
#include <type_traits>


namespace lmms
{

namespace detail
{

class SharedMemoryImpl;

class SharedMemoryData
{
public:
	SharedMemoryData() noexcept;
	SharedMemoryData(std::string&& key, bool readOnly, bool isArray);
	SharedMemoryData(std::string&& key, std::size_t size, bool readOnly, bool isArray);
	SharedMemoryData(std::size_t size, bool readOnly, bool isArray);
	~SharedMemoryData();

	SharedMemoryData(SharedMemoryData&& other) noexcept;
	SharedMemoryData& operator=(SharedMemoryData&& other) noexcept
	{
		auto temp = std::move(other);
		swap(*this, temp);
		return *this;
	}

	friend void swap(SharedMemoryData& a, SharedMemoryData& b) noexcept
	{
		using std::swap;
		swap(a.m_key, b.m_key);
		swap(a.m_impl, b.m_impl);
		swap(a.m_ptr, b.m_ptr);
	}

	const std::string& key() const noexcept { return m_key; }
	void* get() const noexcept { return m_ptr; }
	std::size_t arraySize() const noexcept;

private:
	std::string m_key;
	std::unique_ptr<SharedMemoryImpl> m_impl;
	void* m_ptr = nullptr;
};

} // namespace detail


//! Similar to std::pmr::monotonic_buffer_resource, but the initial buffer can be replaced
class SharedMemoryResource final : public std::pmr::memory_resource
{
public:
	SharedMemoryResource() = default;
	SharedMemoryResource(void* buffer, std::size_t bufferSize) noexcept
		: m_buffer{buffer}
		, m_availableBytes{bufferSize}
		, m_initialBuffer{buffer}
		, m_initialBufferSize{bufferSize}
	{}

	SharedMemoryResource(const SharedMemoryResource&) = delete;
	auto operator=(const SharedMemoryResource&) -> SharedMemoryResource& = delete;
	SharedMemoryResource(SharedMemoryResource&&) = default;
	auto operator=(SharedMemoryResource&&) -> SharedMemoryResource& = default;

	//! Returns the buffer back to its initial state
	void reset() noexcept
	{
		m_buffer = m_initialBuffer;
		m_availableBytes = m_initialBufferSize;
	}

	//! @returns the number of bytes that can still be allocated
	auto availableBytes() const noexcept -> std::size_t { return m_availableBytes; }

	template<typename T>
	friend class SharedMemory;

private:
	//! Replaces the initial buffer
	void reset(void* newBuffer, std::size_t newBufferSize) noexcept
	{
		m_buffer = newBuffer;
		m_availableBytes = newBufferSize;
		m_initialBuffer = newBuffer;
		m_initialBufferSize = newBufferSize;
	}

	void* do_allocate(std::size_t bytes, std::size_t alignment) override
	{
		void* p = std::align(alignment, bytes, m_buffer, m_availableBytes);
		if (!p) { throw std::bad_alloc{}; }

		m_buffer = static_cast<char*>(m_buffer) + bytes;
		m_availableBytes -= bytes;
		return p;
	}
	void do_deallocate(void*, std::size_t, std::size_t) override {} // no-op
	bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override
	{
		return this == &other;
	}

private:
	void* m_buffer = nullptr;
	std::size_t m_availableBytes = 0;
	void* m_initialBuffer = nullptr;
	std::size_t m_initialBufferSize = 0;
};


template<typename T>
class SharedMemory
{
	// This is stricter than necessary, but keeps things easy for now
	static_assert(std::is_trivial_v<T>, "objects held in shared memory must be trivial");
	static_assert(sizeof(T) > 0);

public:
	SharedMemory() = default;
	SharedMemory(const SharedMemory&) = delete;
	SharedMemory& operator=(const SharedMemory&) = delete;
	SharedMemory(SharedMemory&&) = default;
	SharedMemory& operator=(SharedMemory&&) = default;

	void attach(std::string key)
	{
		m_data = detail::SharedMemoryData{std::move(key), std::is_const_v<T>, false};
	}

	void create(std::string key)
	{
		m_data = detail::SharedMemoryData{std::move(key), sizeof(T), std::is_const_v<T>, false};
	}

	void create()
	{
		m_data = detail::SharedMemoryData{sizeof(T), std::is_const_v<T>, false};
	}

	void detach() noexcept
	{
		m_data = detail::SharedMemoryData{};
	}

	const std::string& key() const noexcept { return m_data.key(); }
	T* get() const noexcept { return static_cast<T*>(m_data.get()); }

	std::size_t size() const noexcept { return get() ? 1 : 0; }
	std::size_t size_bytes() const noexcept { return get() ? sizeof(T) : 0; }

	T* operator->() const noexcept { return get(); }
	T& operator*() const noexcept { return *get(); }
	explicit operator bool() const noexcept { return get() != nullptr; }

private:
	detail::SharedMemoryData m_data;
};

template<typename T>
class SharedMemory<T[]>
{
	// This is stricter than necessary, but keeps things easy for now
	static_assert(std::is_trivial_v<T>, "objects held in shared memory must be trivial");
	static_assert(sizeof(T) > 0);

public:
	SharedMemory() = default;
	SharedMemory(const SharedMemory&) = delete;
	SharedMemory& operator=(const SharedMemory&) = delete;
	SharedMemory(SharedMemory&&) = default;
	SharedMemory& operator=(SharedMemory&&) = default;

	void attach(std::string key)
	{
		m_data = detail::SharedMemoryData{std::move(key), std::is_const_v<T>, true};
		m_resource.reset(m_data.get(), size_bytes());
	}

	void create(std::string key, std::size_t size)
	{
		m_data = detail::SharedMemoryData{std::move(key), size * sizeof(T), std::is_const_v<T>, true};
		m_resource.reset(m_data.get(), size_bytes());
	}

	void create(std::size_t size)
	{
		m_data = detail::SharedMemoryData{size * sizeof(T), std::is_const_v<T>, true};
		m_resource.reset(m_data.get(), size_bytes());
	}

	void detach() noexcept
	{
		m_data = detail::SharedMemoryData{};
		m_resource.reset(nullptr, 0);
	}

	const std::string& key() const noexcept { return m_data.key(); }
	T* get() const noexcept { return static_cast<T*>(m_data.get()); }

	std::size_t size() const noexcept { return m_data.arraySize() / sizeof(T); }
	std::size_t size_bytes() const noexcept { return m_data.arraySize(); }

	T& operator[](std::size_t index) const noexcept { return get()[index]; }
	explicit operator bool() const noexcept { return get() != nullptr; }

	SharedMemoryResource* resource() noexcept { return &m_resource; }

private:
	detail::SharedMemoryData m_data;
	SharedMemoryResource m_resource;
};

} // namespace lmms

#endif // LMMS_SHARED_MEMORY_H
