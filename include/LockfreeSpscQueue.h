/*
 * LockfreeSpscQueue.h
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

#ifndef LMMS_LOCKFREE_SPSC_QUEUE_H
#define LMMS_LOCKFREE_SPSC_QUEUE_H

#include <algorithm>
#include <array>
#include <atomic>
#include <cstddef>
#include <optional>
#include <span>
#include <vector>

namespace lmms {

namespace {
constexpr auto DynamicSize = static_cast<size_t>(-1);
}

template <typename T, size_t N = DynamicSize> class LockfreeSpscQueue
{
public:
	LockfreeSpscQueue(const LockfreeSpscQueue&) = delete;
	LockfreeSpscQueue(LockfreeSpscQueue&&) = delete;
	LockfreeSpscQueue& operator=(const LockfreeSpscQueue&) = delete;
	LockfreeSpscQueue& operator=(LockfreeSpscQueue&&) = delete;

	LockfreeSpscQueue()
		requires(N != DynamicSize)
	= default;

	LockfreeSpscQueue(size_t size)
		requires(N == DynamicSize)
		: m_buffer(size)
	{
	}

	~LockfreeSpscQueue()
	{
		if (!m_shutdownFlag.test(std::memory_order_relaxed)) { shutdown(); }
	}

	void push(T value) { push(&value, 1); }

	void push(const T* values, size_t size)
	{
		auto region = reserveContiguousWriteSpace(size);
		std::copy_n(values, size, region.data());
		commitWrite(size);
	}

	auto tryPush(T value) -> bool { return tryPush(&value, 1); }

	auto tryPush(const T* values, size_t size) -> size_t
	{
		auto region = reserveContiguousWriteSpace(0, size);
		std::copy_n(values, region.size(), region.data());
		commitWrite(region.size());
		return region.size();
	}

	auto pop() -> T
	{
		auto value = T{};
		pop(&value, 1);
		return value;
	}

	void pop(T* values, size_t size)
	{
		auto region = reserveContiguousReadSpace(size);
		std::copy_n(region.data(), size, values);
		commitRead(size);
	}

	auto tryPop() -> std::optional<T>
	{
		auto value = T{};
		return tryPop(&value, 1) == 0 ? std::nullopt : std::optional<T>{value};
	}

	auto tryPop(T* values, size_t size) -> size_t
	{
		auto region = reserveContiguousReadSpace(0, size);
		std::copy_n(region.data(), region.size(), values);
		commitRead(region.size());
		return region.size();
	}

	auto reserveContiguousWriteSpace(size_t min = 0, size_t max = static_cast<size_t>(-1)) -> std::span<T>
	{
		const auto writeIndex = m_writeIndex.load(std::memory_order_relaxed);
		auto available = size_t{0};

		do
		{
			const auto readIndex = m_readIndex.load(std::memory_order_acquire);
			available = writeIndex < readIndex ? readIndex - writeIndex - 1
											   : m_buffer.size() - writeIndex - (readIndex == 0 ? 1 : 0);
			if (available < min)
			{
				m_spaceAvailableFlag.clear(std::memory_order_relaxed);
				m_spaceAvailableFlag.wait(false, std::memory_order_relaxed);
			}

		} while (available < min && !m_shutdownFlag.test(std::memory_order_relaxed));

		return {&m_buffer[writeIndex], std::min(available, max)};
	}

	auto reserveContiguousReadSpace(size_t min = 0, size_t max = static_cast<size_t>(-1)) -> std::span<T>
	{
		const auto readIndex = m_readIndex.load(std::memory_order_relaxed);
		auto available = size_t{0};

		do
		{
			const auto writeIndex = m_writeIndex.load(std::memory_order_acquire);
			available = readIndex <= writeIndex ? writeIndex - readIndex : m_buffer.size() - readIndex;

			if (available < min)
			{
				m_dataAvailableFlag.clear(std::memory_order_relaxed);
				m_dataAvailableFlag.wait(false, std::memory_order_relaxed);
			}

		} while (available < min && !m_shutdownFlag.test(std::memory_order_relaxed));

		return {&m_buffer[readIndex], std::min(available, max)};
	}

	void commitWrite(size_t count)
	{
		const auto index = m_writeIndex.load(std::memory_order_relaxed);
		m_writeIndex.store((index + count) % m_buffer.size());

		m_dataAvailableFlag.test_and_set(std::memory_order_relaxed);
		m_dataAvailableFlag.notify_all();
	}

	void commitRead(size_t count)
	{
		const auto index = m_readIndex.load(std::memory_order_relaxed);
		m_readIndex.store((index + count) % m_buffer.size());

		m_spaceAvailableFlag.test_and_set(std::memory_order_relaxed);
		m_spaceAvailableFlag.notify_all();
	}

	void shutdown()
	{
		m_shutdownFlag.test_and_set(std::memory_order_relaxed);
		m_dataAvailableFlag.test_and_set(std::memory_order_relaxed);
		m_spaceAvailableFlag.test_and_set(std::memory_order_relaxed);
		m_dataAvailableFlag.notify_all();
		m_spaceAvailableFlag.notify_all();
	}

	auto empty() const -> bool
	{
		const auto readIndex = m_readIndex.load(std::memory_order_relaxed);
		const auto writeIndex = m_writeIndex.load(std::memory_order_acquire);
		return readIndex == writeIndex;
	}

	auto full() const -> bool
	{
		const auto readIndex = m_readIndex.load(std::memory_order_acquire);
		const auto writeIndex = m_writeIndex.load(std::memory_order_relaxed);
		return (writeIndex + 1) % m_buffer.size() == readIndex;
	}

	auto capacity() -> size_t { return m_buffer.size(); }

private:
	std::conditional_t<N == DynamicSize, std::vector<T>, std::array<T, N>> m_buffer;
	alignas(std::hardware_destructive_interference_size) std::atomic_size_t m_readIndex;
	alignas(std::hardware_destructive_interference_size) std::atomic_size_t m_writeIndex;
	alignas(std::hardware_destructive_interference_size) std::atomic_flag m_dataAvailableFlag;
	alignas(std::hardware_destructive_interference_size) std::atomic_flag m_spaceAvailableFlag;
	alignas(std::hardware_destructive_interference_size) std::atomic_flag m_shutdownFlag;
};

} // namespace lmms

#endif // LMMS_LOCKFREE_SPSC_QUEUE_H
