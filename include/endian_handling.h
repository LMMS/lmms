/*
 * endian_handling.h - handle endianness
 *
 * Copyright (c) 2005-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * Copyright (c) 2026      Dalton Messmer <messmer.dalton/at/gmail.com>
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

#ifndef LMMS_ENDIAN_HANDLING_H
#define LMMS_ENDIAN_HANDLING_H

#include <bit>
#include <type_traits>

namespace lmms {

constexpr bool isLittleEndian() noexcept
{
	return std::endian::native == std::endian::little;
}

constexpr bool isBigEndian() noexcept
{
	return std::endian::native == std::endian::big;
}

//! TODO C++23: Use std::byteswap
template<typename Int>
	requires (sizeof(Int) <= 4 && std::is_integral_v<Int>)
constexpr Int byteswap(Int i) noexcept
{
	if constexpr (sizeof(Int) == 1) { return i; }
	else if constexpr (sizeof(Int) == 2)
	{
		return ((i & 0xFF) << 8) | ((i >> 8) & 0xFF);
	}
	else if constexpr (sizeof(Int) == 4)
	{
		return ((i & 0xff000000) >> 24)
		     | ((i & 0x00ff0000) >> 8)
		     | ((i & 0x0000ff00) << 8)
		     | ((i & 0x000000ff) << 24);
	}
	else { static_assert(sizeof(Int) == 1, "invalid size"); }
}

template<std::endian e, typename Int>
constexpr Int byteswapIf(Int i) noexcept
{
	if constexpr (std::endian::native == e) { return byteswap(i); }
	else { return i; }
}

template<typename Int>
constexpr Int byteswapIfBE(Int i) noexcept { return byteswapIf<std::endian::big>(i); }

template<typename Int>
constexpr Int byteswapIfLE(Int i) noexcept { return byteswapIf<std::endian::little>(i); }

} // namespace lmms

#endif // LMMS_ENDIAN_HANDLING_H
