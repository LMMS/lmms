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

#include <algorithm>
#include <span>
#include <vector>

#include "AudioEngine.h"

namespace lmms {
/**
 * @brief BipBuffer is a class that implements the bip buffer (also known as the bipartite buffer). It is a specialized
 * version of the circular buffer that only deals with contiguous regions. Users can @a reserve or @a retrieve
 * contiguous regions within the buffer for writing data to or reading data from respectively. When the read or write
 * operation is complete, users can @a commit if to acknowledge data being written into the buffer, or @a decommit to
 * acknowledge reading elements from the buffer.
 *
 * @tparam T
 */
template <typename T> class BipBuffer
{
public:
	/**
	 * @brief Construct a new Bip Buffer object that contains with a capacity of @a capacity elements (default @a
	 * capacity is @ref DEFAULT_BUFFER_SIZE)
	 *
	 * @param capacity
	 */
	BipBuffer(std::size_t capacity = DEFAULT_BUFFER_SIZE)
		: m_buffer(capacity)
	{
	}

	/**
	 * @brief Reserve a contiguous region of at most @a size elements within the buffer for writing data to.
	 *
	 * @param size
	 * @return std::span<T> the contiguous region, may have a size less than @a size
	 */
	[[nodiscard]] auto reserveWrite(std::size_t size = static_cast<std::size_t>(-1)) -> std::span<T>
	{
		const auto available = m_writeIndex < m_readIndex ? m_readIndex - m_writeIndex - 1
														  : m_buffer.size() - m_writeIndex - (m_readIndex == 0 ? 1 : 0);
		return {&m_buffer[m_writeIndex], std::min(size, available)};
	}

	/**
	 * @brief Reserve a contiguous region of at most @a size elements within the buffer for reading elements from.
	 *
	 * @param size
	 * @return std::span<const T> the contiguous region, may have a size less than @a size
	 */
	[[nodiscard]] auto reserveRead(std::size_t size = static_cast<std::size_t>(-1)) -> std::span<const T>
	{
		const auto available = m_readIndex <= m_writeIndex ? m_writeIndex - m_readIndex : m_buffer.size() - m_readIndex;
		return {&m_buffer[m_readIndex], std::min(size, available)};
	}

	/**
	 * @brief Acknowledge writing @a size elements into the buffer.
	 *
	 * @param size
	 */
	void commitWrite(std::size_t size) { m_writeIndex = (m_writeIndex + size) % m_buffer.size(); }

	/**
	 * @brief Acknowledge reading @a size elements from the buffer.
	 *
	 * @param size
	 */
	void commitRead(std::size_t size) { m_readIndex = (m_readIndex + size) % m_buffer.size(); }

private:
	std::vector<T> m_buffer;
	std::size_t m_writeIndex = 0;
	std::size_t m_readIndex = 0;
};
} // namespace lmms

#endif // LMMS_BIP_BUFFER_H
