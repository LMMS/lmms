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
#include <bit>

namespace lmms {

inline constexpr auto DynamicSpscQueueSize = static_cast<size_t>(-1);

template <typename T, size_t N = DynamicSpscQueueSize> class LockfreeSpscQueue
{
public:
	LockfreeSpscQueue(const LockfreeSpscQueue&) = delete;
	LockfreeSpscQueue(LockfreeSpscQueue&&) = delete;
	LockfreeSpscQueue& operator=(const LockfreeSpscQueue&) = delete;
	LockfreeSpscQueue& operator=(LockfreeSpscQueue&&) = delete;

	LockfreeSpscQueue()
		requires(N != DynamicSpscQueueSize)
	= default;

	LockfreeSpscQueue(size_t size)
		requires(N == DynamicSpscQueueSize)
		: m_buffer(size)
	{
	}

	~LockfreeSpscQueue()
	{
		if (!m_shutdownFlag.test(std::memory_order_relaxed)) { shutdown(); }
	}

	auto push(T value) -> bool { return push(&value, 1); }

	auto push(const T* values, size_t size) -> bool
	{
		auto region = reserveWriteSpace(size);
		const auto totalSize = region.first.size() + region.second.size();
		if (totalSize != size) { return false; }

		std::copy_n(values, region.first.size(), region.first.data());
		std::copy_n(values + region.first.size(), region.second.size(), region.second.data());
		commitWrite(totalSize);
		return true;
	}

	auto pop() -> std::optional<T>
	{
		auto value = T{};
		return pop(&value, 1) == 0 ? std::nullopt : std::optional<T>{value};
	}

	auto pop(T* values, size_t size) -> bool
	{
		const auto region = reserveReadSpace(size);
		const auto totalSize = region.first.size() + region.second.size();
		if (totalSize != size) { return false; }

		std::copy_n(region.first.data(), region.first.size(), values);
		std::copy_n(region.second.data(), region.second.size(), values + region.first.size());
		commitRead(totalSize);
		return true;
	}

	auto reserveReadSpace(size_t requested = static_cast<size_t>(-1))
		-> std::pair<std::span<const T>, std::span<const T>>
	{
		const auto readIndex = m_readIndex.load(std::memory_order_relaxed);
		const auto writeIndex = m_writeIndex.load(std::memory_order_acquire);

		if (readIndex <= writeIndex)
		{
			const auto size = std::min(writeIndex - readIndex, requested);
			return {{&m_buffer[readIndex], size}, {}};
		}
		else
		{
			const auto tailAvailable = m_buffer.size() - readIndex;
			const auto tailSize = std::min(tailAvailable, requested);
			const auto headSize = std::min(requested - tailSize, writeIndex);
			return {{&m_buffer[readIndex], tailSize}, {&m_buffer[0], headSize}};
		}
	}

	auto reserveWriteSpace(size_t requested = static_cast<size_t>(-1)) -> std::pair<std::span<T>, std::span<T>>
	{
		const auto writeIndex = m_writeIndex.load(std::memory_order_relaxed);
		const auto readIndex = m_readIndex.load(std::memory_order_acquire);

		if (writeIndex < readIndex)
		{
			const auto size = std::min(readIndex - writeIndex - 1, requested);
			return {{&m_buffer[writeIndex], size}, {}};
		}
		else
		{
			const auto tailAvailable = m_buffer.size() - writeIndex - (readIndex == 0 ? 1 : 0);
			const auto tailSize = std::min(tailAvailable, requested);
			const auto headSize = std::min(requested - tailSize, readIndex - 1);
			return {{&m_buffer[writeIndex], tailSize}, {&m_buffer[0], headSize}};
		}
	}

	auto reserveContiguousReadSpace(size_t requested = static_cast<size_t>(-1)) -> std::span<const T>
	{
		const auto region = reserveReadSpace(requested);
		return region.first;
	}

	auto reserveContiguousWriteSpace(size_t requested = static_cast<size_t>(-1)) -> std::span<T>
	{
		const auto region = reserveWriteSpace(requested);
		return region.first;
	}

	void commitWrite(size_t count)
	{
		const auto index = m_writeIndex.load(std::memory_order_relaxed);
		const auto value = std::has_single_bit(m_buffer.size()) ? (index + count) & (m_buffer.size() - 1)
																: (index + count) % m_buffer.size();
		m_writeIndex.store(value, std::memory_order_release);

		m_dataAvailableFlag.test_and_set(std::memory_order_relaxed);
		m_dataAvailableFlag.notify_one();
	}

	void commitRead(size_t count)
	{
		const auto index = m_readIndex.load(std::memory_order_relaxed);
		const auto value = std::has_single_bit(m_buffer.size()) ? (index + count) & (m_buffer.size() - 1)
																: (index + count) % m_buffer.size();
		m_readIndex.store(value, std::memory_order_release);
	}

	void shutdown()
	{
		m_shutdownFlag.test_and_set(std::memory_order_relaxed);
		m_dataAvailableFlag.test_and_set(std::memory_order_relaxed);
		m_dataAvailableFlag.notify_one();
	}

	void waitForData()
	{
		while (empty() && !m_shutdownFlag.test(std::memory_order_relaxed))
		{
			m_dataAvailableFlag.clear(std::memory_order_relaxed);
			m_dataAvailableFlag.wait(false, std::memory_order_relaxed);
		}
	}

	auto peek() const -> const T&
	{
		const auto readIndex = m_readIndex.load(std::memory_order_acquire);
		return m_buffer[readIndex];
	}

	auto size() const -> size_t
	{
		const auto readIndex = m_readIndex.load(std::memory_order_relaxed);
		const auto writeIndex = m_writeIndex.load(std::memory_order_relaxed);
		return (writeIndex + m_buffer.size() - readIndex) % m_buffer.size();
	}

	auto empty() const -> bool { return size() == 0; }
	auto full() const -> bool { return size() == capacity(); }
	auto free() const -> size_t { return capacity() - size(); }
	auto capacity() const -> size_t { return m_buffer.size() - 1; }

private:
	std::conditional_t<N == DynamicSpscQueueSize, std::vector<T>, std::array<T, N>> m_buffer;
	alignas(64) std::atomic_size_t m_readIndex;
	alignas(64) std::atomic_size_t m_writeIndex;
	alignas(64) std::atomic_flag m_dataAvailableFlag;
	alignas(64) std::atomic_flag m_shutdownFlag;
};

} // namespace lmms

#endif // LMMS_LOCKFREE_SPSC_QUEUE_H
