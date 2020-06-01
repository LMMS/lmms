/*
 * MixHelpers.h - helper functions for mixing buffers
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

#ifndef MIX_HELPERS_H
#define MIX_HELPERS_H

#include "lmms_basics.h"

class ValueBuffer;
namespace MixHelpers
{

bool isSilent( const sampleFrame* src, int frames );

bool useNaNHandler();

void setNaNHandler( bool use );

bool sanitize( sampleFrame * src, int frames );

/*! \brief Add samples from src to dst */
void add( sampleFrame* dst, const sampleFrame* src, int frames );


/*! \brief Add samples from src multiplied by coeffSrc to dst */
void addMultiplied( sampleFrame* dst, const sampleFrame* src, float coeffSrc, int frames );

/*! \brief Add samples from src multiplied by coeffSrc to dst, swap inputs */
void addSwappedMultiplied( sampleFrame* dst, const sampleFrame* src, float coeffSrc, int frames );

/*! \brief Add samples from src multiplied by coeffSrc and coeffSrcBuf to dst */
void addMultipliedByBuffer( sampleFrame* dst, const sampleFrame* src, float coeffSrc, ValueBuffer * coeffSrcBuf, int frames );

/*! \brief Add samples from src multiplied by coeffSrc and coeffSrcBuf to dst */
void addMultipliedByBuffers( sampleFrame* dst, const sampleFrame* src, ValueBuffer * coeffSrcBuf1, ValueBuffer * coeffSrcBuf2, int frames );

/*! \brief Same as addMultiplied, but sanitize output (strip out infs/nans) */
void addSanitizedMultiplied( sampleFrame* dst, const sampleFrame* src, float coeffSrc, int frames );

/*! \brief Add samples from src multiplied by coeffSrc and coeffSrcBuf to dst - sanitized version */
void addSanitizedMultipliedByBuffer( sampleFrame* dst, const sampleFrame* src, float coeffSrc, ValueBuffer * coeffSrcBuf, int frames );

/*! \brief Add samples from src multiplied by coeffSrc and coeffSrcBuf to dst - sanitized version */
void addSanitizedMultipliedByBuffers( sampleFrame* dst, const sampleFrame* src, ValueBuffer * coeffSrcBuf1, ValueBuffer * coeffSrcBuf2, int frames );

/*! \brief Add samples from src multiplied by coeffSrcLeft/coeffSrcRight to dst */
void addMultipliedStereo( sampleFrame* dst, const sampleFrame* src, float coeffSrcLeft, float coeffSrcRight, int frames );

/*! \brief Multiply dst by coeffDst and add samples from src multiplied by coeffSrc */
void multiplyAndAddMultiplied( sampleFrame* dst, const sampleFrame* src, float coeffDst, float coeffSrc, int frames );

/*! \brief Multiply dst by coeffDst and add samples from srcLeft/srcRight multiplied by coeffSrc */
void multiplyAndAddMultipliedJoined( sampleFrame* dst, const sample_t* srcLeft, const sample_t* srcRight, float coeffDst, float coeffSrc, int frames );

}

#endif

