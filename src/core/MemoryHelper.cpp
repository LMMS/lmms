/*
 * Copyright (c) 2014 Simon Symeonidis <lethaljellybean/at/gmail/com>
 * Copyright (c) 2008-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 *
 * This file is part of LMMS - http://lmms.io
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

#include <stdlib.h>

#include "lmms_basics.h"
#include "MemoryHelper.h"

/**
 * Allocate a number of bytes and return them.
 * @param byteNum is the number of bytes
 */
void* MemoryHelper::alignedMalloc( int byteNum )
{
	char *ptr, *ptr2, *aligned_ptr;
	int align_mask = ALIGN_SIZE - 1;

	ptr = static_cast<char*>( malloc( byteNum + ALIGN_SIZE + sizeof( int ) ) );

	if( ptr == NULL ) return NULL;

	ptr2 = ptr + sizeof( int );
	aligned_ptr = ptr2 + ( ALIGN_SIZE - ( ( size_t ) ptr2 & align_mask ) );

	ptr2 = aligned_ptr - sizeof( int );
	*( ( int* ) ptr2 ) = ( int )( aligned_ptr - ptr );

	return aligned_ptr;
}


/**
 * Free an aligned buffer
 * @param _buffer is the buffer to free
 */
void MemoryHelper::alignedFree( void* _buffer )
{
	if( _buffer )
	{
		int *ptr2 = static_cast<int*>( _buffer ) - 1;
		_buffer = static_cast<char*>( _buffer ) - *ptr2;
		free( _buffer );
	}
}

