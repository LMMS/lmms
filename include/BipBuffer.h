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

#include <span>
#include <utility>
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
		if (size == static_cast<std::size_t>(-1))
		{
			size = m_wrapRegion.empty() ? m_buffer.size() - m_readWriteRegion.endIndex()
										: m_readWriteRegion.beginIndex() - m_wrapRegion.endIndex();
		}

		if (m_readWriteRegion.endIndex() + size <= m_buffer.size())
		{
			return reserveRegionSpace(m_readWriteRegion, size);
		}

		if (m_wrapRegion.endIndex() + size <= m_readWriteRegion.beginIndex())
		{
			return reserveRegionSpace(m_wrapRegion, size);
		}

		return std::span<T>{};
	}

	//! Returns a contiguous region in the buffer where elements can be read from.
	auto view() const -> std::span<const T>
	{
		const auto begin = m_buffer.begin() + m_readWriteRegion.beginIndex();
		const auto end = m_buffer.begin() + m_readWriteRegion.endIndex();
		return std::span<const T>{begin, end};
	}

	//! Acknowledges that @p size `T` elements were written into the buffer.
	void commit(std::size_t size)
	{
		if (!m_wrapRegion.empty())
		{
			assert(size <= m_readWriteRegion.beginIndex() - m_wrapRegion.endIndex());
			m_wrapRegion.grow(size);
			return;
		}

		assert(size <= m_buffer.size() - m_readWriteRegion.endIndex());
		m_readWriteRegion.grow(size);
	}

	//! Acknowledges that @p size `T` elements were read from the buffer.
	void decommit(std::size_t size)
	{
		assert(size <= m_readWriteRegion.length());
		m_readWriteRegion.shrink(size);
		if (m_readWriteRegion.empty()) { m_readWriteRegion = std::exchange(m_wrapRegion, Region{}); }
	}

private:
	class Region
	{
	public:
		auto grow(std::size_t size) { m_endIndex += size; }
		auto shrink(std::size_t size) { m_beginIndex += size; }

		auto length() const { return m_endIndex - m_beginIndex; }
		auto empty() const { return m_beginIndex == m_endIndex; }

		auto beginIndex() const { return m_beginIndex; }
		auto endIndex() const { return m_endIndex; }

	private:
		std::size_t m_beginIndex = 0;
		std::size_t m_endIndex = 0;
	};

	auto reserveRegionSpace(Region region, std::size_t size) -> std::span<T>
	{
		const auto begin = m_buffer.begin() + region.endIndex();
		const auto end = begin + size;
		return {begin, end};
	}

	Container m_buffer;
	Region m_readWriteRegion;
	Region m_wrapRegion;
};

} // namespace lmms

#endif // LMMS_CIRCULAR_BUFFER_H
