/*
 * FeatureDetection.h - compile-time and runtime CPU feature detection
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

#ifndef LMMS_FEATURE_DETECTION_H
#define LMMS_FEATURE_DETECTION_H

// NOTE: This header is NOT meant to provide an exhaustive, granular list of CPU features,
//       but only the features that we actually want to use and support compile-time
//       or runtime dispatch on. Feel free to add additional CPU features here as needed.

#include "lmmsconfig.h"
#include "lmms_export.h"

#include <cstdint>
#include <string>

///////////////////////
// CPU feature flags //
///////////////////////

#define LMMS_CPU_FEATURE_NONE    0u
#define LMMS_CPU_FEATURE_INVALID 0x80000000u

// x86_64 features
#if defined(LMMS_HOST_X86_64) || defined(DOXYGEN)
//! CMOV, CMPXCHG8B (CX8), FPU, FXSR, MMX, OSFXSR, SCE, SSE, SSE2
//! @note x86_64-v1 is the baseline for all x86_64 CPUs, so this feature is unconditionally present.
#	define LMMS_CPU_FEATURE_X86_64_V1 (1u << 0)
//! x86-64-v1 + SSE3, SSSE3, SSE4.1, and SSE4.2
#	define LMMS_CPU_FEATURE_SSE4_2    ((1u << 1) | LMMS_CPU_FEATURE_X86_64_V1)
#	define LMMS_CPU_FEATURE_AVX       (1u << 2)
#	define LMMS_CPU_FEATURE_AVX2      ((1u << 3) | LMMS_CPU_FEATURE_AVX)
#	define LMMS_CPU_FEATURE_AVX512F   ((1u << 4) | LMMS_CPU_FEATURE_AVX2)
#else
#	define LMMS_CPU_FEATURE_X86_64_V1 LMMS_CPU_FEATURE_INVALID
#	define LMMS_CPU_FEATURE_SSE4_2    LMMS_CPU_FEATURE_INVALID
#	define LMMS_CPU_FEATURE_AVX       LMMS_CPU_FEATURE_INVALID
#	define LMMS_CPU_FEATURE_AVX2      LMMS_CPU_FEATURE_INVALID
#	define LMMS_CPU_FEATURE_AVX512F   LMMS_CPU_FEATURE_INVALID
#endif

// arm64 features
#if defined(LMMS_HOST_ARM64) || defined(DOXYGEN)
#	define LMMS_CPU_FEATURE_NEON      (1u << 0)
#	define LMMS_CPU_FEATURE_SVE       ((1u << 1) | LMMS_CPU_FEATURE_NEON)
#	define LMMS_CPU_FEATURE_SVE2      ((1u << 2) | LMMS_CPU_FEATURE_SVE)
#else
#	define LMMS_CPU_FEATURE_NEON      LMMS_CPU_FEATURE_INVALID
#	define LMMS_CPU_FEATURE_SVE       LMMS_CPU_FEATURE_INVALID
#	define LMMS_CPU_FEATURE_SVE2      LMMS_CPU_FEATURE_INVALID
#endif

///////////////////////////////////////////
// Compile-time microarchitecture target //
///////////////////////////////////////////

#if defined(DOXYGEN)
//! @brief Contains @a LMMS_CPU_FEATURE_* flags indicating which features are unconditionally present
//!        on the target microarchitecture.
//!
//! More efficient code paths can be conditionally enabled at compile-time based on the value
//! of this macro. The value is affected by the @a TARGET_UARCH config option.
//!
//! @see LMMS_CPU_SUPPORTS
#	define LMMS_TARGET_CPU_FEATURES
#elif defined(LMMS_HOST_X86_64)
#	include <immintrin.h>
#	if defined(_MSC_VER)
		// See: https://learn.microsoft.com/en-us/cpp/intrinsics/check-isa-arch-support?view=msvc-170
#		include <isa_availability.h>
#		if __check_arch_support(__IA_SUPPORT_SSE42, 0)
#			define LMMS_TARGET_SSE_LEVEL LMMS_CPU_FEATURE_SSE4_2
#		else
#			define LMMS_TARGET_SSE_LEVEL LMMS_CPU_FEATURE_X86_64_V1
#		endif
#	else
#		if defined(__SSE4_2__) && defined(__SSE4_1__) && defined(__SSSE3__) && defined(__SSE3__)
#			define LMMS_TARGET_SSE_LEVEL LMMS_CPU_FEATURE_SSE4_2
#		else
#			define LMMS_TARGET_SSE_LEVEL LMMS_CPU_FEATURE_X86_64_V1
#		endif
#	endif
	// See: https://learn.microsoft.com/en-us/cpp/preprocessor/predefined-macros?view=msvc-170
#	if defined(__AVX512F__)
#		define LMMS_TARGET_AVX_LEVEL LMMS_CPU_FEATURE_AVX512F
#	elif defined(__AVX2__)
#		define LMMS_TARGET_AVX_LEVEL LMMS_CPU_FEATURE_AVX2
#	elif defined(__AVX__)
#		define LMMS_TARGET_AVX_LEVEL LMMS_CPU_FEATURE_AVX
#	else
#		define LMMS_TARGET_AVX_LEVEL LMMS_CPU_FEATURE_NONE
#	endif
#	define LMMS_TARGET_CPU_FEATURES (LMMS_TARGET_SSE_LEVEL | LMMS_TARGET_AVX_LEVEL)
#elif defined(LMMS_HOST_ARM64)
	// TODO: Add support for MSVC
#	if defined(__ARM_FEATURE_SVE2)
#		define LMMS_TARGET_CPU_FEATURES LMMS_CPU_FEATURE_SVE2
#	elif defined(__ARM_FEATURE_SVE)
#		define LMMS_TARGET_CPU_FEATURES LMMS_CPU_FEATURE_SVE
#	elif defined(__ARM_NEON) || defined(__ARM_NEON__)
#		define LMMS_TARGET_CPU_FEATURES LMMS_CPU_FEATURE_NEON
#	else
#		define LMMS_TARGET_CPU_FEATURES LMMS_CPU_FEATURE_NONE
#	endif
else
	// Other processor
#	define LMMS_TARGET_CPU_FEATURES LMMS_CPU_FEATURE_NONE
#endif

namespace lmms {

//! Provides runtime CPU feature detection
class LMMS_EXPORT FeatureDetection
{
public:
	//! @returns @a LMMS_CPU_FEATURE_* flags indicating all CPU features detected at runtime
	//! @note Feature detection runs only during the first function call and results are cached for subsequent calls.
	static auto runtimeCpuFeatures() noexcept -> std::uint32_t
	{
		if (m_cachedFeatures != LMMS_CPU_FEATURE_INVALID) { return m_cachedFeatures; }
		m_cachedFeatures = determineRuntimeCpuFeatures();
		return m_cachedFeatures;
	}

	//! @brief Checks whether the current CPU meets the baseline CPU requirements for this particular LMMS build
	//!
	//! This function could, for example, allow an LMMS build that targets a CPU with AVX512 instructions
	//! to exit gracefully when attempting to run on an old CPU without those instructions.
	static auto isCurrentCpuSupported() noexcept -> bool
	{
		return (runtimeCpuFeatures() & LMMS_TARGET_CPU_FEATURES) == LMMS_TARGET_CPU_FEATURES;
	}

	//! @returns a display string for the given CPU features
	static auto formattedCpuFeatures(std::uint32_t features = LMMS_TARGET_CPU_FEATURES) -> std::string;

private:
	static auto determineRuntimeCpuFeatures() noexcept -> std::uint32_t;
	static inline std::uint32_t m_cachedFeatures = LMMS_CPU_FEATURE_INVALID;
};

} // namespace lmms

//! Checks if the specified CPU features are supported due to the compile-time microarchitecture target.
//! Can be used in preprocessor #if/#else conditions, if-constexpr conditions, or regular runtime conditions.
#define LMMS_CPU_SUPPORTS(features) \
	(((features) & LMMS_TARGET_CPU_FEATURES) == (features))

//! Checks if the specified CPU features are supported due to the compile-time microarchitecture target
//! or if support is detected on the current CPU at runtime.
#define LMMS_RUNTIME_CPU_SUPPORTS(features) \
	(LMMS_CPU_SUPPORTS((features)) || ((features) & FeatureDetection::runtimeCpuFeatures()) == (features))

#endif // LMMS_FEATURE_DETECTION_H
