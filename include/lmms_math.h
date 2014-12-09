/*
 * lmms_math.h - defines math functions
 *
 * Copyright (c) 2004-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifndef LMMS_MATH_H
#define LMMS_MATH_H

#include <stdint.h>
#include "lmms_constants.h"
#include "lmmsconfig.h"
#include <QtCore/QtGlobal>

#include <cmath>
using namespace std;

#if defined (LMMS_BUILD_WIN32) || defined (LMMS_BUILD_APPLE) 
#ifndef isnanf
#define isnanf(x)	isnan(x)
#endif
#ifndef isinff
#define isinff(x)	isinf(x)
#endif
#ifndef _isnanf
#define _isnanf(x) isnan(x)
#endif
#ifndef _isinff
#define _isinff(x) isinf(x)
#endif
#ifndef exp10
#define exp10(x) pow( 10, x )
#endif
#ifndef exp10f
#define exp10f(x) powf( 10, x )
#endif
#endif

#ifdef __INTEL_COMPILER

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

//! @brief Takes advantage of fmal() function if present in hardware
static inline long double fastFmal( long double a, long double b, long double c ) {
#ifdef FP_FAST_FMAL
	#ifdef __clang__
		return fma( a, b, c );
	#else
		return fmal( a, b, c );
	#endif
#else
	return a * b + c;
#endif
}

//! @brief Takes advantage of fmaf() function if present in hardware
static inline float fastFmaf( float a, float b, float c ) {
#ifdef FP_FAST_FMAF
	#ifdef __clang__
		return fma( a, b, c );
	#else
		return fmaf( a, b, c );
	#endif
#else
	return a * b + c;
#endif
}

//! @brief Takes advantage of fma() function if present in hardware
static inline double fastFma( double a, double b, double c ) {
#ifdef FP_FAST_FMA
	return fma( a, b, c );
#else
	return a * b + c;
#endif
}

// source: http://martin.ankerl.com/2007/10/04/optimized-pow-approximation-for-java-and-c-c/
static inline double fastPow( double a, double b )
{
	union
	{
		double d;
		int32_t x[2];
	} u = { a };
	u.x[1] = static_cast<int32_t>( b * ( u.x[1] - 1072632447 ) + 1072632447 );
	u.x[0] = 0;
	return u.d;
}

// sinc function
static inline double sinc( double _x )
{
	return _x == 0.0 ? 1.0 : sin( F_PI * _x ) / ( F_PI * _x );
}


//! @brief Exponential function that deals with negative bases
static inline float signedPowf( float v, float e )
{
	return v < 0 
		? powf( -v, e ) * -1.0f
		: powf( v, e );
}


//! @brief Scales @value from linear to logarithmic.
//! Value should be within [0,1]
static inline float logToLinearScale( float min, float max, float value )
{
	if( min < 0 )
	{
		const float mmax = qMax( qAbs( min ), qAbs( max ) );
		const float val = value * ( max - min ) + min;
		return signedPowf( val / mmax, F_E ) * mmax;
	}
	return powf( value, F_E ) * ( max - min ) + min;
}


//! @brief Scales value from logarithmic to linear. Value should be in min-max range.
static inline float linearToLogScale( float min, float max, float value )
{
	static const float EXP = 1.0f / F_E;
	const float val = ( value - min ) / ( max - min );
	if( min < 0 )
	{
		const float mmax = qMax( qAbs( min ), qAbs( max ) );
		return signedPowf( value / mmax, EXP ) * mmax;
	}
	return powf( val, EXP ) * ( max - min ) + min;
}




//! @brief Converts linear amplitude (0-1.0) to dBV scale. 
//! @param amp Linear amplitude, where 1.0 = 0dBV. 
//! @return Amplitude in dBV. -inf for 0 amplitude.
static inline float ampToDbv( float amp )
{
	return amp == 0.0f
		? -INFINITY
		: log10f( amp ) * 20.0f;
}


//! @brief Converts dBV-scale to linear amplitude with 0dBV = 1.0
//! @param dbv The dBV value to convert: all infinites are treated as -inf and result in 0
//! @return Linear amplitude
static inline float dbvToAmp( float dbv )
{
	return isinff( dbv )
		? 0.0f
		: exp10f( dbv * 0.05f );
}


#endif
