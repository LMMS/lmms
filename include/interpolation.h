/*
 * interpolation.h - fast implementations of several interpolation-algorithms
 *
 * Copyright (c) 2004-2005 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifndef INTERPOLATION_H
#define INTERPOLATION_H

#ifndef __USE_XOPEN
#define __USE_XOPEN
#endif

#include <math.h>
#include "lmms_constants.h"
#include "lmms_math.h"

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
	const float f = ( 1.0f - cosf( x * F_PI ) ) * 0.5f;
	return fastFmaf( f, v1-v0, v0 );
}


inline float linearInterpolate( float v0, float v1, float x )
{
	return fastFmaf( x, v1-v0, v0 );
}


inline float optimalInterpolate( float v0, float v1, float x )
{
	const float z = x - 0.5f;
	const float even = v1 + v0;
	const float odd = v1 - v0;
	
	const float c0 = even * 0.50037842517188658;
	const float c1 = odd * 1.00621089801788210;
	const float c2 = even * -0.004541102062639801;
	const float c3 = odd * -1.57015627178718420;
	
	return ( ( c3*z + c2 ) * z + c1 ) * z + c0;
}


inline float optimal4pInterpolate( float v0, float v1, float v2, float v3, float x )
{
	const float z = x - 0.5f;
	const float even1 = v2 + v1;
	const float odd1 = v2 - v1;
	const float even2 = v3 + v0; 
	const float odd2 = v3 - v0;

	const float c0 = even1 * 0.45868970870461956 + even2 * 0.04131401926395584;
	const float c1 = odd1 * 0.48068024766578432 + odd2 * 0.17577925564495955;
	const float c2 = even1 * -0.246185007019907091 + even2 * 0.24614027139700284;
	const float c3 = odd1 * -0.36030925263849456 + odd2 * 0.10174985775982505;

	return ( ( c3*z + c2 ) * z + c1 ) * z + c0;
}



inline float lagrangeInterpolate( float v0, float v1, float v2, float v3, float x )
{
	const float c0 = v1;
	const float c1 = v2 - v0 * ( 1.0f / 3.0f ) - v1 * 0.5f - v3 * ( 1.0f / 6.0f );
	const float c2 = 0.5f * (v0 + v2) - v1;
	const float c3 = ( 1.0f/6.0f ) * ( v3 - v0 ) + 0.5f * ( v1 - v2 );
	return ( ( c3*x + c2 ) * x + c1 ) * x + c0;
}





#endif
