/*
 * MixHelpers.cpp - helper functions for mixing buffers
 *
 * Copyright (c) 2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include "MixHelpers.h"

#include <algorithm>
#include <cmath>

#include "ValueBuffer.h"
#include "SampleFrame.h"
#include "SimdHelpers.h"

namespace lmms::MixHelpers
{

namespace {

constexpr auto SilenceThreshold = 0.000001f; // -120 dBFS

/*! \brief Function for applying MIXOP on all sample frames */
template<typename MIXOP>
inline void run(SampleFrame* dst, const SampleFrame* src, int frames, const MIXOP& OP)
{
	for( int i = 0; i < frames; ++i )
	{
		OP( dst[i], src[i] );
	}
}

/*! \brief Function for applying MIXOP on all sample frames - split source */
template<typename MIXOP>
inline void run(SampleFrame* dst, const sample_t* srcLeft, const sample_t* srcRight, int frames, const MIXOP& OP)
{
	for( int i = 0; i < frames; ++i )
	{
		const SampleFrame src = { srcLeft[i], srcRight[i] };
		OP( dst[i], src );
	}
}

} // namespace

bool isSilent(const SampleFrame* src, int frames)
{
	return isSilent({&src[0][0], static_cast<std::size_t>(frames * 2)});
}

bool isSilent(std::span<const sample_t> buffer)
{
	return std::ranges::all_of(buffer, [&](const sample_t s) { return std::abs(s) < SilenceThreshold; });
}

struct AddOp
{
	void operator()( SampleFrame& dst, const SampleFrame& src ) const
	{
		dst += src;
	}
} ;

void add( SampleFrame* dst, const SampleFrame* src, int frames )
{
	run<>( dst, src, frames, AddOp() );
}

namespace {

void addScalar(float* const* LMMS_RESTRICT dst, const float* const* LMMS_RESTRICT src,
	ch_cnt_t channels, f_cnt_t frames)
{
	for (ch_cnt_t channel = 0; channel < channels; ++channel)
	{
		float* const dstPtr = dst[channel];
		const float* const srcPtr = src[channel];
		for (f_cnt_t frame = 0; frame < frames; ++frame)
		{
			dstPtr[frame] += srcPtr[frame];
		}
	}
}

#if defined(LMMS_HOST_X86_64)

LMMS_SIMD_BEGIN_IMPL

template<std::uint8_t lanes>
void addSimd(float* const* LMMS_RESTRICT dst, const float* const* LMMS_RESTRICT src,
	ch_cnt_t channels, f_cnt_t frames)
{
	constexpr std::size_t mask = lanes - 1;

	const f_cnt_t alignedFrames = frames - (frames & mask);
	for (ch_cnt_t channel = 0; channel < channels; ++channel)
	{
		float* const dstPtr = dst[channel];
		const float* const srcPtr = src[channel];

		for (f_cnt_t frame = 0; frame < alignedFrames; frame += lanes)
		{
			_mmX_storeu_ps<lanes>(dstPtr + frame,
				_mmX_add_ps<lanes>(
					_mmX_loadu_ps<lanes>(dstPtr + frame),
					_mmX_loadu_ps<lanes>(srcPtr + frame)
				)
			);
		}

		for (f_cnt_t frame = alignedFrames; frame < frames; ++frame)
		{
			dstPtr[frame] += srcPtr[frame];
		}
	}
}

template LMMS_FUNC_TARGET_AVX
void addSimd<8>(float* const* LMMS_RESTRICT dst, const float* const* LMMS_RESTRICT src,
	ch_cnt_t channels, f_cnt_t frames);

template LMMS_FUNC_TARGET_AVX512F
void addSimd<16>(float* const* LMMS_RESTRICT dst, const float* const* LMMS_RESTRICT src,
	ch_cnt_t channels, f_cnt_t frames);

LMMS_SIMD_END_IMPL

#endif // LMMS_HOST_X86_64

} // namespace

void add(PlanarBufferView<sample_t> dst, PlanarBufferView<const sample_t> src)
{
	assert(dst.channels() == src.channels());
	assert(dst.frames() == src.frames());

	static auto dispatcher = SimdDispatcher {
			// TODO: Can use CTAD and noexcept functions after we upgrade to MSVC 19.51 which fixes a conformance bug
			SimdDispatchConfig<false, void, float* const*, const float* const*, ch_cnt_t, f_cnt_t> {
#if defined(LMMS_HOST_X86_64) && !defined(_MSC_VER)
			.avx512f = addSimd<16>,
			.avx     = addSimd<8>,
#endif
			.scalar  = addScalar,
			.preferAutoVectorization = true
		}
	};

	dispatcher(dst.data(), src.data(), dst.channels(), dst.frames());
}


