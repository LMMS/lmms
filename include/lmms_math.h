/*
 * lmms_math.h - defines math functions
 *
 * Copyright (c) 2004-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef LMMS_MATH_H
#define LMMS_MATH_H

#include <QtGlobal>
#include <algorithm>
#include <cmath>
#include <cstdint>

#include "lmms_constants.h"
#include "lmmsconfig.h"
#include <cassert>

namespace lmms
{

#ifdef __INTEL_COMPILER

static inline float absFraction( const float _x )
{
	return( _x - floorf( _x ) );
}

static inline float fraction( const float _x )
{
	return( _x - floorf( _x ) - ( _x >= 0.0f ? 0.0 : 1.0 ) );
}

#else

/*!
 * @brief Returns the wrapped fractional part of a float, a value between 0.0f and 1.0f.
 *
 * absFraction( 2.3) =>  0.3
 * absFraction(-2.3) =>  0.7
 *
 * Note that this not the same as the absolute value of the fraction (as the function name suggests).
 * If the result is interpreted as a phase of an oscillator, it makes that negative phases are
 * converted to positive phases.
 */
static inline float absFraction(const float x)
{
	return x - std::floor(x);
}

/*!
 * @brief Returns the fractional part of a float, a value between -1.0f and 1.0f.
 *
 * fraction( 2.3) =>  0.3
 * fraction(-2.3) => -0.3
 *
 * Note that if the return value is used as a phase of an oscillator, that the oscillator must support
 * negative phases.
 */
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

#endif // __INTEL_COMPILER



constexpr int FAST_RAND_MAX = 32767;
static inline int fast_rand()
{
	static unsigned long next = 1;
	next = next * 1103515245 + 12345;
	return( (unsigned)( next / 65536 ) % 32768 );
}

static inline double fastRand( double range )
{
	static const double fast_rand_ratio = 1.0 / FAST_RAND_MAX;
	return fast_rand() * range * fast_rand_ratio;
}

static inline float fastRandf( float range )
{
	static const float fast_rand_ratio = 1.0f / FAST_RAND_MAX;
	return fast_rand() * range * fast_rand_ratio;
}

//! @brief Takes advantage of fmal() function if present in hardware
static inline long double fastFmal( long double a, long double b, long double c ) 
{
#ifdef FP_FAST_FMAL
	#ifdef __clang__
		return fma( a, b, c );
	#else
		return fmal( a, b, c );
	#endif
#else
	return a * b + c;
#endif // FP_FAST_FMAL
}

//! @brief Takes advantage of fmaf() function if present in hardware
static inline float fastFmaf( float a, float b, float c ) 
{
#ifdef FP_FAST_FMAF
	#ifdef __clang__
		return fma( a, b, c );
	#else
		return fmaf( a, b, c );
	#endif
#else
	return a * b + c;
#endif // FP_FAST_FMAF
}

//! @brief Takes advantage of fma() function if present in hardware
static inline double fastFma( double a, double b, double c ) 
{
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
		const float mmax = std::max(std::abs(min), std::abs(max));
		const float val = value * ( max - min ) + min;
		float result = signedPowf( val / mmax, F_E ) * mmax;
		return std::isnan( result ) ? 0 : result;
	}
	float result = powf( value, F_E ) * ( max - min ) + min;
	return std::isnan( result ) ? 0 : result;
}


//! @brief Scales value from logarithmic to linear. Value should be in min-max range.
static inline float linearToLogScale( float min, float max, float value )
{
	static const float EXP = 1.0f / F_E;
	const float valueLimited = std::clamp(value, min, max);
	const float val = ( valueLimited - min ) / ( max - min );
	if( min < 0 )
	{
		const float mmax = std::max(std::abs(min), std::abs(max));
		float result = signedPowf( valueLimited / mmax, EXP ) * mmax;
		return std::isnan( result ) ? 0 : result;
	}
	float result = powf( val, EXP ) * ( max - min ) + min;
	return std::isnan( result ) ? 0 : result;
}




