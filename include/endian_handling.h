/*
 * endian_handling.h - handle endianness
 *
 * Copyright (c) 2005-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * Copyright (c) 2026 Fawn <rubiefawn@proton.me>
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

#include <cstdint>
#include <bit>

namespace lmms
{

constexpr std::int16_t swap16IfBE(std::int16_t i)
{
	if constexpr (std::endian::native == std::endian::big)
	{
		return (i & 0x00ff) << 8 | (i & 0xff00) >> 8;
	}
	else { return i; }
}

constexpr std::int32_t swap32IfBE(std::int32_t i)
{
	if constexpr (std::endian::native == std::endian::big)
	{
		return (i & 0xff000000) >> 24 | (i & 0x00ff0000) >> 8 | (i & 0x0000ff00) << 8 | (i & 0x000000ff) << 24;
	}
	else { return i; }
}

} // namespace lmms

#endif // LMMS_ENDIAN_HANDLING_H
