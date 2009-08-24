/*
 * endian_handling.h - handle endianess
 *
 * Copyright (c) 2005-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * 
 * This file is part of Linux MultiMedia Studio - http://lmms.sourceforge.net
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


#ifndef _ENDIAN_HANDLING_H
#define _ENDIAN_HANDLING_H

#include <QtCore/QSysInfo>

#include "lmms_basics.h"


inline bool isLittleEndian()
{
	return( QSysInfo::ByteOrder == QSysInfo::LittleEndian );
}


inline Sint16 swap16IfBE( Sint16 _i )
{
	return( isLittleEndian() ? _i : ( ( _i & 0xFF ) << 8) | ( ( _i >> 8 ) & 0xFF ) );
}


inline Sint32 swap32IfBE( Sint32 _i )
{
	return( isLittleEndian() ? _i : ( ( _i & 0xff000000 ) >> 24 ) |
					( ( _i & 0x00ff0000 ) >> 8 )  |
					( ( _i & 0x0000ff00 ) << 8 )  |
					( ( _i & 0x000000ff ) << 24 ) );
}

#endif
