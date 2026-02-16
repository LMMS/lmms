/*
 * Hardware.h - This file contains useful tools that are hardware-
 * dependent, such as cache-line size and busy-wait hints.
 *
 * Copyright (c) 2026 Fawn Sannar <rubiefawn/at/gmail.com>
 *
 * This file is part of LMMS - https://lmms.io
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef LMMS_HARDWARE_H
#define LMMS_HARDWARE_H

#include <new>
#include "lmmsconfig.h"
// Includes for busy_wait_hint()
#if defined(LMMS_HOST_X86_64) || defined(LMMS_HOST_X86)
	#include <immintrin.h>
// TODO: Use LMMS_HOST_ARM64 instead of __aarch64__ and _M_ARM64 once MSVC quits
// using its own non-standard intrinsics headers (which may be never)
#elif defined(__aarch64__) // arm64
	#if defined(__ARM_ACLE)
		#include <arm_acle.h>
	#elif defined(__GNUG__)
		// TODO: Remove this once GCC properly provides __ARM_ACLE
		// 15 is the only allowed value for the parameter, so just ignore it and use 15 lol
		inline void __isb(unsigned int scope = 15)
		{
			asm volatile ("isb 15" ::: "memory");
		}
	#endif
#elif defined(_M_ARM64) // arm64 msvc
	#include <intrin.h>
#endif

namespace lmms
{

//! @brief Platform-dependent minimum amount of padding between objects to prevent false cache sharing.
// TODO: Use std::hardware_destructive_interference_size directly once our compilers are updated enough (GCC 12.1+, Clang 19+, MSVC 2017 15.3)
// TODO: Add other platforms as needed (LMMS currently only supports 64-bit x86 and ARM)
inline constexpr std::size_t hardware_destructive_interference_size =
#if __cpp_lib_hardware_interference_size >= 201703L
	std::hardware_destructive_interference_size
#elif defined(LMMS_HOST_ARM64)
	256
#elif defined(LMMS_HOST_X86_64) || defined(LMMS_HOST_X86)
	64
#else
	#warning Defaulting to 64 for lmms::hardware_destructive_interference_size for this architecture. This may be incorrect.
	64
#endif
	;

// TODO: Add docs now that this isn't a macro
inline void busy_wait_hint()
{
#if defined(LMMS_HOST_X86_64) || defined(LMMS_HOST_X86)
	_mm_pause()
#elif defined(LMMS_HOST_ARM64)
	__isb(15)
#else
	// TODO: Add other platforms as needed (LMMS currently only supports 64-bit x86 and ARM)
	#error No busy-wait hint intrinsic available for this platform!
#endif
	;
}

} // namespace lmms

#endif // LMMS_HARDWARE_H
