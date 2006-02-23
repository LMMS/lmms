#ifndef SINGLE_SOURCE_COMPILE

/*
 * base64.cpp - namespace base64 with methods for encoding/decoding binary data
 *              to/from base64
 *
 * Copyright (c) 2006 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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



#include "base64.h"
#include "types.h"


#ifdef QT3

namespace base64
{


void encode( const char * _data, const int _size, QString & _dst )
{
	// code mostly taken from
	// qt-x11-opensource-src-4.0.1/src/corelib/tools/qbytearray.cpp

	const char alphabet[] = "ABCDEFGH" "IJKLMNOP" "QRSTUVWX" "YZabcdef"
				"ghijklmn" "opqrstuv" "wxyz0123" "456789+/";
	const char padchar = '=';
	int padlen = 0;

	const int dsize = ( ( _size * 4 ) / 3 ) + 3;
	char * ptr = new char[dsize + 1];
	char * out = ptr;

	int i = 0;
	while( i < _size )
	{
		Uint32 chunk = 0;
		chunk |= Uint32( Uint8( _data[i++] ) ) << 16;
		if( i == dsize )
		{
			padlen = 2;
		}
		else
		{
			chunk |= Uint32( Uint8( _data[i++] ) ) << 8;
			if( i == _size )
			{
				padlen = 1;
			}
			else
			{
				chunk |= Uint32( Uint8( _data[i++] ) );
			}
		}

		Uint32 j = ( chunk & 0x00fc0000 ) >> 18;
		Uint32 k = ( chunk & 0x0003f000 ) >> 12;
		Uint32 l = ( chunk & 0x00000fc0 ) >> 6;
		Uint32 m = ( chunk & 0x0000003f );
		*out++ = alphabet[j];
		*out++ = alphabet[k];
		if( padlen > 1 )
		{
			*out++ = padchar;
		}
		else
		{
			*out++ = alphabet[l];
		}
		if( padlen > 0 )
		{
			*out++ = padchar;
		}
		else
		{
			*out++ = alphabet[m];
		}
	}

	// terminate string
	ptr[out - ptr] = 0;
	_dst = ptr;
	delete[] ptr;
}



void decode( const QString & _b64, char * * _data, int * _size )
{
	const char * src = _b64.ascii();
	const csize ssize = _b64.length();
	const csize dsize = *_size = ( ssize * 3 ) / 4;
	*_data = new char[dsize]; 

	// code mostly taken from
	// qt-x11-opensource-src-4.0.1/src/corelib/tools/qbytearray.cpp
	unsigned int buf = 0;
	int nbits = 0;
	int offset = 0;

	for( csize i = 0; i < ssize; ++i )
	{
		int ch = src[i];
		int d;

		if( ch >= 'A' && ch <= 'Z' )
		{
			d = ch - 'A';
		}
		else if( ch >= 'a' && ch <= 'z' )
		{
			d = ch - 'a' + 26;
		}
		else if( ch >= '0' && ch <= '9' )
		{
			d = ch - '0' + 52;
		}
		else if( ch == '+' )
		{
			d = 62;
		}
		else if( ch == '/' )
		{
			d = 63;
		}
		else
		{
			d = -1;
		}

		if( d != -1 )
		{
			buf = ( buf << 6 ) | (Uint32)d;
			nbits += 6;
			if( nbits >= 8 )
			{
				nbits -= 8;
				( *_data )[offset++] = buf >> nbits;
				buf &= ( 1 << nbits ) - 1;
			}
		}
	}
}


} ;

#endif


#endif
