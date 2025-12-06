/*
 * SharedMemory.h
 *
 * Copyright (c) 2022 Dominic Clark <mrdomclark/at/gmail.com>
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
	SharedMemoryData(std::string&& key, bool readOnly);
	SharedMemoryData(std::string&& key, std::size_t size, bool readOnly);
	SharedMemoryData(std::size_t size, bool readOnly);
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
	std::size_t size_bytes() const noexcept;

private:
	std::string m_key;
	std::unique_ptr<SharedMemoryImpl> m_impl;
	void* m_ptr = nullptr;
};

} // namespace detail


template<typename T>
class SharedMemory
{
	// This is stricter than necessary, but keeps things easy for now
	static_assert(std::is_trivial_v<T>, "objects held in shared memory must be trivial");

public:
	SharedMemory() = default;
	SharedMemory(SharedMemory&&) = default;
	SharedMemory& operator=(SharedMemory&&) = default;

	void attach(std::string key)
	{
		m_data = detail::SharedMemoryData{std::move(key), std::is_const_v<T>};
	}

	void create(std::string key)
	{
		m_data = detail::SharedMemoryData{std::move(key), sizeof(T), std::is_const_v<T>};
	}

	void create()
	{
		m_data = detail::SharedMemoryData{sizeof(T), std::is_const_v<T>};
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

public:
	SharedMemory() = default;
	SharedMemory(SharedMemory&&) = default;
	SharedMemory& operator=(SharedMemory&&) = default;

	void attach(std::string key)
	{
		m_data = detail::SharedMemoryData{std::move(key), std::is_const_v<T>};
	}

	void create(std::string key, std::size_t size)
	{
		m_data = detail::SharedMemoryData{std::move(key), size * sizeof(T), std::is_const_v<T>};
	}

	void create(std::size_t size)
	{
		m_data = detail::SharedMemoryData{size * sizeof(T), std::is_const_v<T>};
	}

	void detach() noexcept
	{
		m_data = detail::SharedMemoryData{};
	}

	const std::string& key() const noexcept { return m_data.key(); }
	T* get() const noexcept { return static_cast<T*>(m_data.get()); }

	std::size_t size() const noexcept { return m_data.size_bytes() / sizeof(T); }
	std::size_t size_bytes() const noexcept { return m_data.size_bytes(); }

	T& operator[](std::size_t index) const noexcept { return get()[index]; }
	explicit operator bool() const noexcept { return get() != nullptr; }

private:
	detail::SharedMemoryData m_data;
};

} // namespace lmms

#endif // LMMS_SHARED_MEMORY_H
