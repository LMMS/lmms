/*
 * templates.h - miscellanous templates and algorithms
 *
 * Linux MultiMedia Studio
 * Copyright (c) 2004-2005 Tobias Doerffel <tobydox@users.sourceforge.net>
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


#ifndef _TEMPLATES_H
#define _TEMPLATES_H

#include "qt3support.h"

#ifdef QT4

#include <QtAlgorithms>

#else

#include <qtl.h>

#endif


template<class T>
inline T tAbs( const T & x )
{
	return( x < static_cast<T>( 0 ) ? -x : x );
}




template<class T>
inline T tMin( const T & x1, const T & x2 )
{
	if( x1 < x2 )
	{
		return( x1 );
	}
	return( x2 );
}




template<class T>
inline T tMax( const T & x1, const T & x2 )
{
	if( x1 > x2 )
	{
		return( x1 );
	}
	return( x2 );
}




template<class T>
inline T tLimit( const T & x, const T & x1, const T & x2 )
{
	return( tMin<T>( tMax<T>( x, tMin<T>( x1, x2 ) ),
							tMax<T>( x1, x2 ) ) );
	
}


#endif
