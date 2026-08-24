/*
 * FeatureDetection.cpp - compile-time and runtime CPU feature detection
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

#include "FeatureDetection.h"

#include "versioninfo.h"

#if defined(LMMS_HOST_X86_64) && defined(_MSC_VER)
// See: https://learn.microsoft.com/en-us/cpp/intrinsics/cpuid-cpuidex?view=msvc-170
#include <intrin.h>
#endif

namespace lmms {

auto FeatureDetection::determineRuntimeCpuFeatures() noexcept -> std::uint32_t
{
	auto result = std::uint32_t{LMMS_CPU_FEATURE_NONE};

#if defined(LMMS_HOST_X86_64)
	// eax, ebx, ecx, edx registers
	int regs[4] = {}; // NOLINT

#if defined(_MSC_VER)
	__cpuid(regs, /* eax */ 1);
#else
	asm volatile("cpuid"
		: "=a" (regs[0]), "=b" (regs[1]), "=c" (regs[2]), "=d" (regs[3])
		: "a" (1), "c" (0));
#endif

	if ((regs[3] & (1 << 25)) && (regs[3] & (1 << 26))) // SSE + SSE2
	{
		if ((regs[2] & (1 << 0))      // SSE3
			&& (regs[2] & (1 << 9))   // SSSE3
			&& (regs[2] & (1 << 19))  // SSE4.1
			&& (regs[2] & (1 << 20))) // SSE4.2
		{
			result |= LMMS_CPU_FEATURE_SSE4_2;
		}
		else
		{
			result |= LMMS_CPU_FEATURE_X86_64_V1;
		}
	}

	if (regs[2] & (1 << 28)) { result |= LMMS_CPU_FEATURE_AVX; }

#if defined(_MSC_VER)
	__cpuidex(regs, /* eax */ 7, /* ecx */ 0);
#else
	asm volatile("cpuid"
		: "=a" (regs[0]), "=b" (regs[1]), "=c" (regs[2]), "=d" (regs[3])
		: "a" (7), "c" (0));
#endif

	if (result & LMMS_CPU_FEATURE_AVX)
	{
		if (regs[1] & (1 << 5)) // AVX2
		{
			if (regs[1] & (1 << 16)) // AVX512F
			{
				result |= LMMS_CPU_FEATURE_AVX512F;
			}
			else
			{
				result |= LMMS_CPU_FEATURE_AVX2;
			}
		}
	}
#endif // x86_64

	// TODO: ARM64 feature detection

	return result;
}

auto FeatureDetection::formattedCpuFeatures(std::uint32_t features) -> std::string
{
	auto str = '[' + std::string{LMMS_BUILDCONF_MACHINE} + "] ";

#if defined(LMMS_HOST_X86_64)
	if ((LMMS_CPU_FEATURE_X86_64_V1 & features) == LMMS_CPU_FEATURE_X86_64_V1)
	{
		str += "x86_64-v1, ";
	}
	if ((LMMS_CPU_FEATURE_SSE4_2 & features) == LMMS_CPU_FEATURE_SSE4_2)
	{
		str += "SSE3, SSSE3, SSE4.1, SSE4.2, ";
	}
	if ((LMMS_CPU_FEATURE_AVX & features) == LMMS_CPU_FEATURE_AVX)
	{
		str += "AVX, ";
	}
	if ((LMMS_CPU_FEATURE_AVX2 & features) == LMMS_CPU_FEATURE_AVX2)
	{
		str += "AVX2, ";
	}
	if ((LMMS_CPU_FEATURE_AVX512F & features) == LMMS_CPU_FEATURE_AVX512F)
	{
		str += "AVX512F, ";
	}
#elif defined(LMMS_HOST_ARM64)
	if ((LMMS_CPU_FEATURE_NEON & features) == LMMS_CPU_FEATURE_NEON)
	{
		str += "NEON, ";
	}
	if ((LMMS_CPU_FEATURE_SVE & features) == LMMS_CPU_FEATURE_SVE)
	{
		str += "SVE, ";
	}
	if ((LMMS_CPU_FEATURE_SVE2 & features) == LMMS_CPU_FEATURE_SVE2)
	{
		str += "SVE2, ";
	}
#else
	str += "???";
#endif

	if (str.ends_with(", ")) { str.erase(str.size() - 2); }

	return str;
}

} // namespace lmms
