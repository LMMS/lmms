/*
 * endian.h - handle endianess-problems
 *
 * Copyright (c) 2005 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */


#ifndef _ENDIAN_HANDLING_H
#define _ENDIAN_HANDLING_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "types.h"


inline bool isLittleEndian( void )
{
#ifdef WORDS_BIGENDIAN
	return( FALSE );
#else
	return( TRUE );
#endif
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
