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

//! A collection of different SIMD-enabled functions for runtime dispatch + dispatch settings
template<bool ne, class R, class... A>
struct SimdDispatchConfig
{
	using Target = auto(*)(A...) noexcept(ne) -> R;
	using TargetRef = auto(&)(A...) noexcept(ne) -> R;

#if defined(LMMS_HOST_X86_64)
	Target avx512f = nullptr;
	Target avx2    = nullptr;
	Target avx     = nullptr;
	Target sse4_2  = nullptr;
	Target sse2    = nullptr;
#elif defined(LMMS_HOST_ARM64)
	Target sve2    = nullptr;
	Target sve     = nullptr;
	Target neon    = nullptr;
#endif

	//! Non-SIMD fallback (required)
	TargetRef scalar;

	//! If true, then when XYZ SIMD instructions are unconditionally available (via -march or /arch)
	//! assume the compiler does a better job auto-vectorizing the @a scalar target than
	//! the manually-written XYZ-or-lower SIMD targets. This is the "compiler knows best" setting.
	//! It's great for SIMD noobs or for benchmarking purposes to discover if you're a SIMD noob.
	bool preferAutoVectorization = false;

	//! If disabled, only the @a scalar target is used
	bool enableDynamicDispatch = true;
};

namespace detail {

//! This consteval class exists as a way to move all possible work in resolving the
//! dispatch target from runtime to compile-time
template<bool ne, class R, class... A>
class SimdDispatchResolver
{
public:
	SimdDispatchResolver() = delete;
	consteval SimdDispatchResolver(SimdDispatchConfig<ne, R, A...> config)
		: m_resolver{getResolver(config)}
		, m_config{config}
		, m_highestImplementedTarget{getHighestImplementedTarget(config)}
	{}

	using TargetRef = auto(&)(A...) noexcept(ne) -> R;
	LMMS_INLINE auto operator()() const noexcept -> TargetRef { return (this->*m_resolver)(); }

private:
	using ResolverFunc = auto(SimdDispatchResolver::*)() const noexcept -> TargetRef;
	consteval static auto getResolver(const SimdDispatchConfig<ne, R, A...>& config) -> ResolverFunc
	{
#if defined(LMMS_DISABLE_DYNAMIC_DISPATCH)
		return &SimdDispatchResolver::resolveDisabled;
#else
		if (!config.enableDynamicDispatch) { return &SimdDispatchResolver::resolveDisabled; }
		return config.preferAutoVectorization
			? &SimdDispatchResolver::resolvePreferAutoVectorization
			: &SimdDispatchResolver::resolvePreferManual;
#endif
	}

	auto resolveDisabled() const noexcept -> TargetRef
	{
		return m_config.scalar;
	}

	auto resolvePreferManual() const noexcept -> TargetRef
	{
		switch (FeatureDetection::fastRuntimeCpuFeatures() & m_highestImplementedTarget)
		{
#if defined(LMMS_HOST_X86_64)
			case LMMS_CPU_FEATURE_AVX512F:   if (m_config.avx512f) { return *m_config.avx512f; } [[fallthrough]];
			case LMMS_CPU_FEATURE_AVX2:      if (m_config.avx2)    { return *m_config.avx2; }    [[fallthrough]];
			case LMMS_CPU_FEATURE_AVX:       if (m_config.avx)     { return *m_config.avx; }     [[fallthrough]];
			case LMMS_CPU_FEATURE_SSE4_2:    if (m_config.sse4_2)  { return *m_config.sse4_2; }  [[fallthrough]];
			case LMMS_CPU_FEATURE_X86_64_V1: if (m_config.sse2)    { return *m_config.sse2; }    [[fallthrough]];
#elif defined(LMMS_HOST_ARM64)
			case LMMS_CPU_FEATURE_SVE2:      if (m_config.sve2)    { return *m_config.sve2; }    [[fallthrough]];
			case LMMS_CPU_FEATURE_SVE:       if (m_config.sve)     { return *m_config.sve; }     [[fallthrough]];
			case LMMS_CPU_FEATURE_NEON:      if (m_config.neon)    { return *m_config.neon; }    [[fallthrough]];
#endif
			case LMMS_CPU_FEATURE_NONE: [[fallthrough]];
			default: break;
		}

		return m_config.scalar;
	}

