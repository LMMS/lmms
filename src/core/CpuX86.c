/*
 * cpu_x86.c - x86 specific optimized operations
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

#include "Cpu.h"

#define PREFETCH_RW(x,rw)								\
			__builtin_prefetch(x,rw,0);					\
			__builtin_prefetch(x+128/sizeof(*x),rw,0);	\
			__builtin_prefetch(x+256/sizeof(*x),rw,0);	\
			__builtin_prefetch(x+384/sizeof(*x),rw,0);

#if 0	/* for benchmarking only */
#undef PREFETCH_RW
#define PREFETCH_RW(x,rw)
#endif
#define PREFETCH_READ(x)	PREFETCH_RW(x,0)
#define PREFETCH_WRITE(x)	PREFETCH_RW(x,1)

/* workaround for conflicting declarations in GCC and MinGW headers */
#define _aligned_malloc __aligned_malloc
#define _aligned_free __aligned_free

#ifdef X86_OPTIMIZATIONS

#ifdef BUILD_MMX

#include <mmintrin.h>

void memCpyMMX( void * RP _dst, const void * RP _src, int _size )
{
	const int s = _size / ( sizeof( __m64 ) * 8 );
	int i;
	char fpu_save[108];
	char * RP src = (char *) _src;
	char * RP dst = (char *) _dst;
	__asm__ __volatile__ ( " fsave %0; fwait\n"::"m"(fpu_save[0]) );

	PREFETCH_READ(src);
	PREFETCH_WRITE(dst);

	for( i=0; i<s; ++i )
	{
		__asm__ __volatile__ (
		"1: prefetchnta 320(%0)\n"
		"2: movq (%0), %%mm0\n"
		"   movq 8(%0), %%mm1\n"
		"   movq 16(%0), %%mm2\n"
		"   movq 24(%0), %%mm3\n"
		"   movq %%mm0, (%1)\n"
		"   movq %%mm1, 8(%1)\n"
		"   movq %%mm2, 16(%1)\n"
		"   movq %%mm3, 24(%1)\n"
		"   movq 32(%0), %%mm0\n"
		"   movq 40(%0), %%mm1\n"
		"   movq 48(%0), %%mm2\n"
		"   movq 56(%0), %%mm3\n"
		"   movq %%mm0, 32(%1)\n"
		"   movq %%mm1, 40(%1)\n"
		"   movq %%mm2, 48(%1)\n"
		"   movq %%mm3, 56(%1)\n"
		: : "r" (src), "r" (dst) : "memory");
		src+=64;
		dst+=64;
	}
	__asm__ __volatile__ ( " fsave %0; fwait\n"::"m"(fpu_save[0]) );


}



void memClearMMX( void * RP _dst, int _size )
{
	__m64 * dst = (__m64 *) _dst;
	const int s = _size / ( sizeof( *dst ) * 8 );
	__m64 val = _mm_setzero_si64();
	int i;

	PREFETCH_WRITE(dst);

	for( i = 0; i < s; ++i )
	{
		__asm__ __volatile__ (
			"movq    %0, (%1)\n"
			"movq    %0, 8(%1)\n"
			"movq    %0, 16(%1)\n"
			"movq    %0, 24(%1)\n"
			"movq    %0, 32(%1)\n"
			"movq    %0, 40(%1)\n"
			"movq    %0, 48(%1)\n"
			"movq    %0, 56(%1)\n"
				: : "y" (val), "r" (dst) : "memory" );
		dst += 8;
	}
	_mm_empty();
}

#endif


#ifdef BUILD_SSE

#include <xmmintrin.h>

void memCpySSE( void * RP _dst, const void * RP _src, int _size )
{
	__m128 * dst = (__m128 *) _dst;
	__m128 * src = (__m128 *) _src;
	const int s = _size / ( sizeof( *dst ) * 4 );
	int i;

	PREFETCH_READ(src);
	PREFETCH_WRITE(dst);

	for( i = 0; i < s; ++i )
	{
		dst[0] = src[0];
		dst[1] = src[1];
		dst[2] = src[2];
		dst[3] = src[3];
		src += 4;
		dst += 4;
	}
}




void memClearSSE( void * RP _dst, int _size )
{
	__m128 * dst = (__m128 *) _dst;
	const int s = _size / ( sizeof( *dst ) * 4 );
	__m128 val = _mm_setzero_ps();
	int i;

	PREFETCH_WRITE(dst);

	for( i = 0; i < s; ++i )
	{
		dst[0] = val;
		dst[1] = val;
		dst[2] = val;
		dst[3] = val;
		dst += 4;
	}
}




