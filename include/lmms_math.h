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
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <numbers>
#include <concepts>

#include "lmms_constants.h"

#ifdef __SSE2__
#include <emmintrin.h>
#endif

namespace lmms
{


// TODO C++23: Make constexpr since std::abs() will be constexpr
inline bool approximatelyEqual(float x, float y) noexcept
{
	return x == y || std::abs(x - y) < F_EPSILON;
}

// TODO C++23: Make constexpr since std::trunc() will be constexpr
/*!
 * @brief Returns the fractional part of a float, a value between -1.0f and 1.0f.
 *
 * fraction( 2.3) =>  0.3
 * fraction(-2.3) => -0.3
 *
 * Note that if the return value is used as a phase of an oscillator, that the oscillator must support
 * negative phases.
 */
inline auto fraction(std::floating_point auto x) noexcept
{
	return x - std::trunc(x);
}


// TODO C++23: Make constexpr since std::floor() will be constexpr
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
inline auto absFraction(std::floating_point auto x) noexcept
{
	return x - std::floor(x);
}


//! @brief Returns a pseudorandom integer within [0, 32768).
//! @returns A pseudorandom integer greater than or equal to 0 and less than 32767.
inline int fastRand() noexcept
{
	thread_local unsigned long s_next = 1;
	s_next = s_next * 1103515245 + 12345;
	return s_next / 65536 % 32768;
}


//! @brief Returns a pseudorandom number within [0, @p upper].
//! @returns A pseudorandom number greater than or equal to 0 and less than or equal to @p upper.
template<std::floating_point T>
inline T fastRand(T upper) noexcept
{
	constexpr auto FAST_RAND_RATIO = static_cast<T>(1.0 / 32767);
	return fastRand() * upper * FAST_RAND_RATIO;
}


//! @brief Returns a pseudorandom integer within [0, @p upper].
//! @returns A pseudorandom integer greater than or equal to 0 and less than or equal to @p upper.
template<std::integral T>
inline T fastRand(T upper) noexcept
{
	// The integer specialization of this function is kind of weird, but it is
	// necessary to prevent massive bias away from the maximum value.
	// FAST_RAND_RATIO here is 1 greater than normal, so when multiplied by, it
	// will result in a random float within [0, @p upper) instead of the usual
	// [0, @p upper].
	constexpr float FAST_RAND_RATIO = 1.f / 32768;
	// Since the random float will be in a range that does not include @upper
	// due to the above ratio, increase the magnitude of the random float by 1.
	// All values greater than @p upper get rounded down to @p range, making the
	// chance of returning a value of @p upper the same as any other of the
	// possible values.
	// HACK: Even on -O3, without this static_cast, it will convert @p upper to float twice for some reason
	const float fupper = static_cast<float>(upper);
	const float r = fupper + std::copysign(1.f, fupper);
	// Always round towards 0 (implicit truncation occurs during static_cast).
	return static_cast<T>(fastRand() * r * FAST_RAND_RATIO);
}


//! @brief Returns a pseudorandom integer within [@p from, @p to].
//! This function does not require the parameters to be in the proper order.
//! fastRand(a, b) behaves identically to fastRand(b, a).
//! @returns A pseudorandom integer greater than or equal to @p from and less than or equal to @p to.
template<typename T> requires std::is_arithmetic_v<T>
inline auto fastRand(T from, T to) noexcept { return from + fastRand(to - from); }


//! @brief Returns true one in @p chance times at random.
inline bool oneIn(unsigned chance) noexcept { return 0 == (fastRand() % chance); }


//! Round `value` to `where` depending on step size
template<class T>
static void roundAt(T& value, const T& where, const T& stepSize)
{
	if (std::abs(value - where) < F_EPSILON * std::abs(stepSize))
	{
		value = where;
	}
}

//! Source: http://martin.ankerl.com/2007/10/04/optimized-pow-approximation-for-java-and-c-c/
inline double fastPow(double a, double b)
{
	double d;
	std::int32_t x[2];

	std::memcpy(x, &a, sizeof(x));
	x[1] = static_cast<std::int32_t>(b * (x[1] - 1072632447) + 1072632447);
	x[0] = 0;

	std::memcpy(&d, x, sizeof(d));
	return d;
}


//! returns +1 if val >= 0, else -1
template<typename T>
constexpr T sign(T val) noexcept
{ 
	return val >= 0 ? 1 : -1; 
}


//! if val >= 0.0f, returns sqrt(val), else: -sqrt(-val)
inline float sqrt_neg(float val) 
{
	return std::sqrt(std::abs(val)) * sign(val);
}

//! @brief Exponential function that deals with negative bases
inline float signedPowf(float v, float e)
{
	return std::pow(std::abs(v), e) * sign(v);
}


//! @brief Scales @value from linear to logarithmic.
//! Value should be within [0,1]
inline float logToLinearScale(float min, float max, float value)
{
	using namespace std::numbers;
	if (min < 0)
	{
		const float mmax = std::max(std::abs(min), std::abs(max));
		const float val = value * (max - min) + min;
		float result = signedPowf(val / mmax, e_v<float>) * mmax;
		return std::isnan(result) ? 0 : result;
	}
	float result = std::pow(value, e_v<float>) * (max - min) + min;
	return std::isnan(result) ? 0 : result;
}


//! @brief Scales value from logarithmic to linear. Value should be in min-max range.
inline float linearToLogScale(float min, float max, float value)
{
	constexpr auto inv_e = static_cast<float>(1.0 / std::numbers::e);
	const float valueLimited = std::clamp(value, min, max);
	const float val = (valueLimited - min) / (max - min);
	if (min < 0)
	{
		const float mmax = std::max(std::abs(min), std::abs(max));
		float result = signedPowf(valueLimited / mmax, inv_e) * mmax;
		return std::isnan(result) ? 0 : result;
	}
	float result = std::pow(val, inv_e) * (max - min) + min;
	return std::isnan(result) ? 0 : result;
}


// TODO C++26: Make constexpr since std::exp() will be constexpr
template<typename T> requires std::is_arithmetic_v<T>
inline auto fastPow10f(T x)
{
	using F_T = std::conditional_t<std::is_floating_point_v<T>, T, float>;
	return std::exp(std::numbers::ln10_v<F_T> * x);
}


// TODO C++26: Make constexpr since std::log() will be constexpr
template<typename T> requires std::is_arithmetic_v<T>
inline auto fastLog10f(T x)
{
	using F_T = std::conditional_t<std::is_floating_point_v<T>, T, float>;
	constexpr auto inv_ln10 = static_cast<F_T>(1.0 / std::numbers::ln10);
	return std::log(x) * inv_ln10;
}


//! @brief Converts linear amplitude (>0-1.0) to dBFS scale. 
//! @param amp Linear amplitude, where 1.0 = 0dBFS. ** Must be larger than zero! **
//! @return Amplitude in dBFS. 
inline float ampToDbfs(float amp)
{
	return fastLog10f(amp) * 20.0f;
}


//! @brief Converts dBFS-scale to linear amplitude with 0dBFS = 1.0
//! @param dbfs The dBFS value to convert. ** Must be a real number - not inf/nan! **
//! @return Linear amplitude
inline float dbfsToAmp(float dbfs)
{
	return fastPow10f(dbfs * 0.05f);
}


//! @brief Converts linear amplitude (0-1.0) to dBFS scale. Handles zeroes as -inf.
//! @param amp Linear amplitude, where 1.0 = 0dBFS. 
//! @return Amplitude in dBFS. -inf for 0 amplitude.
inline float safeAmpToDbfs(float amp)
{
	return amp == 0.0f ? -INFINITY : ampToDbfs(amp);
}


//! @brief Converts dBFS-scale to linear amplitude with 0dBFS = 1.0. Handles infinity as zero.
//! @param dbfs The dBFS value to convert: all infinites are treated as -inf and result in 0
//! @return Linear amplitude
inline float safeDbfsToAmp(float dbfs)
{
	return std::isinf(dbfs) ? 0.0f : dbfsToAmp(dbfs);
}


// TODO C++20: use std::formatted_size
// @brief Calculate number of digits which LcdSpinBox would show for a given number
inline int numDigitsAsInt(float f)
{
	// use rounding:
	// LcdSpinBox sometimes uses std::round(), sometimes cast rounding
	// we use rounding to be on the "safe side"
	int asInt = static_cast<int>(std::round(f));
	int digits = 1; // always at least 1
	if(asInt < 0)
	{
		++digits;
		asInt = -asInt;
	}
	// "asInt" is positive from now
	int power = 1;
	for (int i = 1; i < 10; ++i)
	{
		power *= 10;
		if (asInt >= power) { ++digits; } // 2 digits for >=10, 3 for >=100
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

#ifdef __SSE2__
// exp approximation for SSE2: https://stackoverflow.com/a/47025627/5759631
// Maximum relative error of 1.72863156e-3 on [-87.33654, 88.72283]
inline __m128 fastExp(__m128 x)
{
	__m128 f, p, r;
	__m128i t, j;
	const __m128 a = _mm_set1_ps (12102203.0f); /* (1 << 23) / log(2) */
	const __m128i m = _mm_set1_epi32 (0xff800000); /* mask for integer bits */
	const __m128 ttm23 = _mm_set1_ps (1.1920929e-7f); /* exp2(-23) */
	const __m128 c0 = _mm_set1_ps (0.3371894346f);
	const __m128 c1 = _mm_set1_ps (0.657636276f);
	const __m128 c2 = _mm_set1_ps (1.00172476f);

	t = _mm_cvtps_epi32 (_mm_mul_ps (a, x));
	j = _mm_and_si128 (t, m);            /* j = (int)(floor (x/log(2))) << 23 */
	t = _mm_sub_epi32 (t, j);
	f = _mm_mul_ps (ttm23, _mm_cvtepi32_ps (t)); /* f = (x/log(2)) - floor (x/log(2)) */
	p = c0;                              /* c0 */
	p = _mm_mul_ps (p, f);               /* c0 * f */
	p = _mm_add_ps (p, c1);              /* c0 * f + c1 */
	p = _mm_mul_ps (p, f);               /* (c0 * f + c1) * f */
	p = _mm_add_ps (p, c2);              /* p = (c0 * f + c1) * f + c2 ~= 2^f */
	r = _mm_castsi128_ps (_mm_add_epi32 (j, _mm_castps_si128 (p))); /* r = p * 2^i*/
	return r;
}

// Lost Robot's SSE2 adaptation of Kari's vectorized log approximation: https://stackoverflow.com/a/65537754/5759631
// Maximum relative error of 7.922410e-4 on [1.0279774e-38f, 3.4028235e+38f]
inline __m128 fastLog(__m128 a)
{
	__m128i aInt = _mm_castps_si128(a);
	__m128i e = _mm_sub_epi32(aInt, _mm_set1_epi32(0x3f2aaaab));
	e = _mm_and_si128(e, _mm_set1_epi32(0xff800000));
	__m128i subtr = _mm_sub_epi32(aInt, e);
	__m128 m = _mm_castsi128_ps(subtr);
	__m128 i = _mm_mul_ps(_mm_cvtepi32_ps(e), _mm_set1_ps(1.19209290e-7f));
	__m128 f = _mm_sub_ps(m, _mm_set1_ps(1.0f));
	__m128 s = _mm_mul_ps(f, f);
	__m128 r = _mm_add_ps(_mm_mul_ps(_mm_set1_ps(0.230836749f), f), _mm_set1_ps(-0.279208571f));
	__m128 t = _mm_add_ps(_mm_mul_ps(_mm_set1_ps(0.331826031f), f), _mm_set1_ps(-0.498910338f));
	r = _mm_add_ps(_mm_mul_ps(r, s), t);
	r = _mm_add_ps(_mm_mul_ps(r, s), f);
	r = _mm_add_ps(_mm_mul_ps(i, _mm_set1_ps(0.693147182f)), r);
	return r;
}

inline __m128 sse2Abs(__m128 x)
{
	return _mm_and_ps(x, _mm_castsi128_ps(_mm_set1_epi32(0x7fffffff)));// clear sign bit
}

inline __m128 sse2Floor(__m128 x)
{
	__m128 t = _mm_cvtepi32_ps(_mm_cvttps_epi32(x)); // trunc toward 0
	__m128 needs_correction = _mm_cmplt_ps(x, t); // checks if x < trunc
	return _mm_sub_ps(t, _mm_and_ps(needs_correction, _mm_set1_ps(1.0f)));
}

inline __m128 sse2Round(__m128 x)
{
	__m128 sign_mask = _mm_cmplt_ps(x, _mm_setzero_ps());// checks if x < 0
	__m128 bias_pos = _mm_set1_ps(0.5f);
	__m128 bias_neg = _mm_set1_ps(-0.5f);
	__m128 bias = _mm_or_ps(_mm_and_ps(sign_mask, bias_neg), _mm_andnot_ps(sign_mask, bias_pos));
	__m128 y = _mm_add_ps(x, bias);
	return _mm_cvtepi32_ps(_mm_cvttps_epi32(y));
}

#endif // __SSE2__

} // namespace lmms

#endif // LMMS_MATH_H
