/*
 * CircularBuffer.h
 *
 * Copyright (c) 2026 saker <sakertooth@gmail.com>
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

#ifndef LMMS_CIRCULAR_BUFFER_H
#define LMMS_CIRCULAR_BUFFER_H

#include <algorithm>
#include <array>
#include <atomic>
#include <cstddef>
#include <optional>
#include <span>
#include <vector>

namespace lmms {

namespace detail {

constexpr auto DynamicCircularBufferSize = static_cast<size_t>(-1);

enum class CircularBufferSide
{
	Writer,
	Reader
};

template <typename T, typename Committer> class CircularBufferRegion
{
public:
	CircularBufferRegion(const CircularBufferRegion&) = delete;
	CircularBufferRegion(CircularBufferRegion&&) = delete;
	CircularBufferRegion& operator=(const CircularBufferRegion&) = delete;
	CircularBufferRegion& operator=(CircularBufferRegion&&) = delete;

	CircularBufferRegion(std::span<T> region, Committer comitter)
		: m_region(std::move(region))
		, m_comitter(std::move(comitter))
	{
	}

	~CircularBufferRegion()
	{
		if (!m_committed) { commit(m_region.size()); }
	}

	operator T*() { return m_region.data(); }
	operator T*() const { return m_region.data(); }

	auto operator[](size_t index) const { return m_region[index]; }
	auto operator[](size_t index) { return m_region[index]; }

	auto data() const -> const T* { return m_region.data(); }
	auto data() -> T* { return m_region.data(); }

	auto size() const -> size_t { return m_region.size(); }
	auto empty() const -> bool { return m_region.empty(); }

	void commit(size_t count)
	{
		assert(count <= m_region.size());
		m_comitter(count);
		m_region = m_region.subspan(count);
		m_committed = true;
	}

private:
	std::span<T> m_region;
	Committer m_comitter;
	bool m_committed = false;
};

class CircularBufferSerialPolicy
{
public:
	auto readIndex() const -> size_t { return m_readIndex; }
	auto writeIndex() const -> size_t { return m_writeIndex; }
	void setReadIndex(size_t value) { m_readIndex = value; }
	void setWriteIndex(size_t value) { m_writeIndex = value; }

private:
	size_t m_readIndex = 0;
	size_t m_writeIndex = 0;
};

class CircularBufferSpscPolicy
{
public:
	template <CircularBufferSide S> auto readIndex() const -> size_t
	{
		if constexpr (S == CircularBufferSide::Reader) { return m_readIndex.load(std::memory_order_relaxed); }
		else if constexpr (S == CircularBufferSide::Writer) { return m_readIndex.load(std::memory_order_acquire); }
	}

	template <CircularBufferSide S> auto writeIndex() const -> size_t
	{
		if constexpr (S == CircularBufferSide::Reader) { return m_writeIndex.load(std::memory_order_acquire); }
		else if constexpr (S == CircularBufferSide::Writer) { return m_writeIndex.load(std::memory_order_relaxed); }
	}

	void setReadIndex(size_t value) { m_readIndex.store(value, std::memory_order_release); }
	void setWriteIndex(size_t value) { m_writeIndex.store(value, std::memory_order_release); }

private:
	alignas(std::hardware_destructive_interference_size) std::atomic<size_t> m_readIndex = 0;
	alignas(std::hardware_destructive_interference_size) std::atomic<size_t> m_writeIndex = 0;
};

template <typename T, typename IndexPolicy, size_t N = DynamicCircularBufferSize> class CircularBufferBase : IndexPolicy
{
public:
	static_assert(N >= 2, "Size must at least be 2");

	CircularBufferBase()
		requires(N != DynamicCircularBufferSize)
	= default;

	CircularBufferBase(size_t size)
		requires(N == DynamicCircularBufferSize)
		: m_buffer(size)
	{
	}

	auto reserveWriteRegion(size_t count = static_cast<size_t>(-1))
	{
		const auto writeIndex = IndexPolicy::template writeIndex<CircularBufferSide::Writer>();
		const auto readIndex = IndexPolicy::template readIndex<CircularBufferSide::Writer>();
		const auto available = writeIndex < readIndex ? readIndex - writeIndex - 1
													  : m_buffer.size() - writeIndex - (readIndex == 0 ? 1 : 0);
		const auto actual = std::min(count, available);
		return CircularBufferRegion{
			std::span<T>{&m_buffer[writeIndex], actual}, [this](size_t count) { commitWriteRegion(count); }};
	}

	auto reserveReadRegion(size_t count = static_cast<size_t>(-1))
	{
		const auto writeIndex = IndexPolicy::template writeIndex<CircularBufferSide::Reader>();
		const auto readIndex = IndexPolicy::template readIndex<CircularBufferSide::Reader>();
		const auto available = readIndex <= writeIndex ? writeIndex - readIndex : m_buffer.size() - readIndex;
		const auto actual = std::min(count, available);
		return CircularBufferRegion{
			std::span<T>{&m_buffer[readIndex], actual}, [this](size_t count) { commitReadRegion(count); }};
	}

	auto push(const T* src, size_t size) -> size_t
	{
		auto region = reserveWriteRegion(size);
		std::copy_n(src, region.size(), region.data());
		return region.size();
	}

	auto pop(T* dst, size_t size) -> size_t
	{
		auto region = reserveReadRegion(size);
		std::copy_n(region.data(), region.size(), dst);
		return region.size();
	}

	auto push(T value) -> bool { return push(&value, 1) == 1; }

	auto pop() -> std::optional<T>
	{
		auto value = T{};
		return pop(&value, 1) == 1 ? std::optional<T>{std::move(value)} : std::nullopt;
	}

	auto peek() const -> const T&
	{
		const auto readIndex = IndexPolicy::template readIndex<CircularBufferSide::Reader>();
		return m_buffer[readIndex];
	}

	auto capacity() const -> size_t { return m_buffer.size(); }

	auto full() const -> bool
	{
		const auto readIndex = IndexPolicy::template readIndex<CircularBufferSide::Writer>();
		const auto writeIndex = IndexPolicy::template writeIndex<CircularBufferSide::Writer>();
		return (writeIndex + 1) % m_buffer.size() == readIndex;
	}

	auto empty() const -> bool
	{
		const auto readIndex = IndexPolicy::template readIndex<CircularBufferSide::Reader>();
		const auto writeIndex = IndexPolicy::template writeIndex<CircularBufferSide::Reader>();
		return readIndex == writeIndex;
	}

private:
	auto commitReadRegion(size_t count)
	{
		const auto readIndex = IndexPolicy::template readIndex<CircularBufferSide::Reader>();
		IndexPolicy::setReadIndex((readIndex + count) % m_buffer.size());
	}

	auto commitWriteRegion(size_t count)
	{
		const auto writeIndex = IndexPolicy::template writeIndex<CircularBufferSide::Writer>();
		IndexPolicy::setWriteIndex((writeIndex + count) % m_buffer.size());
	}

	using Buffer = std::conditional_t<N == DynamicCircularBufferSize, std::vector<T>, std::array<T, N>>;
	Buffer m_buffer;
};
} // namespace detail

template <typename T, size_t N = detail::DynamicCircularBufferSize>
using CircularBuffer = detail::CircularBufferBase<T, detail::CircularBufferSerialPolicy, N>;

template <typename T, size_t N = detail::DynamicCircularBufferSize>
using LockfreeSpscQueue = detail::CircularBufferBase<T, detail::CircularBufferSpscPolicy, N>;
} // namespace lmms

#endif // LMMS_CIRCULAR_BUFFER_H
