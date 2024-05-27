/*
 * interpolation.h - fast implementations of several interpolation-algorithms
 *
 * Copyright (c) 2004-2005 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef LMMS_INTERPOLATION_H
#define LMMS_INTERPOLATION_H

#ifndef __USE_XOPEN
#define __USE_XOPEN
#endif

#include <cmath>
#include "lmms_constants.h"
#include "lmms_math.h"

namespace lmms
{

inline float hermiteInterpolate(float v0, float v1, float v2, float v3, float x)
{
	float c0 = v1;
	float c1 = 1 / 2.0 * (v2 - v0);
	float c2 = v0 - 5 / 2.0 * v1 + 2 * v2 - 1 / 2.0 * v3;
	float c3 = 1 / 2.0 * (v3 - v0) + 3 / 2.0 * (v1 - v2);
	return fastFmaf(fastFmaf(fastFmaf(c3, x, c2), x, c1), x, c0);
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
	
	return fastFmaf( fastFmaf( fastFmaf( c3, z, c2 ), z, c1 ), z, c0 );
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

	return fastFmaf( fastFmaf( fastFmaf( c3, z, c2 ), z, c1 ), z, c0 );
}



inline float lagrangeInterpolate( float v0, float v1, float v2, float v3, float x )
{
	const float c0 = v1;
	const float c1 = v2 - v0 * ( 1.0f / 3.0f ) - v1 * 0.5f - v3 * ( 1.0f / 6.0f );
	const float c2 = 0.5f * (v0 + v2) - v1;
	const float c3 = ( 1.0f/6.0f ) * ( v3 - v0 ) + 0.5f * ( v1 - v2 );
	return fastFmaf( fastFmaf( fastFmaf( c3, x, c2 ), x, c1 ), x, c0 );
}



} // namespace lmms

#endif // LMMS_INTERPOLATION_H
