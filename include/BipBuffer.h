/*
 * BipBuffer.h
 *
 * Copyright (c) 2025 Sotonye Atemie <sakertooth@gmail.com>
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

#ifndef LMMS_BIP_BUFFER_H
#define LMMS_BIP_BUFFER_H

#include <atomic>
#include <cassert>
#include <span>
#include <vector>

namespace lmms {

//! A circular buffer class that works by reading and writing
//! contiguous blocks of `T`.
//! Useful for when you need circular buffer mechanisms but also working with an interface that requires contiguous
//! buffers.
template <typename T, typename Container = std::vector<T>,
	typename Enable = std::enable_if<std::is_same_v<T, typename Container::value_type>>>
class BipBuffer
{
public:
	//! Reserve a region of @p size `T` elements for the producer to write into.
	//! Reserves as much available space as possible by default.
	//! Returns the reserved region or an empty one if the reserve was unsuccessful.
	auto reserve(std::size_t size = static_cast<std::size_t>(-1)) -> std::span<T>
	{
		auto readWriteRegion = m_readWriteRegion.load(std::memory_order_acquire);
		auto [readWriteRegionBegin, readWriteRegionEnd] = unpackRegion(readWriteRegion);

		auto wrapRegion = m_wrapRegion.load(std::memory_order_acquire);
		auto [wrapRegionBegin, wrapRegionEnd] = unpackRegion(wrapRegion);

		const auto readWriteRegionFreeSpace = static_cast<std::uint32_t>(m_buffer.size()) - readWriteRegionEnd;
		const auto wrapRegionFreeSpace = readWriteRegionBegin - wrapRegionEnd;

		if (size == static_cast<std::size_t>(-1)) { size = std::max(readWriteRegionFreeSpace, wrapRegionFreeSpace); }

		if (readWriteRegionFreeSpace >= wrapRegionFreeSpace)
		{
			const auto begin = m_buffer.begin() + readWriteRegionEnd;
			const auto end = begin + size;
			return begin == end ? std::span<T>{} : std::span<T>{begin, end};
		}
		else
		{
			const auto begin = m_buffer.begin() + wrapRegionEnd;
			const auto end = begin + size;
			return begin == end ? std::span<T>{} : std::span<T>{begin, end};
		}
	}

	//! Returns a contiguous region in the buffer where elements can be read from.
	auto view() const -> std::span<const T>
	{
		const auto readWriteRegion = m_readWriteRegion.load(std::memory_order_acquire);
		const auto [readWriteRegionBegin, readWriteRegionEnd] = unpackRegion(readWriteRegion);
		const auto span = std::span<const T>{m_buffer.begin() + readWriteRegionBegin, m_buffer.begin() + readWriteRegionEnd};
		return readWriteRegionBegin == readWriteRegionEnd ? std::span<const T>{} : span;
	}

	//! Acknowledges that @p size `T` elements were written into the buffer.
	void commit(std::size_t size)
	{
		if (size == 0) { return; }

		auto readWriteRegion = m_readWriteRegion.load(std::memory_order_acquire);
		auto [readWriteRegionBegin, readWriteRegionEnd] = unpackRegion(readWriteRegion);

		auto wrapRegion = m_wrapRegion.load(std::memory_order_acquire);
		auto [wrapRegionBegin, wrapRegionEnd] = unpackRegion(wrapRegion);

		const auto readWriteRegionFreeSpace = static_cast<std::uint32_t>(m_buffer.size()) - readWriteRegionEnd;
		const auto wrapRegionFreeSpace = readWriteRegionBegin - wrapRegionEnd;

		if (readWriteRegionFreeSpace >= wrapRegionFreeSpace)
		{
			assert(size <= readWriteRegionFreeSpace);
			readWriteRegionEnd += size;
			readWriteRegion = packRegion(readWriteRegionBegin, readWriteRegionEnd);
			m_readWriteRegion.store(readWriteRegion, std::memory_order_release);
		}
		else
		{
			assert(size <= wrapRegionFreeSpace);
			wrapRegionEnd += size;
			wrapRegion = packRegion(wrapRegionBegin, wrapRegionEnd);
			m_wrapRegion.store(wrapRegion, std::memory_order_release);
		}
	}

	//! Acknowledges that @p size `T` elements were read from the buffer.
	void decommit(std::size_t size)
	{
		if (size == 0) { return; }

		auto readWriteRegion = m_readWriteRegion.load(std::memory_order_acquire);
		auto [readWriteRegionBegin, readWriteRegionEnd] = unpackRegion(readWriteRegion);

		auto wrapRegion = m_wrapRegion.load(std::memory_order_acquire);
		auto [wrapRegionBegin, wrapRegionEnd] = unpackRegion(wrapRegion);

		const auto readWriteRegionSize = readWriteRegionEnd - readWriteRegionBegin;
		const auto wrapRegionSize = wrapRegionEnd - wrapRegionBegin;
		assert(readWriteRegionSize > 0 || wrapRegionSize > 0);

		if (readWriteRegionSize > 0)
		{
			assert(size <= readWriteRegionSize);
			readWriteRegionBegin += size;
			readWriteRegion = packRegion(readWriteRegionBegin, readWriteRegionEnd);
			m_readWriteRegion.store(readWriteRegion, std::memory_order_release);
		}
		else if (wrapRegionSize > 0)
		{
			assert(size <= wrapRegionSize);
			wrapRegionBegin += size;
			wrapRegionBegin = packRegion(wrapRegionBegin, wrapRegionEnd);
			m_wrapRegion.store(wrapRegion, std::memory_order_release);
		}
	}

private:
	static std::uint64_t packRegion(std::uint32_t begin, std::uint32_t end)
	{
		return static_cast<std::uint64_t>(begin) << 32 | end;
	}

	static std::pair<std::uint32_t, std::uint32_t> unpackRegion(std::uint64_t region)
	{
		return {static_cast<std::uint32_t>(region >> 32), static_cast<std::uint32_t>(region)};
	}

	Container m_buffer;
	std::atomic_uint64_t m_readWriteRegion = 0;
	std::atomic_uint64_t m_wrapRegion = 0;
};
} // namespace lmms

#endif // LMMS_CIRCULAR_BUFFER_H
