/*
 * interpolation.h - fast implementations of several interpolation-algorithms
 *
 * Copyright (c) 2004-2005 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifndef _INTERPOLATION_H
#define _INTERPOLATION_H

#ifndef __USE_XOPEN
#define __USE_XOPEN
#endif

#include <math.h>
#include "lmms_constants.h"

inline float hermiteInterpolate( float x0, float x1, float x2, float x3,
								float frac_pos )
{
	const float frsq = frac_pos*frac_pos;
	const float frsq2 = 2*frsq;
	return( ( (x2-x0) *0.5f ) * ( frac_pos * (frsq+1) -frsq2 ) +
				( frsq2*frac_pos - 3*frsq ) * ( x1-x2 ) +
			frsq2 * (frac_pos-1) * ( ( x3-x1 ) * 0.25f ) + x1 );

/*
   const float frsq	= frac_pos*frac_pos;
   //const float frsq2	= 2*frsq;
   frac_pos *= 0.5;
   const float frcu	= frsq*frac_pos;
   return (
   
   (frcu - frsq + frac_pos) * ((x2 - x0)) +
   
   (4*frcu - 3*frsq) * (x1 - x2)
   //frsq*(2*frac_pos-3) * (x1 - x2)
   
   + (frcu - 0.5*frsq)*((x3 - x1))  
    
   + x1
   
   );
*/
}



inline float cubicInterpolate( float v0, float v1, float v2, float v3, float x )
{
	float frsq = x*x;
	float frcu = frsq*v0;
	float t1 = v3 + 3*v1;

	return( v1 + 0.5f * frcu + x * ( v2 - frcu * ( 1.0f/6.0f ) -
		t1 * ( 1.0f/6.0f ) - v0 / 3.0f ) + frsq * x * ( t1 *
		( 1.0f/6.0f ) - 0.5f * v2 ) + frsq * ( 0.5f * v2 - v1 ) );
}



inline float cosinusInterpolate( float v0, float v1, float x )
{
	float f = cosf( x * ( F_PI_2 ) );
	return( v0*f + v1*( 1.0f-f ) );
}



inline float linearInterpolate( float v0, float v1, float x )
{
	return( v0*( 1.0f-x ) + v1*x );
}


#endif
