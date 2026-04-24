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

#include <cstdint>
#include <new>
#include "lmmsconfig.h"
#if defined(LMMS_HOST_X86_64) || defined(LMMS_HOST_X86)
	#include <immintrin.h>
#elif defined(LMMS_HOST_ARM64) || defined(LMMS_HOST_ARM32)
	#if defined(__ARM_ACLE)
		#include <arm_acle.h>
	#elif defined(_MSC_VER)
		#include <intrin.h>
	#elif defined(__GNUG__) // HACK: Remove this once GCC properly provides __ARM_ACLE
		#if defined(LMMS_HOST_ARM64)
			// https://developer.arm.com/documentation/ddi0602/2026-03/Base-Instructions/ISB--Instruction-synchronization-barrier-
			// 15 is the only allowed value for the parameter, so just ignore it and use 15 lol
			inline void __isb(unsigned int scope = 15) { asm volatile ("isb 15" ::: "memory"); }
		#elif defined(LMMS_HOST_ARM32)
			inline void __yield() { asm volatile ("yield"); }
		#endif
	#endif
#endif

namespace lmms
{

//! @brief Platform-dependent minimum amount of padding between objects to prevent false cache sharing.
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



//! @brief Platform-dependent hint to the processor that it is in a busy-wait loop.
//! This helps optimize spinlocks by slowing down the processor a bit, which helps reduce contention on atomics.
// TODO: Add other platforms as needed (LMMS_HOST_*)
inline void busyWaitHint()
{
#if defined(LMMS_HOST_X86_64) || defined(LMMS_HOST_X86)
	_mm_pause()
#elif defined(LMMS_HOST_ARM64)
	__isb(15)
#elif defined(LMMS_HOST_ARM32)
	__yield()
#else
	#warning lmms::busyWaitHint() is not implemented on this architecture and will have no effect. Performance may suffer.
	(void)
#endif
	;
}



//! @brief Disable IEEE 754 denormals on the current thread.
//!
//! This is desirable for audio calculations as processing denormal
//! values is significantly slower than normal values, and in the
//! context of audio signal processing, denormal values are typically
//! near silence and contain no useful information anyways.
inline void disableDenormals()
{
#if defined(LMMS_HOST_X86_64) || defined(LMMS_HOST_X86)
	// https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html
	// Intel® 64 and IA-32 Architectures Software Developer's Manual Volume 1: Basic Architecture
	// 11.6.3 Checking for the DAZ Flag in the MXCSR Register
	unsigned int flags = 0x8000; // FTZ
	std::uint8_t buf[512] = {0};
	#if defined(LMMS_HOST_X86_64)
		_fxsave64(buf);
	#elif defined(LMMS_HOST_X86)
		_fxsave(buf);
	#endif
	flags |= buf[28] & 0x0040; // DAZ if supported
	_mm_setcsr(_mm_getcsr() | flags);
#elif defined(LMMS_HOST_ARM64) || defined(LMMS_HOST_ARM32)
	constexpr auto FZ = 1 << 24; // Flushing denormalized numbers to zero control bit
	#if defined(_MSC_VER) && defined(LMMS_HOST_ARM64)
		// https://learn.microsoft.com/en-us/cpp/intrinsics/arm64-intrinsics
		// 0x5a20 == ARM64_SYSREG(0b11, 0b011, 0b0100, 0b0100, 0b000)
		_WriteStatusReg(0x5a20, _ReadStatusReg(0x5a20) | FZ);
	#elif defined(_MSC_VER) && defined(LMMS_HOST_ARM32)
		// MSVC for ARM32 is discontinued, so no point in implementing
		// this. Attempting to build using MSVC for ARM32 anyways will
		// result in fatal error C1189. Thanks Microsoft!
		#warning lmms::disableDenormals() is not implemented on this compiler and will have no effect. Performance may suffer.
	#elif defined(LMMS_HOST_ARM64)
		// https://developer.arm.com/documentation/ddi0601/2026-03/AArch64-Registers/FPCR--Floating-point-Control-Register
		std::uint64_t fpcr;
		asm volatile ("mrs %0, fpcr" : "=r" (fpcr));
		asm volatile ("msr fpcr, %0" :: "ri" (fpcr | FZ));
	#elif defined(LMMS_HOST_ARM32)
		// https://developer.arm.com/documentation/ddi0601/2026-03/AArch32-Registers/FPSCR--Floating-Point-Status-and-Control-Register
		std::uint32_t fpscr;
		asm volatile ("vmrs %0, fpscr" : "=r" (fpscr));
		asm volatile ("vmsr fpscr, %0" :: "ri" (fpscr | FZ));
	#endif
#else
	#warning lmms::disableDenormals() is not implemented on this architecture and will have no effect. Performance may suffer.
#endif
}

} // namespace lmms

#endif // LMMS_HARDWARE_H
