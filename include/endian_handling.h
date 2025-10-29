/*
 * endian_handling.h - handle endianness
 *
 * Copyright (c) 2005-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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
#include <QSysInfo>


namespace lmms
{


inline bool isLittleEndian()
{
	return( QSysInfo::ByteOrder == QSysInfo::LittleEndian );
}


inline int16_t swap16IfBE( int16_t i )
{
	return( isLittleEndian() ? i : ( ( i & 0xFF ) << 8) | ( ( i >> 8 ) & 0xFF ) );
}


inline int32_t swap32IfBE( int32_t i )
{
	return( isLittleEndian() ? i : ( ( i & 0xff000000 ) >> 24 ) |
					( ( i & 0x00ff0000 ) >> 8 )  |
					( ( i & 0x0000ff00 ) << 8 )  |
					( ( i & 0x000000ff ) << 24 ) );
}


} // namespace lmms

#endif // LMMS_ENDIAN_HANDLING_H