void bufApplyGainSSE( sampleFrameA * RP _dst, float _gain, int _frames )
{
	int i;

	PREFETCH_WRITE(_dst);

	for( i = 0; i < _frames; )
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




void bufMixSSE( sampleFrameA * RP _dst, const sampleFrameA * RP _src,
								int _frames )
{
	int i;

	PREFETCH_READ(_src);
	PREFETCH_WRITE(_dst);

	for( i = 0; i < _frames; )
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


void bufMixCoeffSSE( sampleFrameA * RP _dst, const sampleFrameA * RP _src,
								float _coeff, int _frames )
{
	int i;

	PREFETCH_READ(_src);
	PREFETCH_WRITE(_dst);

	for( i = 0; i < _frames; )
	{
		_dst[i+0][0] += _src[i+0][0]*_coeff;
		_dst[i+0][1] += _src[i+0][1]*_coeff;
		_dst[i+1][0] += _src[i+1][0]*_coeff;
		_dst[i+1][1] += _src[i+1][1]*_coeff;
		_dst[i+2][0] += _src[i+2][0]*_coeff;
		_dst[i+2][1] += _src[i+2][1]*_coeff;
		_dst[i+3][0] += _src[i+3][0]*_coeff;
		_dst[i+3][1] += _src[i+3][1]*_coeff;
		i += 4;
	}
}


void bufMixLRCoeffSSE( sampleFrameA * RP _dst,
					const sampleFrameA * RP _src,
					float _left, float _right, int _frames )
{
	int i;

	PREFETCH_READ(_src);
	PREFETCH_WRITE(_dst);

	for( i = 0; i < _frames; )
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



void unalignedBufMixLRCoeffSSE( sampleFrame * RP _dst, const sampleFrame * RP _src,
							const float _left,
							const float _right,
								int _frames )
{
	int i;
	if( unlikely( _frames % 2 ) )
	{
		_dst[0][0] += _src[0][0] * _left;
		_dst[0][1] += _src[0][1] * _right;
		++_src;
		++_dst;
		--_frames;
	}

	PREFETCH_READ(_src);
	PREFETCH_WRITE(_dst);

	for( i = 0; i < _frames; )
	{
		_dst[i+0][0] += _src[i+0][0]*_left;
		_dst[i+0][1] += _src[i+0][1]*_right;
		_dst[i+1][0] += _src[i+1][0]*_left;
		_dst[i+1][1] += _src[i+1][1]*_right;
		i += 2;
	}
}



void bufWetDryMixSSE( sampleFrameA * RP _dst,
					const sampleFrameA * RP _src,
					float _wet, float _dry, int _frames )
{
	int i;

	PREFETCH_READ(_src);
	PREFETCH_WRITE(_dst);

	for( i = 0; i < _frames; )
	{
		_dst[i+0][0] = _dst[i+0][0]*_dry + _src[i+0][0]*_wet;
		_dst[i+0][1] = _dst[i+0][1]*_dry + _src[i+0][1]*_wet;
		_dst[i+1][0] = _dst[i+1][0]*_dry + _src[i+1][0]*_wet;
		_dst[i+1][1] = _dst[i+1][1]*_dry + _src[i+1][1]*_wet;
		_dst[i+2][0] = _dst[i+2][0]*_dry + _src[i+2][0]*_wet;
		_dst[i+2][1] = _dst[i+2][1]*_dry + _src[i+2][1]*_wet;
		_dst[i+3][0] = _dst[i+3][0]*_dry + _src[i+3][0]*_wet;
		_dst[i+3][1] = _dst[i+3][1]*_dry + _src[i+3][1]*_wet;
		i += 4;
	}
}




void bufWetDryMixSplittedSSE( sampleFrameA * RP _dst,
					const float * RP _left,
					const float * RP _right,
					float _wet, float _dry, int _frames )
{
	int i;

	PREFETCH_READ(_left);
	PREFETCH_READ(_right);
	PREFETCH_WRITE(_dst);

	for( i = 0; i < _frames; )
	{
		_dst[i+0][0] = _dst[i+0][0]*_dry + _left[i+0]*_wet;
		_dst[i+0][1] = _dst[i+0][1]*_dry + _right[i+0]*_wet;
		_dst[i+1][0] = _dst[i+1][0]*_dry + _left[i+1]*_wet;
		_dst[i+1][1] = _dst[i+1][1]*_dry + _right[i+1]*_wet;
		i += 2;
	}
}



#endif


#ifdef BUILD_SSE2

#include <emmintrin.h>

void memCpySSE2( void * RP _dst, const void * RP _src, int _size )
{
	__m128i * dst = (__m128i *) _dst;
	__m128i * src = (__m128i *) _src;
	const int s = _size / ( sizeof( *dst ) * 4 );
	int i;

	PREFETCH_READ(src);
	PREFETCH_WRITE(dst);

	for( i = 0; i < s; ++i )
	{
		_mm_store_si128( dst+0, _mm_load_si128( src+0 ) );
		_mm_store_si128( dst+1, _mm_load_si128( src+1 ) );
		_mm_store_si128( dst+2, _mm_load_si128( src+2 ) );
		_mm_store_si128( dst+3, _mm_load_si128( src+3 ) );
		src += 4;
		dst += 4;
	}
}




void memClearSSE2( void * RP _dst, int _size )
{
	__m128i * dst = (__m128i *) _dst;
	const int s = _size / ( sizeof( *dst ) * 4 );
	__m128i val = _mm_setzero_si128();
	int i;

	PREFETCH_WRITE(dst);

	for( i = 0; i < s; ++i )
	{
		_mm_store_si128( dst+0, val );
		_mm_store_si128( dst+1, val );
		_mm_store_si128( dst+2, val );
		_mm_store_si128( dst+3, val );
		dst += 4;
	}
}



int convertToS16SSE2( const sampleFrameA * RP _src,
					intSampleFrameA * RP _dst,
					const fpp_t _frames,
					const float _master_gain,
					const bool _convert_endian )
{
	int t1;
	int t2;
	fpp_t frame;
	const float f = _master_gain * OUTPUT_SAMPLE_MULTIPLIER;

	PREFETCH_READ(_src);
	PREFETCH_WRITE(_dst);

	if( _convert_endian )
	{
		for( frame = 0; frame < _frames; ++frame )
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
		for( frame = 0; frame < _frames; ++frame )
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



#endif

#endif