struct AddMultipliedOp
{
	AddMultipliedOp( float coeff ) : m_coeff( coeff ) { }

	void operator()( SampleFrame& dst, const SampleFrame& src ) const
	{
		dst += src * m_coeff;
	}

	const float m_coeff;
} ;


void addMultiplied( SampleFrame* dst, const SampleFrame* src, float coeffSrc, int frames )
{
	run<>( dst, src, frames, AddMultipliedOp(coeffSrc) );
}


struct AddSwappedMultipliedOp
{
	AddSwappedMultipliedOp( float coeff ) : m_coeff( coeff ) { }

	void operator()( SampleFrame& dst, const SampleFrame& src ) const
	{
		dst[0] += src[1] * m_coeff;
		dst[1] += src[0] * m_coeff;
	}

	const float m_coeff;
};

void multiply(SampleFrame* dst, float coeff, int frames)
{
	for (int i = 0; i < frames; ++i)
	{
		dst[i] *= coeff;
	}
}

void addSwappedMultiplied( SampleFrame* dst, const SampleFrame* src, float coeffSrc, int frames )
{
	run<>( dst, src, frames, AddSwappedMultipliedOp(coeffSrc) );
}


void addMultipliedByBuffer( SampleFrame* dst, const SampleFrame* src, float coeffSrc, ValueBuffer * coeffSrcBuf, int frames )
{
	for( int f = 0; f < frames; ++f )
	{
		dst[f][0] += src[f][0] * coeffSrc * coeffSrcBuf->values()[f];
		dst[f][1] += src[f][1] * coeffSrc * coeffSrcBuf->values()[f];
	}
}

void addMultipliedByBuffers( SampleFrame* dst, const SampleFrame* src, ValueBuffer * coeffSrcBuf1, ValueBuffer * coeffSrcBuf2, int frames )
{
	for( int f = 0; f < frames; ++f )
	{
		dst[f][0] += src[f][0] * coeffSrcBuf1->values()[f] * coeffSrcBuf2->values()[f];
		dst[f][1] += src[f][1] * coeffSrcBuf1->values()[f] * coeffSrcBuf2->values()[f];
	}

}

struct AddMultipliedStereoOp
{
	AddMultipliedStereoOp( float coeffLeft, float coeffRight )
	{
		m_coeffs[0] = coeffLeft;
		m_coeffs[1] = coeffRight;
	}

	void operator()( SampleFrame& dst, const SampleFrame& src ) const
	{
		dst[0] += src[0] * m_coeffs[0];
		dst[1] += src[1] * m_coeffs[1];
	}

	std::array<float, 2> m_coeffs;
} ;


void addMultipliedStereo( SampleFrame* dst, const SampleFrame* src, float coeffSrcLeft, float coeffSrcRight, int frames )
{

	run<>( dst, src, frames, AddMultipliedStereoOp(coeffSrcLeft, coeffSrcRight) );
}





struct MultiplyAndAddMultipliedOp
{
	MultiplyAndAddMultipliedOp( float coeffDst, float coeffSrc )
	{
		m_coeffs[0] = coeffDst;
		m_coeffs[1] = coeffSrc;
	}

	void operator()( SampleFrame& dst, const SampleFrame& src ) const
	{
		dst[0] = dst[0]*m_coeffs[0] + src[0]*m_coeffs[1];
		dst[1] = dst[1]*m_coeffs[0] + src[1]*m_coeffs[1];
	}

	std::array<float, 2> m_coeffs;
} ;


void multiplyAndAddMultiplied( SampleFrame* dst, const SampleFrame* src, float coeffDst, float coeffSrc, int frames )
{
	run<>( dst, src, frames, MultiplyAndAddMultipliedOp(coeffDst, coeffSrc) );
}



void multiplyAndAddMultipliedJoined( SampleFrame* dst,
										const sample_t* srcLeft,
										const sample_t* srcRight,
										float coeffDst, float coeffSrc, int frames )
{
	run<>( dst, srcLeft, srcRight, frames, MultiplyAndAddMultipliedOp(coeffDst, coeffSrc) );
}

} // namespace lmms::MixHelpers

