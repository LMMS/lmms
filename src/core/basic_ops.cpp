/*
 * basic_ops.cpp - basic memory operations
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


#include "basic_ops.h"

#include <cstdlib>
#include <cstdio>
#include <memory.h>



void * alignedMalloc( int _bytes )
{
	char *ptr,*ptr2,*aligned_ptr;
	int align_mask = ALIGN_SIZE- 1;
	ptr =(char *) malloc( _bytes + ALIGN_SIZE + sizeof(int) );
	if( ptr == NULL )
	{
		return NULL;
	}

	ptr2 = ptr + sizeof(int);
	aligned_ptr = ptr2 + ( ALIGN_SIZE- ( (size_t) ptr2 & align_mask ) );


	ptr2 = aligned_ptr - sizeof(int);
	*((int *) ptr2) = (int)( aligned_ptr - ptr );

	return aligned_ptr;
}


void alignedFree( void * _buf )
{
	if( _buf )
	{
		int * ptr2 = (int *) _buf - 1;
		void * buf2 = (char *) _buf - *ptr2;
		if( buf2 )
		{
			free( buf2 );
		}
	}
}


sampleFrameA * alignedAllocFrames( int _n )
{
	return (sampleFrameA *) alignedMalloc( _n * sizeof( sampleFrameA ) );
}


void alignedFreeFrames( sampleFrame * _buf )
{
	alignedFree( _buf );
}




// slow fallback
void alignedMemCpyNoOpt( void * RP _dst, const void * RP _src, int _size )
{
	const int s = _size / ( sizeof( int ) * 16 );
	const int * RP src = (const int *) _src;
	int * RP dst = (int *) _dst;
	for( int i = 0; i < s; )
	{
		dst[i+0] = src[i+0];
		dst[i+1] = src[i+1];
		dst[i+2] = src[i+2];
		dst[i+3] = src[i+3];
		dst[i+4] = src[i+4];
		dst[i+5] = src[i+5];
		dst[i+6] = src[i+6];
		dst[i+7] = src[i+7];
		dst[i+8] = src[i+8];
		dst[i+9] = src[i+9];
		dst[i+10] = src[i+10];
		dst[i+11] = src[i+11];
		dst[i+12] = src[i+12];
		dst[i+13] = src[i+13];
		dst[i+14] = src[i+14];
		dst[i+15] = src[i+15];
		i += 16;
	}
}


// slow fallback
void alignedMemClearNoOpt( void * _dst, int _size )
{
	const int s = _size / ( sizeof( int ) * 4 );
	int * dst = (int *) _dst;
	for( int i = 0; i < s; ++i )
	{
		dst[0] = 0;
		dst[1] = 0;
		dst[2] = 0;
		dst[3] = 0;
		dst += 4;
	}
}



void alignedBufApplyGainNoOpt( sampleFrameA * RP _dst, float _gain,
								int _frames )
{
	for( int i = 0; i < _frames; )
	{
		_dst[i+0][0] *= _gain;
		_dst[i+0][1] *= _gain;
		_dst[i+1][0] *= _gain;
		_dst[i+1][1] *= _gain;
		_dst[i+2][0] *= _gain;
		_dst[i+2][1] *= _gain;
		_dst[i+3][0] *= _gain;
		_dst[i+3][1] *= _gain;
		_dst[i+4][0] *= _gain;
		_dst[i+4][1] *= _gain;
		_dst[i+5][0] *= _gain;
		_dst[i+5][1] *= _gain;
		_dst[i+6][0] *= _gain;
		_dst[i+6][1] *= _gain;
		_dst[i+7][0] *= _gain;
		_dst[i+7][1] *= _gain;
		i += 8;
	}
}


void alignedBufMixNoOpt( sampleFrameA * RP _dst, const sampleFrameA * RP _src,
								int _frames )
{
	for( int i = 0; i < _frames; )
	{
		_dst[i+0][0] += _src[i+0][0];
		_dst[i+0][1] += _src[i+0][1];
		_dst[i+1][0] += _src[i+1][0];
		_dst[i+1][1] += _src[i+1][1];
		_dst[i+2][0] += _src[i+2][0];
		_dst[i+2][1] += _src[i+2][1];
		_dst[i+3][0] += _src[i+3][0];
		_dst[i+3][1] += _src[i+3][1];
		i += 4;
	}
}



void alignedBufMixLRCoeffNoOpt( sampleFrameA * RP _dst,
					const sampleFrameA * RP _src,
					float _left, float _right, int _frames )
{
	for( int i = 0; i < _frames; )
	{
		_dst[i+0][0] += _src[i+0][0]*_left;
		_dst[i+0][1] += _src[i+0][1]*_right;
		_dst[i+1][0] += _src[i+1][0]*_left;
		_dst[i+1][1] += _src[i+1][1]*_right;
		_dst[i+2][0] += _src[i+2][0]*_left;
		_dst[i+2][1] += _src[i+2][1]*_right;
		_dst[i+3][0] += _src[i+3][0]*_left;
		_dst[i+3][1] += _src[i+3][1]*_right;
		i += 4;
	}
}



void unalignedBufMixLRCoeffNoOpt( sampleFrame * RP _dst,
						const sampleFrame * RP _src,
							const float _left,
							const float _right,
								int _frames )
{
	if( _frames % 2 )
	{
		_dst[0][0] += _src[0][0] * _left;
		_dst[0][1] += _src[0][1] * _right;
		++_src;
		++_dst;
		--_frames;
	}
	for( int i = 0; i < _frames; )
	{
		_dst[i+0][0] += _src[i+0][0]*_left;
		_dst[i+0][1] += _src[i+0][1]*_right;
		_dst[i+1][0] += _src[i+1][0]*_left;
		_dst[i+1][1] += _src[i+1][1]*_right;
		i += 2;
	}
}



void alignedBufWetDryMixNoOpt( sampleFrameA * RP _dst,
					const sampleFrameA * RP _src,
					float _wet, float _dry, int _frames )
{
	for( int i = 0; i < _frames; ++i )
	{
		_dst[i+0][0] = _dst[i+0][0]*_dry + _src[i+0][0]*_wet;
		_dst[i+0][1] = _dst[i+0][1]*_dry + _src[i+0][1]*_wet;
	}
}




void alignedBufWetDryMixSplittedNoOpt( sampleFrameA * RP _dst,
					const float * RP _left,
					const float * RP _right,
					float _wet, float _dry, int _frames )
{
	int i;
	for( i = 0; i < _frames; ++i )
	{
		_dst[i+0][0] = _dst[i+0][0]*_dry + _left[i+0]*_wet;
		_dst[i+0][1] = _dst[i+0][1]*_dry + _right[i+0]*_wet;
		++i;
	}
}




int alignedConvertToS16NoOpt( const sampleFrameA * RP _src,
					intSampleFrameA * RP _dst,
					const fpp_t _frames,
					const float _master_gain,
					const bool _convert_endian )
{
	int t1;
	int t2;
	const float f = _master_gain * OUTPUT_SAMPLE_MULTIPLIER;
	if( _convert_endian )
	{
		for( fpp_t frame = 0; frame < _frames; ++frame )
		{
			t1 = _src[frame][0] * f;
			t1 = unlikely( t1 > 32767 ) ? 32767 : t1;
			t1 = unlikely( t1 < -32768 ) ? -32768 : t1;
			_dst[frame][0] = ( t1 & 0x00ff) << 8 |
							( t1 & 0xff00 ) >> 8;

			t2 = _src[frame][1] * f;
			t2 = unlikely( t2 > 32767 ) ? 32767 : t2;
			t2 = unlikely( t2 < -32768 ) ? -32768 : t2;
			_dst[frame][1] = ( t2 & 0x00ff) << 8 |
						( t2 & 0xff00 ) >> 8;
		}
	}
	else
	{
		for( fpp_t frame = 0; frame < _frames; ++frame )
		{
			t1 = _src[frame][0] * f;
			t1 = unlikely( t1 > 32767 ) ? 32767 : t1;
			t1 = unlikely( t1 < -32768 ) ? -32768 : t1;
			_dst[frame][0] = t1;

			t2 = _src[frame][1] * f;
			t2 = unlikely( t2 > 32767 ) ? 32767 : t2;
			t2 = unlikely( t2 < -32768 ) ? -32768 : t2;
			_dst[frame][1] = t2;
		}
	}

	return _frames * DEFAULT_CHANNELS * BYTES_PER_INT_SAMPLE;
}


alignedMemCpyFunc alignedMemCpy = alignedMemCpyNoOpt;
alignedMemClearFunc alignedMemClear = alignedMemClearNoOpt;
alignedBufApplyGainFunc alignedBufApplyGain = alignedBufApplyGainNoOpt;
alignedBufMixFunc alignedBufMix = alignedBufMixNoOpt;
alignedBufMixLRCoeffFunc alignedBufMixLRCoeff = alignedBufMixLRCoeffNoOpt;
unalignedBufMixLRCoeffFunc unalignedBufMixLRCoeff = unalignedBufMixLRCoeffNoOpt;
alignedBufWetDryMixFunc alignedBufWetDryMix = alignedBufWetDryMixNoOpt;
alignedBufWetDryMixSplittedFunc alignedBufWetDryMixSplitted = alignedBufWetDryMixSplittedNoOpt;
alignedConvertToS16Func alignedConvertToS16 = alignedConvertToS16NoOpt;


#ifdef X86_OPTIMIZATIONS
enum CPUFeatures
{
    None        = 0,
    MMX         = 0x1,
    MMXEXT      = 0x2,
    MMX3DNOW    = 0x4,
    MMX3DNOWEXT = 0x8,
    SSE         = 0x10,
    SSE2        = 0x20,
    CMOV        = 0x40,
    IWMMXT      = 0x80
};

extern "C"
{
#ifdef LMMS_HOST_X86
void alignedMemCpyMMX( void * RP _dst, const void * RP _src, int _size );
void alignedMemClearMMX( void * RP _dst, int _size );
#endif
void alignedMemCpySSE( void * RP _dst, const void * RP _src, int _size );
void alignedMemClearSSE( void * RP _dst, int _size );
void alignedBufApplyGainSSE( sampleFrameA * RP _dst, float _gain, int _frames );
void alignedBufMixSSE( sampleFrameA * RP _dst, const sampleFrameA * RP _src, int _frames );
void alignedBufMixLRCoeffSSE( sampleFrameA * RP _dst, const sampleFrameA * RP _src, float _left, float _right, int _frames );
void unalignedBufMixLRCoeffSSE( sampleFrame * RP _dst, const sampleFrame * RP _src, const float _left, const float _right, int _frames );
void alignedBufWetDryMixSSE( sampleFrameA * RP _dst, const sampleFrameA * RP _src, float _wet, float _dry, int _frames );
void alignedBufWetDryMixSplittedSSE( sampleFrameA * RP _dst, const float * RP _left, const float * RP _right, float _wet, float _dry, int _frames );
#ifdef LMMS_HOST_X86
void alignedMemCpySSE2( void * RP _dst, const void * RP _src, int _size );
void alignedMemClearSSE2( void * RP _dst, int _size );
int alignedConvertToS16SSE2( const sampleFrameA * RP _src, intSampleFrameA * RP _dst, const fpp_t _frames, const float _master_gain, const bool _convert_endian );
#endif
} ;
#endif



void initBasicOps( void )
{
#ifdef X86_OPTIMIZATIONS
	static bool extensions_checked = false;
	if( extensions_checked == false )
	{
		int features = 0;
		unsigned int result = 0;
		unsigned int extended_result = 0;
		asm(	"push %%ebx\n"
			"pushf\n"
			"pop %%eax\n"
			"mov %%eax, %%ebx\n"
			"xor $0x00200000, %%eax\n"
			"push %%eax\n"
			"popf\n"
			"pushf\n"
			"pop %%eax\n"
			"xor %%edx, %%edx\n"
			"xor %%ebx, %%eax\n"
			"jz 1f\n"

			"mov $0x00000001, %%eax\n"
			"cpuid\n"
			"1:\n"
			"pop %%ebx\n"
			"mov %%edx, %0\n"

			: "=r" (result)
			:
			: "%eax", "%ecx", "%edx"
		);

		asm(	"push %%ebx\n"
			"pushf\n"
			"pop %%eax\n"
			"mov %%eax, %%ebx\n"
			"xor $0x00200000, %%eax\n"
			"push %%eax\n"
			"popf\n"
			"pushf\n"
			"pop %%eax\n"
			"xor %%edx, %%edx\n"
			"xor %%ebx, %%eax\n"
			"jz 2f\n"

			"mov $0x80000000, %%eax\n"
			"cpuid\n"
			"cmp $0x80000000, %%eax\n"
			"jbe 2f\n"
			"mov $0x80000001, %%eax\n"
			"cpuid\n"
			"2:\n"
			"pop %%ebx\n"
			"mov %%edx, %0\n"

			: "=r" (extended_result)
			:
			: "%eax", "%ecx", "%edx"
		);

		if( result & (1u << 15) )
			features |= CMOV;
		if( result & (1u << 23) )
			features |= MMX;
		if( extended_result & (1u << 22) )
			features |= MMXEXT;
		if( extended_result & (1u << 31) )
			features |= MMX3DNOW;
		if( extended_result & (1u << 30) )
			features |= MMX3DNOWEXT;
		if( result & (1u << 25) )
			features |= SSE;
		if( result & (1u << 26) )
			features |= SSE2;

#ifdef LMMS_HOST_X86
		if( features & MMX )
		{
			alignedMemCpy = alignedMemCpyMMX;
			alignedMemClear = alignedMemClearMMX;
		}
#endif
		if( features & SSE )
		{
			fprintf( stderr, "Using SSE optimized routines\n" );
			alignedMemCpy = alignedMemCpySSE;
			alignedMemClear = alignedMemClearSSE;
			alignedBufApplyGain = alignedBufApplyGainSSE;
			alignedBufMix = alignedBufMixSSE;
			alignedBufMixLRCoeff = alignedBufMixLRCoeffSSE;
			unalignedBufMixLRCoeff = unalignedBufMixLRCoeffSSE;
			alignedBufWetDryMix = alignedBufWetDryMixSSE;
			alignedBufWetDryMixSplitted =
						alignedBufWetDryMixSplittedSSE;
		}
		if( features & SSE2 )
		{
			fprintf( stderr, "Using SSE2 optimized routines\n" );
			alignedMemCpy = alignedMemCpySSE2;
			alignedMemClear = alignedMemClearSSE2;
			alignedConvertToS16 = alignedConvertToS16SSE2;
		}
		extensions_checked = true;
	}
#endif
}



