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

//! @brief Platform-dependent minimum amount of padding between objects to prevent false cache sharing.
// TODO: Use std::hardware_destructive_interference_size directly once our compilers are updated enough (GCC 12.1+, Clang 19+, MSVC 2017 15.3)
// TODO: Add other platforms as needed (LMMS currently only supports 64-bit x86 and ARM)
#if __cpp_lib_hardware_interference_size >= 201703L
#include <new>
namespace lmms
{
inline constexpr std::size_t hardware_destructive_interference_size = std::hardware_destructive_interference_size;
}
#elif defined(__aarch64__) || defined(_M_ARM64) // 64-bit ARM
namespace lmms
{
inline constexpr std::size_t hardware_destructive_interference_size = 256;
}
#else // x86_64
namespace lmms
{
inline constexpr std::size_t hardware_destructive_interference_size = 64;
}
#endif


#if defined(__x86_64__) || defined(_M_AMD64) || defined(__i386__) || defined(_M_IX86)
	#include <immintrin.h>
	#define busy_wait_hint() _mm_pause()
#elif defined(__aarch64__) // arm64
	// isb 15 used instead of yield on arm64
	// See https://github.com/rust-lang/rust/commit/c064b6560b7ce0adeb9bbf5d7dcf12b1acb0c807
	#if defined(__ARM_ACLE)
		#include <arm_acle.h>
		#define busy_wait_hint() __isb(15)
	#elif defined(__GNUG__)
		// GCC for ARM lacks these intrinsics at the moment.
		// TODO: Remove inline asm once GCC properly provides __ARM_ACLE
		#define busy_wait_hint() asm volatile ("isb 15" ::: "memory")
	#endif
#elif defined(_M_ARM64) // arm64 msvc
	#include <intrin.h>
	// See https://github.com/rust-lang/rust/commit/c064b6560b7ce0adeb9bbf5d7dcf12b1acb0c807
	#define busy_wait_hint() __isb(15)
#else
	// TODO: Add other platforms as needed (LMMS currently only supports 64-bit x86 and ARM)
	#error No busy-wait hint intrinsic available for this platform!
#endif

#endif
