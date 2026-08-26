/*
 * SimdHelpers.h - for cross-platform SIMD
 *
 * Copyright (c) 2026 Dalton Messmer <messmer.dalton/at/gmail.com>
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

#ifndef LMMS_SIMD_HELPERS_H
#define LMMS_SIMD_HELPERS_H

#include <stdexcept>
#include <type_traits>

#include "FeatureDetection.h"

#if defined(LMMS_HOST_X86_64)
#	include <immintrin.h>
#	if defined(__GNUC__) || defined(__clang__)
#		define LMMS_FUNC_TARGET_SCALAR  __attribute__((target("default")))
#		define LMMS_FUNC_TARGET_SSE2    __attribute__((target("sse2")))
#		define LMMS_FUNC_TARGET_SSE4_2  __attribute__((target("sse4.2")))
#		define LMMS_FUNC_TARGET_AVX     __attribute__((target("avx")))
#		define LMMS_FUNC_TARGET_AVX2    __attribute__((target("avx2")))
#		define LMMS_FUNC_TARGET_AVX512F __attribute__((target("avx512f")))
#	else
#		define LMMS_FUNC_TARGET_SCALAR
#		define LMMS_FUNC_TARGET_SSE2
#		define LMMS_FUNC_TARGET_SSE4_2
#		define LMMS_FUNC_TARGET_AVX
#		define LMMS_FUNC_TARGET_AVX2
#		define LMMS_FUNC_TARGET_AVX512F
#	endif
#elif defined(LMMS_HOST_ARM64)
#	include <arm_neon.h>
#	include <arm_sve.h>
#	if defined(__GNUC__) || defined(__clang__)
#		define LMMS_FUNC_TARGET_SCALAR  __attribute__((target("default")))
#		define LMMS_FUNC_TARGET_NEON    __attribute__((target("+simd")))
#		define LMMS_FUNC_TARGET_SVE     __attribute__((target("+sve")))
#		define LMMS_FUNC_TARGET_SVE2    __attribute__((target("+sve2")))
#	else
#		define LMMS_FUNC_TARGET_SCALAR
#		define LMMS_FUNC_TARGET_NEON
#		define LMMS_FUNC_TARGET_SVE
#		define LMMS_FUNC_TARGET_SVE2
#	endif
#endif

#if defined(__GNUC__) || defined(__clang__)
#	define LMMS_SIMD_BEGIN_IMPL \
		_Pragma("GCC diagnostic push") \
		_Pragma("GCC diagnostic ignored \"-Wpsabi\"")
#	define LMMS_SIMD_END_IMPL \
		_Pragma("GCC diagnostic pop")
#else
#	define LMMS_SIMD_BEGIN_IMPL
#	define LMMS_SIMD_END_IMPL
#endif

namespace lmms {

#ifndef _MSC_VER
#	define LMMS_DEFINE_SIMD_GENERIC(name, fn128, fn256, fn512) \
		template<std::uint8_t lanes> inline constexpr const auto& name = 0; \
		template<> inline constexpr const auto& name<4>  = fn128; \
		template<> inline constexpr const auto& name<8>  = fn256; \
		template<> inline constexpr const auto& name<16> = fn512;
#else
#	define LMMS_DEFINE_SIMD_GENERIC(name, fn128, fn256, fn512)  \
		template<std::uint8_t lanes, class... Args>             \
		LMMS_INLINE auto name(Args... args) -> decltype(auto)   \
		{                                                       \
		    if constexpr (lanes == 4) {                         \
		        return (fn128)(args...);                        \
		    } else if constexpr (lanes == 8) {                  \
		        return (fn256)(args...);                        \
		    } else if constexpr (lanes == 16) {                 \
		        return (fn512)(args...);                        \
		    } else { static_assert(lanes == 4); }               \
		}
#endif

#if defined(LMMS_HOST_X86_64)

// Common SIMD intrinics made generic on the number of lanes

LMMS_DEFINE_SIMD_GENERIC(_mmX_loadu_ps, _mm_loadu_ps, _mm256_loadu_ps, _mm512_loadu_ps)
LMMS_DEFINE_SIMD_GENERIC(_mmX_storeu_ps, _mm_storeu_ps, _mm256_storeu_ps, _mm512_storeu_ps)
LMMS_DEFINE_SIMD_GENERIC(_mmX_add_ps, _mm_add_ps, _mm256_add_ps, _mm512_add_ps)

#endif // LMMS_HOST_X86_64

//! A collection of different SIMD-enabled functions for runtime dispatch
template<bool ne, class R, class... A>
struct SimdDispatchTargets
{
	using Ret = R;
	using Func = R(*)(A...) noexcept(ne);
	using FuncRef = R(&)(A...) noexcept(ne);
	static constexpr bool NoExcept = ne;

#if defined(LMMS_HOST_X86_64)
	Func avx512f = nullptr;
	Func avx2    = nullptr;
	Func avx     = nullptr;
	Func sse4_2  = nullptr;
	Func sse2    = nullptr;
#elif defined(LMMS_HOST_ARM64)
	Func sve2    = nullptr;
	Func sve     = nullptr;
	Func neon    = nullptr;
#endif

	//! Non-SIMD fallback
	Func scalar  = nullptr;
};

namespace detail {

template<class T>
struct IsSimdDispatchTargets { static constexpr bool value = false; };

template<bool ne, class R, class... A>
struct IsSimdDispatchTargets<SimdDispatchTargets<ne, R, A...>> { static constexpr bool value = true; };

} // namespace detail

//! A runtime dispatcher for SIMD functions
template<auto targets> // TODO C++26: Use a variable-template template-parameter
	requires (detail::IsSimdDispatchTargets<decltype(targets)>::value)
class SimdDispatcher
{
	using FuncRef = decltype(targets)::FuncRef;
	using Ret = decltype(targets)::Ret;
	FuncRef m_resolvedTarget;
public:
	SimdDispatcher() : m_resolvedTarget{resolve()} {}

	template<class... Args>
		requires (std::is_invocable_v<FuncRef, Args...>)
	LMMS_INLINE auto operator()(Args&&... args) const noexcept(FuncRef) -> Ret
	{
		if constexpr (std::is_void_v<Ret>)
		{
			m_resolvedTarget(std::forward<Args>(args)...);
			return;
		}
		else
		{
			return m_resolvedTarget(std::forward<Args>(args)...);
		}
	}

private:
	LMMS_INLINE auto resolve() -> FuncRef
	{
		constexpr auto highestDispatchTarget = [&]() -> std::uint32_t {
#if defined(LMMS_HOST_X86_64)
			if (targets.avx512f) { return LMMS_CPU_FEATURE_AVX512F; }
			if (targets.avx2)    { return LMMS_CPU_FEATURE_AVX2; }
			if (targets.avx)     { return LMMS_CPU_FEATURE_AVX; }
			if (targets.sse4_2)  { return LMMS_CPU_FEATURE_SSE4_2; }
			if (targets.sse2)    { return LMMS_CPU_FEATURE_X86_64_V1; }
#elif defined(LMMS_HOST_ARM64)
			if (targets.sve2)    { return LMMS_CPU_FEATURE_SVE2; }
			if (targets.sve)     { return LMMS_CPU_FEATURE_SVE; }
			if (targets.neon)    { return LMMS_CPU_FEATURE_NEON; }
#endif
			return LMMS_CPU_FEATURE_NONE;
		};

		switch (FeatureDetection::runtimeCpuFeatures() & highestDispatchTarget)
		{
#if defined(LMMS_HOST_X86_64)
			case LMMS_CPU_FEATURE_AVX512F:
				if (targets.avx512f) { return *targets.avx512f; }
				[[fallthrough]];
			case LMMS_CPU_FEATURE_AVX2:
				if (targets.avx2) { return *targets.avx2; }
				[[fallthrough]];
			case LMMS_CPU_FEATURE_AVX:
				if (targets.avx) { return *targets.avx; }
				[[fallthrough]];
			case LMMS_CPU_FEATURE_SSE4_2:
				if (targets.sse4_2) { return *targets.sse4_2; }
				[[fallthrough]];
			case LMMS_CPU_FEATURE_X86_64_V1:
				if (targets.sse2) { return *targets.sse2; }
				[[fallthrough]];
#elif defined(LMMS_HOST_ARM64)
			case LMMS_CPU_FEATURE_SVE2:
				if (targets.sve2) { return *targets.sve2; }
				[[fallthrough]];
			case LMMS_CPU_FEATURE_SVE:
				if (targets.sve) { return *targets.sve; }
				[[fallthrough]];
			case LMMS_CPU_FEATURE_NEON:
				if (targets.neon) { return *targets.neon; }
				[[fallthrough]];
#endif
			case LMMS_CPU_FEATURE_NONE:
				[[fallthrough]];
			default:
				if (targets.scalar) { return *targets.scalar; }
				break;
		}

		throw std::logic_error{"SIMD dispatcher could not resolve a target"};
	}
};

} // namespace lmms

#endif // LMMS_SIMD_HELPERS_H
