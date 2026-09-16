/*
 * SimdIntrinsics.h - Cross-platform SIMD intrinsics
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

#ifndef LMMS_SIMD_INTRINSICS_H
#define LMMS_SIMD_INTRINSICS_H

#include "FeatureDetection.h"

#if defined(LMMS_HOST_X86_64)
#	include <immintrin.h>
#elif defined(LMMS_HOST_ARM64)
#	include <arm_neon.h>
#endif

namespace lmms {

#if defined(LMMS_HOST_X86_64)
#	ifndef _MSC_VER
#		define LMMS_DEFINE_SIMD_GENERIC(name, fn128, fn256, fn512)              \
			template<std::uint8_t lanes> inline constexpr const auto& name = 0; \
			template<> inline constexpr const auto& name<4>  = fn128;           \
			template<> inline constexpr const auto& name<8>  = fn256;           \
			template<> inline constexpr const auto& name<16> = fn512;
#	else
#		define LMMS_DEFINE_SIMD_GENERIC(name, fn128, fn256, fn512) \
			template<std::uint8_t lanes, class... Args>            \
			LMMS_INLINE auto name(Args... args) -> decltype(auto)  \
			{                                                      \
			    if constexpr (lanes == 4) {                        \
			        return (fn128)(args...);                       \
			    } else if constexpr (lanes == 8) {                 \
			        return (fn256)(args...);                       \
			    } else if constexpr (lanes == 16) {                \
			        return (fn512)(args...);                       \
			    } else { static_assert(lanes == 4); }              \
			}
#	endif
#elif defined(LMMS_HOST_ARM64)
#	ifndef _MSC_VER
#		define LMMS_DEFINE_SIMD_GENERIC(name, fn128)                            \
			template<std::uint8_t lanes> inline constexpr const auto& name = 0; \
			template<> inline constexpr const auto& name<4>  = fn128;
#	else
#		define LMMS_DEFINE_SIMD_GENERIC(name, fn128)               \
			template<std::uint8_t lanes, class... Args>            \
			LMMS_INLINE auto name(Args... args) -> decltype(auto)  \
			{                                                      \
			    if constexpr (lanes == 4) {                        \
			        return (fn128)(args...);                       \
			    } else { static_assert(lanes == 4); }              \
			}
#	endif
#endif

namespace simd {
namespace detail {

template<typename T, std::uint8_t lanes>
struct Vec {};

template<typename T>
struct Vec<T, 1> { using type = T; };

template<>
struct Vec<float, 4>
{
#if defined(LMMS_HOST_X86_64)
	using type = __m128;
#elif defined(LMMS_HOST_ARM64)
	using type = float32x4_t;
#endif
};

template<>
struct Vec<float, 8>
{
#if defined(LMMS_HOST_X86_64)
	using type = __m256;
#endif
};

template<>
struct Vec<float, 16>
{
#if defined(LMMS_HOST_X86_64)
	using type = __m512;
#endif
};

} // namespace detail

//! Cross-platform lane-generic SIMD vector type
template<typename DataType, std::uint8_t lanes>
using Vec = detail::Vec<DataType, lanes>::type;

/////////////////////////////////////////////////
// Cross-platform lane-generic SIMD intrinsics //
/////////////////////////////////////////////////

#if defined(LMMS_HOST_X86_64)
	LMMS_DEFINE_SIMD_GENERIC(loadAligned,    _mm_load_ps,   _mm256_load_ps,   _mm512_load_ps)
	LMMS_DEFINE_SIMD_GENERIC(loadUnaligned,  _mm_loadu_ps,  _mm256_loadu_ps,  _mm512_loadu_ps)
	LMMS_DEFINE_SIMD_GENERIC(storeAligned,   _mm_store_ps,  _mm256_store_ps,  _mm512_store_ps)
	LMMS_DEFINE_SIMD_GENERIC(storeUnaligned, _mm_storeu_ps, _mm256_storeu_ps, _mm512_storeu_ps)
	LMMS_DEFINE_SIMD_GENERIC(add,            _mm_add_ps,    _mm256_add_ps,    _mm512_add_ps)
#elif defined(LMMS_HOST_ARM64)
	LMMS_DEFINE_SIMD_GENERIC(loadAligned,    vld1q_f32)
	LMMS_DEFINE_SIMD_GENERIC(loadUnaligned,  vld1q_f32)
	LMMS_DEFINE_SIMD_GENERIC(storeAligned,   vst1q_f32)
	LMMS_DEFINE_SIMD_GENERIC(storeUnaligned, vst1q_f32)
	LMMS_DEFINE_SIMD_GENERIC(add,            vaddq_f32)
#endif

// NOTE: Can define more generic intrinsics here as needed

} // namespace simd

#undef LMMS_DEFINE_SIMD_GENERIC

template<typename DataType, std::uint8_t lanes, bool aligned = false>
struct SimdIntrinsics
{
	static_assert(lanes != 0 && (lanes & (lanes - 1)) == 0, "lanes must be a power of 2");

	using Vec = simd::Vec<DataType, lanes>;

	//! @returns the number of elements in a buffer containing @a count elements of the
	//!          type @a DataType that can be processed using SIMD operations with @a lanes lanes.
	//! @note Any remaining elements constitute the "tail" and must be processed using scalar operations.
	LMMS_INLINE static constexpr auto vectorizableCount(std::size_t count) noexcept -> std::size_t
	{
		if constexpr (lanes > 1)
		{
			constexpr std::size_t mask = lanes - 1;
			return count - (count & mask);
		}
		else
		{
			return 0;
		}
	}

	// Loads

	static constexpr const auto& loadAligned    = simd::loadAligned<lanes>;
	static constexpr const auto& loadUnaligned  = simd::loadUnaligned<lanes>;
	static constexpr const auto& load           = aligned ? loadAligned : loadUnaligned;

	// Stores

	static constexpr const auto& storeAligned   = simd::storeAligned<lanes>;
	static constexpr const auto& storeUnaligned = simd::storeUnaligned<lanes>;
	static constexpr const auto& store          = aligned ? storeAligned : storeUnaligned;

	// Add

	static constexpr const auto& add            = simd::add<lanes>;
};

} // namespace lmms

#endif // LMMS_SIMD_INTRINSICS_H
