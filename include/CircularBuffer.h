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
	Atomic
};

template <typename T, std::size_t N, CircularBufferSynchronization Sync = CircularBufferSynchronization::None>
class CircularBufferBase
{
public:
	auto reserveWriteRegion(std::size_t count = static_cast<std::size_t>(-1)) -> std::span<T>
	{
		const auto readIndex = loadIndex(m_readIndex, std::memory_order_acquire);
		const auto writeIndex = loadIndex(m_writeIndex, std::memory_order_relaxed);
		const auto available = (readIndex + N - writeIndex - 1) % N;
		const auto actual = std::min(count, available);
		return {&m_buffer[writeIndex], actual};
	}

	auto reserveReadRegion(std::size_t count = static_cast<std::size_t>(-1)) -> std::span<T>
	{
		const auto readIndex = loadIndex(m_readIndex, std::memory_order_relaxed);
		const auto writeIndex = loadIndex(m_writeIndex, std::memory_order_acquire);
		const auto available = (writeIndex + N - readIndex) % N;
		const auto actual = std::min(count, available);
		return {&m_buffer[readIndex], actual};
	}

	void commitWriteRegion(std::size_t count)
	{
		const auto writeIndex = loadIndex(m_writeIndex, std::memory_order_relaxed);
		storeIndex(m_writeIndex, (writeIndex + count) % N, std::memory_order_release);
	}

	void commitReadRegion(std::size_t count)
	{
		const auto readIndex = loadIndex(m_readIndex, std::memory_order_relaxed);
		storeIndex(m_readIndex, (readIndex + count) % N, std::memory_order_release);
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
	using IndexType = std::conditional_t<Sync == CircularBufferSynchronization::Atomic, std::atomic<size_t>, size_t>;

	constexpr auto loadIndex(const IndexType& index, std::memory_order order = std::memory_order_seq_cst)
	{
		if constexpr (Sync == CircularBufferSynchronization::None) { return index; }
		else if constexpr (Sync == CircularBufferSynchronization::Atomic) { return index.load(order); }
	}

	constexpr auto storeIndex(IndexType& index, std::size_t value, std::memory_order order = std::memory_order_seq_cst)
	{
		if constexpr (Sync == CircularBufferSynchronization::None) { index = value; }
		else if constexpr (Sync == CircularBufferSynchronization::Atomic) { return index.store(value, order); }
	}

	std::array<T, N> m_buffer;
	alignas(std::hardware_destructive_interference_size) IndexType m_readIndex = 0;
	alignas(std::hardware_destructive_interference_size) IndexType m_writeIndex = 0;
};
} // namespace detail

template <typename T, std::size_t N>
using CircularBuffer = detail::CircularBufferBase<T, N, detail::CircularBufferSynchronization::None>;

template <typename T, std::size_t N>
using LockfreeSpscQueue = detail::CircularBufferBase<T, N, detail::CircularBufferSynchronization::Atomic>;

} // namespace lmms