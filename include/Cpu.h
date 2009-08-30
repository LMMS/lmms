/*
 * Cpu.h - CPU specific accellerated operations
 *
 * Copyright (c) 2008-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 *
 * This file is part of Linux MultiMedia Studio - http://lmms.sourceforge.net
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

#ifndef _CPU_H
#define _CPU_H

#include "lmms_basics.h"
#include "export.h"

#ifdef LMMS_HAVE_STDBOOL_H
#include <stdbool.h>
#endif

#ifdef __cplusplus
namespace CPU
{
#endif

void init();

void * memAlloc( int _bytes );
void memFree( void * _buf );

sampleFrameA * EXPORT allocFrames( int _frames );
void EXPORT freeFrames( sampleFrameA * _buf );


// all functions assume data to be 16 byte  and size to be
// multiples of 64 (except for unaligned*())
typedef void (*MemCpyFunc)( void * RP _dst, const void * RP _src,
								int _size );
typedef void (*MemClearFunc)( void * RP _dst, int _size );
typedef void (*BufApplyGainFunc)( sampleFrameA * RP _dst,
						float _gain, int _frames );
typedef void (*BufMixFunc)( sampleFrameA * RP _dst,
						const sampleFrameA * RP _src,
								int _frames );
typedef void (*BufMixLRCoeffFunc)( sampleFrameA * RP _dst,
						const sampleFrameA * RP _src,
						float _left, float _right,
								int _frames );
typedef void (*UnalignedBufMixLRCoeffFunc)( sampleFrame * RP _dst,
						const sampleFrame * RP _src,
						float _left, float _right,
								int _frames );
typedef void (*BufWetDryMixFunc)( sampleFrameA * RP _dst,
					const sampleFrameA * RP _src,
					float _wet, float _dry, int _frames );
typedef void (*BufWetDryMixSplittedFunc)( sampleFrameA * RP _dst,
					const float * RP _left,
					const float * RP _right,
					float _wet, float _dry, int _frames );
typedef int (*ConvertToS16Func)( const sampleFrameA * RP _src,
					intSampleFrameA * RP _dst,
					const fpp_t _frames,
					const float _master_gain,
					const bool _convert_endian );

extern MemCpyFunc memCpy;
extern MemClearFunc memClear;
extern BufApplyGainFunc bufApplyGain;
extern BufMixFunc bufMix;
extern BufMixLRCoeffFunc bufMixLRCoeff;
extern UnalignedBufMixLRCoeffFunc unalignedBufMixLRCoeff;
extern BufWetDryMixFunc bufWetDryMix;
extern EXPORT BufWetDryMixSplittedFunc bufWetDryMixSplitted;
extern ConvertToS16Func convertToS16;

#ifdef __cplusplus
}
#endif

#ifdef LMMS_HOST_X86
#define X86_OPTIMIZATIONS
#endif
#ifdef LMMS_HOST_X86_64
#define X86_OPTIMIZATIONS
#endif

#endif

