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

#include <array>
#include <atomic>
#include <cstddef>
#include <new>
#include <optional>
#include <span>

namespace lmms {

namespace detail {
enum class CircularBufferSynchronization
{
	None,
	Spsc
};

template <typename T, std::size_t N, CircularBufferSynchronization Sync = CircularBufferSynchronization::None>
class CircularBufferBase
{
public:
	auto reserveWriteRegion(std::size_t count = static_cast<std::size_t>(-1)) -> std::span<T>
	{
		const auto readIndex = loadReadIndex<Side::Writer>();
		const auto writeIndex = loadWriteIndex<Side::Writer>();
		const auto available = (readIndex + N - writeIndex - 1) % N;
		const auto actual = std::min(count, available);
		return {&m_buffer[writeIndex], actual};
	}

	auto reserveReadRegion(std::size_t count = static_cast<std::size_t>(-1)) -> std::span<T>
	{
		const auto readIndex = loadReadIndex<Side::Reader>();
		const auto writeIndex = loadWriteIndex<Side::Reader>();
		const auto available = (writeIndex + N - readIndex) % N;
		const auto actual = std::min(count, available);
		return {&m_buffer[readIndex], actual};
	}

	void commitWriteRegion(std::size_t count)
	{
		const auto writeIndex = loadWriteIndex<Side::Writer>();
		storeWriteIndex<Side::Writer>((writeIndex + count) % N);
	}

	void commitReadRegion(std::size_t count)
	{
		const auto readIndex = loadReadIndex<Side::Reader>();
		storeReadIndex<Side::Reader>((readIndex + count) % N);
	}

	auto push(T value) -> bool
	{
		const auto region = reserveWriteRegion(1);
		if (region.empty()) { return false; }

		region[0] = std::move(value);
		commitWriteRegion(region.size());
		return true;
	}

	auto pop() -> std::optional<T>
	{
		const auto region = reserveReadRegion(1);
		if (region.empty()) { return std::nullopt; }

		const auto value = std::move(region[0]);
		commitReadRegion(region.size());
		return value;
	}

private:
	enum class Side
	{
		Writer,
		Reader
	};

	using IndexType = std::conditional_t<Sync == CircularBufferSynchronization::Spsc, std::atomic<size_t>, size_t>;

	template <Side Side> constexpr auto loadReadIndex() -> size_t
	{
		if constexpr (Sync == CircularBufferSynchronization::None) { return m_readIndex; }
		else if constexpr (Sync == CircularBufferSynchronization::Spsc)
		{
			if constexpr (Side == Side::Writer) { return m_readIndex.load(std::memory_order_acquire); }
			else if constexpr (Side == Side::Reader) { return m_readIndex.load(std::memory_order_relaxed); }
		}
	}

	template <Side Side> constexpr auto loadWriteIndex() -> size_t
	{
		if constexpr (Sync == CircularBufferSynchronization::None) { return m_writeIndex; }
		else if constexpr (Sync == CircularBufferSynchronization::Spsc)
		{
			if constexpr (Side == Side::Writer) { return m_writeIndex.load(std::memory_order_relaxed); }
			else if constexpr (Side == Side::Reader) { return m_writeIndex.load(std::memory_order_acquire); }
		}
	}

	template <Side Side> constexpr auto storeReadIndex(size_t value) -> size_t
	{
		if constexpr (Sync == CircularBufferSynchronization::None) { return m_readIndex = value; }
		else if constexpr (Sync == CircularBufferSynchronization::Spsc && Side == Side::Reader)
		{
			return m_readIndex.store(value, std::memory_order_release);
		}
	}

	template <Side Side> constexpr auto storeWriteIndex(size_t value) -> size_t
	{
		if constexpr (Sync == CircularBufferSynchronization::None) { return m_writeIndex = value; }
		else if constexpr (Sync == CircularBufferSynchronization::Spsc && Side == Side::Writer)
		{
			return m_writeIndex.store(value, std::memory_order_release);
		}
	}

	std::array<T, N> m_buffer;
	alignas(std::hardware_destructive_interference_size) IndexType m_readIndex = 0;
	alignas(std::hardware_destructive_interference_size) IndexType m_writeIndex = 0;
};
} // namespace detail

template <typename T, std::size_t N>
using CircularBuffer = detail::CircularBufferBase<T, N, detail::CircularBufferSynchronization::None>;

template <typename T, std::size_t N>
using LockfreeSpscQueue = detail::CircularBufferBase<T, N, detail::CircularBufferSynchronization::Spsc>;

} // namespace lmms