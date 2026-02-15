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

#ifdef LMMS_DEBUG
#include <iostream>
#endif

#include <algorithm>
#include <cmath>

#include "ValueBuffer.h"
#include "SampleFrame.h"



static bool s_NaNHandler;


namespace lmms::MixHelpers
{

/*! \brief Function for applying MIXOP on all sample frames */
template<typename MIXOP>
static inline void run( SampleFrame* dst, const SampleFrame* src, int frames, const MIXOP& OP )
{
	for( int i = 0; i < frames; ++i )
	{
		OP( dst[i], src[i] );
	}
}

/*! \brief Function for applying MIXOP on all sample frames - split source */
template<typename MIXOP>
static inline void run( SampleFrame* dst, const sample_t* srcLeft, const sample_t* srcRight, int frames, const MIXOP& OP )
{
	for( int i = 0; i < frames; ++i )
	{
		const SampleFrame src = { srcLeft[i], srcRight[i] };
		OP( dst[i], src );
	}
}



bool isSilent( const SampleFrame* src, int frames )
{
	const float silenceThreshold = 0.0000001f;

	for( int i = 0; i < frames; ++i )
	{
		if (std::abs(src[i][0]) >= silenceThreshold || std::abs(src[i][1]) >= silenceThreshold)
		{
			return false;
		}
	}

	return true;
}

bool useNaNHandler()
{
	return s_NaNHandler;
}

void setNaNHandler( bool use )
{
	s_NaNHandler = use;
}

bool sanitize(std::span<sample_t> buffer)
{
	if (!useNaNHandler()) { return false; }

	for (std::size_t f = 0; f < buffer.size(); ++f)
	{
		sample_t& sample = buffer[f];
		if (std::isinf(sample) || std::isnan(sample))
		{
#ifdef LMMS_DEBUG
			std::cerr << "Bad data, clearing buffer. frame: "
				<< f << ", value: " << sample << "\n";
#endif

			// Clear the channel if a problem is found
			std::ranges::fill(buffer, 0.f);

			return true;
		}
		else
		{
			sample = std::clamp(sample, sample_t(-1000.0), sample_t(1000.0));
		}
	}
	return false;
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


void add(PlanarBufferView<sample_t> dst, PlanarBufferView<const sample_t> src)
{
	assert(dst.channels() == src.channels());
	assert(dst.frames() == src.frames());

	const auto channels = dst.channels();
	const auto frames = dst.frames();
	for (proc_ch_t channel = 0; channel < channels; ++channel)
	{
		auto* dstPtr = dst.bufferPtr(channel);
		const auto* srcPtr = src.bufferPtr(channel);
		for (f_cnt_t frame = 0; frame < frames; ++frame)
		{
			dstPtr[frame] += srcPtr[frame];
		}
	}
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

void addSanitizedMultipliedByBuffer( SampleFrame* dst, const SampleFrame* src, float coeffSrc, ValueBuffer * coeffSrcBuf, int frames )
{
	if ( !useNaNHandler() )
	{
		addMultipliedByBuffer( dst, src, coeffSrc, coeffSrcBuf,
								frames );
		return;
	}

	for( int f = 0; f < frames; ++f )
	{
		dst[f][0] += ( std::isinf( src[f][0] ) || std::isnan( src[f][0] ) ) ? 0.0f : src[f][0] * coeffSrc * coeffSrcBuf->values()[f];
		dst[f][1] += ( std::isinf( src[f][1] ) || std::isnan( src[f][1] ) ) ? 0.0f : src[f][1] * coeffSrc * coeffSrcBuf->values()[f];
	}
}

void addSanitizedMultipliedByBuffers( SampleFrame* dst, const SampleFrame* src, ValueBuffer * coeffSrcBuf1, ValueBuffer * coeffSrcBuf2, int frames )
{
	if ( !useNaNHandler() )
	{
		addMultipliedByBuffers( dst, src, coeffSrcBuf1, coeffSrcBuf2,
								frames );
		return;
	}

	for( int f = 0; f < frames; ++f )
	{
		dst[f][0] += ( std::isinf( src[f][0] ) || std::isnan( src[f][0] ) )
			? 0.0f
			: src[f][0] * coeffSrcBuf1->values()[f] * coeffSrcBuf2->values()[f];
		dst[f][1] += ( std::isinf( src[f][1] ) || std::isnan( src[f][1] ) )
			? 0.0f
			: src[f][1] * coeffSrcBuf1->values()[f] * coeffSrcBuf2->values()[f];
	}

}


struct AddSanitizedMultipliedOp
{
	AddSanitizedMultipliedOp( float coeff ) : m_coeff( coeff ) { }

	void operator()( SampleFrame& dst, const SampleFrame& src ) const
	{
		dst[0] += ( std::isinf( src[0] ) || std::isnan( src[0] ) ) ? 0.0f : src[0] * m_coeff;
		dst[1] += ( std::isinf( src[1] ) || std::isnan( src[1] ) ) ? 0.0f : src[1] * m_coeff;
	}

	const float m_coeff;
};

void addSanitizedMultiplied( SampleFrame* dst, const SampleFrame* src, float coeffSrc, int frames )
{
	if ( !useNaNHandler() )
	{
		addMultiplied( dst, src, coeffSrc, frames );
		return;
	}

	run<>( dst, src, frames, AddSanitizedMultipliedOp(coeffSrc) );
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

