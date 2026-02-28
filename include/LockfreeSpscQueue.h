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
#include <bit>
#include <cassert>
#include <cstddef>
#include <optional>
#include <span>
#include <vector>

namespace lmms {

namespace detail {

enum class LockfreeSpscQueueRegionMode
{
	Write,
	Read
};

template <typename T, LockfreeSpscQueueRegionMode Mode, typename Committer> class LockfreeSpscQueueRegion
{
public:
	LockfreeSpscQueueRegion(const LockfreeSpscQueueRegion&) = delete;
	LockfreeSpscQueueRegion(LockfreeSpscQueueRegion&&) = delete;
	LockfreeSpscQueueRegion& operator=(const LockfreeSpscQueueRegion&) = delete;
	LockfreeSpscQueueRegion& operator=(LockfreeSpscQueueRegion&&) = delete;

	LockfreeSpscQueueRegion(std::span<T> first, std::span<T> second, Committer committer)
		: m_committer(committer)
		, m_firstRegion(first)
		, m_secondRegion(second)
		, m_size(first.size() + second.size())
	{
	}

	~LockfreeSpscQueueRegion() { m_committer(m_size); }

	auto operator[](size_t index) const -> const T&
	{
		static_assert(Mode == LockfreeSpscQueueRegionMode::Read, "Can only use this overload in read mode");
		assert(index >= 0 && index <= m_size);
		return index < m_firstRegion.size() ? m_firstRegion[index] : m_secondRegion[index - m_firstRegion.size()];
	}

	auto operator[](size_t index) -> T&
	{
		static_assert(Mode == LockfreeSpscQueueRegionMode::Write, "Can only use this overload in write mode");
		assert(index >= 0 && index <= m_size);
		return index < m_firstRegion.size() ? m_firstRegion[index] : m_secondRegion[index - m_firstRegion.size()];
	}

	auto read(T* values, size_t size) const -> bool
	{
		static_assert(Mode == LockfreeSpscQueueRegionMode::Read, "Can only use this overload in read mode");
		if (m_size < size) { return false; }

		const auto readRegion = [this, &values, &size](const std::span<T>& region) {
			const auto copySize = std::min(region.size(), size);
			std::copy_n(region.data(), copySize, values);
			size -= copySize;
		};

		readRegion(m_firstRegion);
		if (size > 0) { readRegion(m_secondRegion); }

		assert(size == 0);
		return true;
	}

	auto write(const T* values, size_t size) -> bool
	{
		static_assert(Mode == LockfreeSpscQueueRegionMode::Write, "Can only use this overload in write mode");
		if (m_size < size) { return false; }

		auto writeRegion = [this, &values, &size](std::span<T>& region) {
			const auto copySize = std::min(region.size(), size);
			std::copy_n(values, copySize, region.data());
			size -= copySize;
		};

		writeRegion(m_firstRegion);
		if (size > 0) { writeRegion(m_secondRegion); }

		assert(size == 0);
		return true;
	}

	auto size() const -> size_t { return m_size; }
	auto empty() const -> bool { return m_size == 0; }

private:
	Committer m_committer = nullptr;
	std::span<T> m_firstRegion;
	std::span<T> m_secondRegion;
	size_t m_size = 0;
};

} // namespace detail

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

	auto pop() -> std::optional<T>
	{
		auto value = T{};
		return pop(&value, 1) == 0 ? std::nullopt : std::optional<T>{value};
	}

	auto push(const T* values, size_t size) -> bool
	{
		auto region = reserveWriteSpace(size);
		return region.write(values, size);
	}

	auto pop(T* values, size_t size) -> bool
	{
		const auto region = reserveReadSpace(size);
		return region.read(values, size);
	}

	auto reserveReadSpace(size_t max = static_cast<size_t>(-1))
	{
		const auto readIndex = m_readIndex.load(std::memory_order_relaxed);
		const auto writeIndex = m_writeIndex.load(std::memory_order_acquire);
		const auto committer = [this](size_t count) { commitRead(count); };

		constexpr auto mode = detail::LockfreeSpscQueueRegionMode::Read;
		using ReadRegion = detail::LockfreeSpscQueueRegion<T, mode, decltype(committer)>;

		if (readIndex <= writeIndex)
		{
			const auto size = std::min(writeIndex - readIndex, max);
			return ReadRegion{{&m_buffer[readIndex], size}, {}, committer};
		}
		else
		{
			const auto tailAvailable = m_buffer.size() - readIndex;
			const auto tailSize = std::min(tailAvailable, max);
			const auto headSize = std::min(max - tailSize, writeIndex);
			return ReadRegion{{&m_buffer[readIndex], tailSize}, {&m_buffer[0], headSize}, committer};
		}
	}

	auto reserveWriteSpace(size_t max = static_cast<size_t>(-1))
	{
		const auto writeIndex = m_writeIndex.load(std::memory_order_relaxed);
		const auto readIndex = m_readIndex.load(std::memory_order_acquire);
		const auto committer = [this](size_t count) { commitWrite(count); };

		constexpr auto mode = detail::LockfreeSpscQueueRegionMode::Write;
		using WriteRegion = detail::LockfreeSpscQueueRegion<T, mode, decltype(committer)>;

		if (writeIndex < readIndex)
		{
			const auto size = std::min(readIndex - writeIndex - 1, max);
			return WriteRegion{{&m_buffer[writeIndex], size}, {}, committer};
		}
		else
		{
			const auto tailAvailable = m_buffer.size() - writeIndex - (readIndex == 0 ? 1 : 0);
			const auto tailSize = std::min(tailAvailable, max);
			const auto headSize = std::min(max - tailSize, readIndex - 1);
			return WriteRegion{{&m_buffer[writeIndex], tailSize}, {&m_buffer[0], headSize}, committer};
		}
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
	auto free() const -> size_t { return capacity() - size(); }
	auto capacity() const -> size_t { return m_buffer.size() - 1; }

private:
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

	std::conditional_t<N == DynamicSpscQueueSize, std::vector<T>, std::array<T, N>> m_buffer;

	// TODO: Use std::hardware_destructive_interference_size when supported by CI
	alignas(64) std::atomic_size_t m_readIndex;
	alignas(64) std::atomic_size_t m_writeIndex;
	alignas(64) std::atomic_flag m_dataAvailableFlag;
	alignas(64) std::atomic_flag m_shutdownFlag;
};

} // namespace lmms

#endif // LMMS_LOCKFREE_SPSC_QUEUE_H
