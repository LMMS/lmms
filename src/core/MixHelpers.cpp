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
#include "lmms_math.h"
#include "ValueBuffer.h"


namespace MixHelpers
{

/*! \brief Function for applying MIXOP on all sample frames */
template<typename MIXOP>
static inline void run( sampleFrame* dst, const sampleFrame* src, int frames, const MIXOP& OP )
{
	for( int i = 0; i < frames; ++i )
	{
		OP( dst[i], src[i] );
	}
}

/*! \brief Function for applying MIXOP on all sample frames - split source */
template<typename MIXOP>
static inline void run( sampleFrame* dst, const sample_t* srcLeft, const sample_t* srcRight, int frames, const MIXOP& OP )
{
	for( int i = 0; i < frames; ++i )
	{
		const sampleFrame src = { srcLeft[i], srcRight[i] };
		OP( dst[i], src );
	}
}



bool isSilent( const sampleFrame* src, int frames )
{
	const float silenceThreshold = 0.0000001f;

	for( int i = 0; i < frames; ++i )
	{
		if( fabsf( src[i][0] ) >= silenceThreshold || fabsf( src[i][1] ) >= silenceThreshold )
		{
			return false;
		}
	}

	return true;
}


/*! \brief Function for sanitizing a buffer of infs/nans - returns true if those are found */
bool sanitize( sampleFrame * src, int frames )
{
	bool found = false;
	for( int f = 0; f < frames; ++f )
	{
		for( int c = 0; c < 2; ++c )
		{
			if( isinff( src[f][c] ) || isnanf( src[f][c] ) )
			{
				src[f][c] = 0.0f;
				found = true;
			}
		}
	}
	return found;
}


struct AddOp
{
	void operator()( sampleFrame& dst, const sampleFrame& src ) const
	{
		dst[0] += src[0];
		dst[1] += src[1];
	}
} ;

void add( sampleFrame* dst, const sampleFrame* src, int frames )
{
	run<>( dst, src, frames, AddOp() );
}



struct AddMultipliedOp
{
	AddMultipliedOp( float coeff ) : m_coeff( coeff ) { }

	void operator()( sampleFrame& dst, const sampleFrame& src ) const
	{
		dst[0] += src[0] * m_coeff;
		dst[1] += src[1] * m_coeff;
	}

	const float m_coeff;
} ;


void addMultiplied( sampleFrame* dst, const sampleFrame* src, float coeffSrc, int frames )
{
	run<>( dst, src, frames, AddMultipliedOp(coeffSrc) );
}


struct AddSwappedMultipliedOp
{
	AddSwappedMultipliedOp( float coeff ) : m_coeff( coeff ) { }

	void operator()( sampleFrame& dst, const sampleFrame& src ) const
	{
		dst[0] += src[1] * m_coeff;
		dst[1] += src[0] * m_coeff;
	}

	const float m_coeff;
};

void addSwappedMultiplied( sampleFrame* dst, const sampleFrame* src, float coeffSrc, int frames )
{
	run<>( dst, src, frames, AddSwappedMultipliedOp(coeffSrc) );
}


void addMultipliedByBuffer( sampleFrame* dst, const sampleFrame* src, float coeffSrc, ValueBuffer * coeffSrcBuf, int frames )
{
	for( int f = 0; f < frames; ++f )
	{
		dst[f][0] += src[f][0] * coeffSrc * coeffSrcBuf->values()[f];
		dst[f][1] += src[f][1] * coeffSrc * coeffSrcBuf->values()[f];
	}
}

void addMultipliedByBuffers( sampleFrame* dst, const sampleFrame* src, ValueBuffer * coeffSrcBuf1, ValueBuffer * coeffSrcBuf2, int frames )
{
	for( int f = 0; f < frames; ++f )
	{
		dst[f][0] += src[f][0] * coeffSrcBuf1->values()[f] * coeffSrcBuf2->values()[f];
		dst[f][1] += src[f][1] * coeffSrcBuf1->values()[f] * coeffSrcBuf2->values()[f];
	}

}

void addSanitizedMultipliedByBuffer( sampleFrame* dst, const sampleFrame* src, float coeffSrc, ValueBuffer * coeffSrcBuf, int frames )
{
	for( int f = 0; f < frames; ++f )
	{
		dst[f][0] += ( isinff( src[f][0] ) || isnanf( src[f][0] ) ) ? 0.0f : src[f][0] * coeffSrc * coeffSrcBuf->values()[f];
		dst[f][1] += ( isinff( src[f][1] ) || isnanf( src[f][1] ) ) ? 0.0f : src[f][1] * coeffSrc * coeffSrcBuf->values()[f];
	}
}

void addSanitizedMultipliedByBuffers( sampleFrame* dst, const sampleFrame* src, ValueBuffer * coeffSrcBuf1, ValueBuffer * coeffSrcBuf2, int frames )
{
	for( int f = 0; f < frames; ++f )
	{
		dst[f][0] += ( isinff( src[f][0] ) || isnanf( src[f][0] ) )
			? 0.0f
			: src[f][0] * coeffSrcBuf1->values()[f] * coeffSrcBuf2->values()[f];
		dst[f][1] += ( isinff( src[f][1] ) || isnanf( src[f][1] ) )
			? 0.0f
			: src[f][1] * coeffSrcBuf1->values()[f] * coeffSrcBuf2->values()[f];
	}

}


struct AddSanitizedMultipliedOp
{
	AddSanitizedMultipliedOp( float coeff ) : m_coeff( coeff ) { }

	void operator()( sampleFrame& dst, const sampleFrame& src ) const
	{
		dst[0] += ( isinff( src[0] ) || isnanf( src[0] ) ) ? 0.0f : src[0] * m_coeff;
		dst[1] += ( isinff( src[1] ) || isnanf( src[1] ) ) ? 0.0f : src[1] * m_coeff;
	}

	const float m_coeff;
};

void addSanitizedMultiplied( sampleFrame* dst, const sampleFrame* src, float coeffSrc, int frames )
{
	run<>( dst, src, frames, AddSanitizedMultipliedOp(coeffSrc) );
}



struct AddMultipliedStereoOp
{
	AddMultipliedStereoOp( float coeffLeft, float coeffRight )
	{
		m_coeffs[0] = coeffLeft;
		m_coeffs[1] = coeffRight;
	}

	void operator()( sampleFrame& dst, const sampleFrame& src ) const
	{
		dst[0] += src[0] * m_coeffs[0];
		dst[1] += src[1] * m_coeffs[1];
	}

	float m_coeffs[2];
} ;


void addMultipliedStereo( sampleFrame* dst, const sampleFrame* src, float coeffSrcLeft, float coeffSrcRight, int frames )
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

	void operator()( sampleFrame& dst, const sampleFrame& src ) const
	{
		dst[0] = dst[0]*m_coeffs[0] + src[0]*m_coeffs[1];
		dst[1] = dst[1]*m_coeffs[0] + src[1]*m_coeffs[1];
	}

	float m_coeffs[2];
} ;


void multiplyAndAddMultiplied( sampleFrame* dst, const sampleFrame* src, float coeffDst, float coeffSrc, int frames )
{
	run<>( dst, src, frames, MultiplyAndAddMultipliedOp(coeffDst, coeffSrc) );
}



void multiplyAndAddMultipliedJoined( sampleFrame* dst,
										const sample_t* srcLeft,
										const sample_t* srcRight,
										float coeffDst, float coeffSrc, int frames )
{
	run<>( dst, srcLeft, srcRight, frames, MultiplyAndAddMultipliedOp(coeffDst, coeffSrc) );
}

}

