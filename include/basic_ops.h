/*
 * basic_ops.h - basic memory operations
 *
 * Copyright (c) 2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifndef _BASIC_OPS_H
#define _BASIC_OPS_H

#include "lmms_basics.h"

#ifdef LMMS_HAVE_STDBOOL_H
#include <stdbool.h>
#endif

void initBasicOps( void );

void * alignedMalloc( int _bytes );
void alignedFree( void * _buf );

sampleFrameA * alignedAllocFrames( int _frames );
void alignedFreeFrames( sampleFrameA * _buf );


// all aligned* functions assume data to be 16 byte aligned and size to be
// multiples of 64
typedef void (*alignedMemCpyFunc)( void * RP _dst, const void * RP _src,
								int _size );
typedef void (*alignedMemClearFunc)( void * RP _dst, int _size );
typedef void (*alignedBufApplyGainFunc)( sampleFrameA * RP _dst,
						float _gain, int _frames );
typedef void (*alignedBufMixFunc)( sampleFrameA * RP _dst,
						const sampleFrameA * RP _src,
								int _frames );
typedef void (*alignedBufMixLRCoeffFunc)( sampleFrameA * RP _dst,
						const sampleFrameA * RP _src,
						float _left, float _right,
								int _frames );
typedef void (*unalignedBufMixLRCoeffFunc)( sampleFrame * RP _dst,
						const sampleFrame * RP _src,
						float _left, float _right,
								int _frames );
typedef void (*alignedBufWetDryMixFunc)( sampleFrameA * RP _dst,
					const sampleFrameA * RP _src,
					float _wet, float _dry, int _frames );
typedef void (*alignedBufWetDryMixSplittedFunc)( sampleFrameA * RP _dst,
					const float * RP _left,
					const float * RP _right,
					float _wet, float _dry, int _frames );
typedef int (*alignedConvertToS16Func)( const sampleFrameA * RP _src,
					intSampleFrameA * RP _dst,
					const fpp_t _frames,
					const float _master_gain,
					const bool _convert_endian );

extern alignedMemCpyFunc alignedMemCpy;
extern alignedMemClearFunc alignedMemClear;
extern alignedBufApplyGainFunc alignedBufApplyGain;
extern alignedBufMixFunc alignedBufMix;
extern alignedBufMixLRCoeffFunc alignedBufMixLRCoeff;
extern unalignedBufMixLRCoeffFunc unalignedBufMixLRCoeff;
extern alignedBufWetDryMixFunc alignedBufWetDryMix;
extern alignedBufWetDryMixSplittedFunc alignedBufWetDryMixSplitted;
extern alignedConvertToS16Func alignedConvertToS16;


#ifdef LMMS_HOST_X86
#define X86_OPTIMIZATIONS
#endif
#ifdef LMMS_HOST_X86_64
#define X86_OPTIMIZATIONS
#endif

#endif