//! @brief Converts linear amplitude (0-1.0) to dBFS scale. Handles zeroes as -inf.
//! @param amp Linear amplitude, where 1.0 = 0dBFS. 
//! @return Amplitude in dBFS. -inf for 0 amplitude.
static inline float safeAmpToDbfs( float amp )
{
	return amp == 0.0f
		? -INFINITY
		: log10f( amp ) * 20.0f;
}


//! @brief Converts dBFS-scale to linear amplitude with 0dBFS = 1.0. Handles infinity as zero.
//! @param dbfs The dBFS value to convert: all infinites are treated as -inf and result in 0
//! @return Linear amplitude
static inline float safeDbfsToAmp( float dbfs )
{
	return std::isinf( dbfs )
		? 0.0f
		: std::pow(10.f, dbfs * 0.05f );
}


//! @brief Converts linear amplitude (>0-1.0) to dBFS scale. 
//! @param amp Linear amplitude, where 1.0 = 0dBFS. ** Must be larger than zero! **
//! @return Amplitude in dBFS. 
static inline float ampToDbfs(float amp)
{
	return log10f(amp) * 20.0f;
}


//! @brief Converts dBFS-scale to linear amplitude with 0dBFS = 1.0
//! @param dbfs The dBFS value to convert. ** Must be a real number - not inf/nan! **
//! @return Linear amplitude
static inline float dbfsToAmp(float dbfs)
{
	return std::pow(10.f, dbfs * 0.05f);
}



//! returns 1.0f if val >= 0.0f, -1.0 else
static inline float sign( float val ) 
{ 
	return val >= 0.0f ? 1.0f : -1.0f; 
}


//! if val >= 0.0f, returns sqrtf(val), else: -sqrtf(-val)
static inline float sqrt_neg( float val ) 
{
	return sqrtf( fabs( val ) ) * sign( val );
}


// fast approximation of square root
static inline float fastSqrt( float n )
{
	union 
	{
		int32_t i;
		float f;
	} u;
	u.f = n;
	u.i = ( u.i + ( 127 << 23 ) ) >> 1;
	return u.f;
}

//! returns value furthest from zero
template<class T>
static inline T absMax( T a, T b )
{
	return std::abs(a) > std::abs(b) ? a : b;
}

//! returns value nearest to zero
template<class T>
static inline T absMin( T a, T b )
{
	return std::abs(a) < std::abs(b) ? a : b;
}

//! Returns the linear interpolation of the two values
template<class T, class F>
constexpr T lerp(T a, T b, F t)
{
	return (1. - t) * a + t * b;
}

// @brief Calculate number of digits which LcdSpinBox would show for a given number
// @note Once we upgrade to C++20, we could probably use std::formatted_size
static inline int numDigitsAsInt(float f)
{
	// use rounding:
	// LcdSpinBox sometimes uses roundf(), sometimes cast rounding
	// we use rounding to be on the "safe side"
	const float rounded = roundf(f);
	int asInt = static_cast<int>(rounded);
	int digits = 1; // always at least 1
	if(asInt < 0)
	{
		++digits;
		asInt = -asInt;
	}
	// "asInt" is positive from now
	int32_t power = 1;
	for(int32_t i = 1; i<10; ++i)
	{
		power *= 10;
		if(static_cast<int32_t>(asInt) >= power) { ++digits; } // 2 digits for >=10, 3 for >=100
		else { break; }
	}
	return digits;
}

template <typename T>
class LinearMap
{
public:
	LinearMap(T x1, T y1, T x2, T y2)
	{
		T const dx = x2 - x1;
		assert (dx != T(0));

		m_a = (y2 - y1) / dx;
		m_b = y1 - m_a * x1;
	}

	T map(T x) const
	{
		return m_a * x + m_b;
	}

private:
	T m_a;
	T m_b;
};

} // namespace lmms

#endif // LMMS_MATH_H
