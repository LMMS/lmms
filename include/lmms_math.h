/*
 * lmms_math.h - defines math functions
 *
 * Copyright (c) 2004-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifndef _LMMS_MATH_H
#define _LMMS_MATH_H


#ifdef __INTEL_COMPILER

#include <math.h>

static inline float absFraction( const float _x )
{
	return( _x - ( _x >= 0.0f ? floorf( _x ) : floorf( _x ) - 1 ) );
}

static inline float fraction( const float _x )
{
	return( _x - floorf( _x ) );
}

#else

static inline float absFraction( const float _x )
{
	return( _x - ( _x >= 0.0f ? static_cast<int>( _x ) :
						static_cast<int>( _x ) - 1 ) );
}

static inline float fraction( const float _x )
{
	return( _x - static_cast<int>( _x ) );
}


#if 0
// SSE3-version
static inline float absFraction( float _x )
{
	unsigned int tmp;
	asm(
		"fld %%st\n\t"
		"fisttp %1\n\t"
		"fild %1\n\t"
		"ftst\n\t"
		"sahf\n\t"
		"jae 1f\n\t"
		"fld1\n\t"
		"fsubrp %%st, %%st(1)\n\t"
	"1:\n\t"
		"fsubrp %%st, %%st(1)"
		: "+t"( _x ), "=m"( tmp )
		:
		: "st(1)", "cc" );
	return( _x );
}

static inline float absFraction( float _x )
{
	unsigned int tmp;
	asm(
		"fld %%st\n\t"
		"fisttp %1\n\t"
		"fild %1\n\t"
		"fsubrp %%st, %%st(1)"
		: "+t"( _x ), "=m"( tmp )
		:
		: "st(1)" );
	return( _x );
}
#endif

#endif



#define FAST_RAND_MAX 32767
static inline int fast_rand()
{
	static unsigned long next = 1;
	next = next * 1103515245 + 12345;
	return( (unsigned)( next / 65536 ) % 32768 );
}




#endif