	auto resolvePreferAutoVectorization() const noexcept -> TargetRef
	{
		switch (FeatureDetection::fastRuntimeCpuFeatures() & m_highestImplementedTarget)
		{
#if defined(LMMS_HOST_X86_64)
			case LMMS_CPU_FEATURE_AVX512F:
#	if LMMS_CPU_SUPPORTS(LMMS_CPU_FEATURE_AVX512F)
				return m_config.scalar;
#	else
				if (m_config.avx512f) { return *m_config.avx512f; } [[fallthrough]];
#	endif
			case LMMS_CPU_FEATURE_AVX2:
#	if LMMS_CPU_SUPPORTS(LMMS_CPU_FEATURE_AVX2)
				return m_config.scalar;
#	else
				if (m_config.avx2)    { return *m_config.avx2; }    [[fallthrough]];
#	endif
			case LMMS_CPU_FEATURE_AVX:
#	if LMMS_CPU_SUPPORTS(LMMS_CPU_FEATURE_AVX)
				return m_config.scalar;
#	else
				if (m_config.avx)     { return *m_config.avx; }     [[fallthrough]];
#	endif
			case LMMS_CPU_FEATURE_SSE4_2:
#	if LMMS_CPU_SUPPORTS(LMMS_CPU_FEATURE_SSE4_2)
				return m_config.scalar;
#	else
				if (m_config.sse4_2)  { return *m_config.sse4_2; }  [[fallthrough]];
#	endif
			case LMMS_CPU_FEATURE_X86_64_V1:
#	if LMMS_CPU_SUPPORTS(LMMS_CPU_FEATURE_X86_64_V1)
				return m_config.scalar;
#	else
				if (m_config.sse2)    { return *m_config.sse2; }    [[fallthrough]];
#	endif
#elif defined(LMMS_HOST_ARM64)
			case LMMS_CPU_FEATURE_SVE2:
#	if LMMS_CPU_SUPPORTS(LMMS_CPU_FEATURE_SVE2)
				return m_config.scalar;
#	else
				if (m_config.sve2) { return *m_config.sve2; } [[fallthrough]];
#	endif
			case LMMS_CPU_FEATURE_SVE:
#	if LMMS_CPU_SUPPORTS(LMMS_CPU_FEATURE_SVE)
				return m_config.scalar;
#	else
				if (m_config.sve)  { return *m_config.sve; }  [[fallthrough]];
#	endif
			case LMMS_CPU_FEATURE_NEON:
#	if LMMS_CPU_SUPPORTS(LMMS_CPU_FEATURE_NEON)
				return m_config.scalar;
#	else
				if (m_config.neon) { return *m_config.neon; } [[fallthrough]];
#	endif
#endif
			case LMMS_CPU_FEATURE_NONE: [[fallthrough]];
			default: break;
		}

		return m_config.scalar;
	}

	consteval static auto getHighestImplementedTarget(const SimdDispatchConfig<ne, R, A...>& config)
		-> std::uint32_t
	{
#if defined(LMMS_HOST_X86_64)
			if (config.avx512f) { return LMMS_CPU_FEATURE_AVX512F; }
			if (config.avx2)    { return LMMS_CPU_FEATURE_AVX2; }
			if (config.avx)     { return LMMS_CPU_FEATURE_AVX; }
			if (config.sse4_2)  { return LMMS_CPU_FEATURE_SSE4_2; }
			if (config.sse2)    { return LMMS_CPU_FEATURE_X86_64_V1; }
#elif defined(LMMS_HOST_ARM64)
			if (config.sve2)    { return LMMS_CPU_FEATURE_SVE2; }
			if (config.sve)     { return LMMS_CPU_FEATURE_SVE; }
			if (config.neon)    { return LMMS_CPU_FEATURE_NEON; }
#endif
			return LMMS_CPU_FEATURE_NONE;
	}

	ResolverFunc m_resolver;
	SimdDispatchConfig<ne, R, A...> m_config;
	std::uint32_t m_highestImplementedTarget;
};

} // namespace detail

//! A runtime dispatcher for SIMD functions
template<bool ne, class R, class... A>
class SimdDispatcher
{
	using TargetRef = auto(&)(A...) noexcept(ne) -> R;
	TargetRef m_resolvedTarget;
public:
	explicit SimdDispatcher(const detail::SimdDispatchResolver<ne, R, A...>& resolver)
		: m_resolvedTarget{resolver()}
	{}

	LMMS_INLINE auto operator()(A&&... args) const noexcept(ne) -> R
	{
		if constexpr (std::is_void_v<R>)
		{
			m_resolvedTarget(std::forward<A>(args)...);
		}
		else
		{
			return m_resolvedTarget(std::forward<A>(args)...);
		}
	}
};

template<bool ne, class R, class... A>
SimdDispatcher(SimdDispatchConfig<ne, R, A...>) -> SimdDispatcher<ne, R, A...>;

} // namespace lmms

#endif // LMMS_SIMD_HELPERS_H
