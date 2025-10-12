/*
 * panning.h - declaration of some types, concerning the
 *             panning of a note
 *
 * Copyright (c) 2004-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef LMMS_PANNING_H
#define LMMS_PANNING_H

#include "LmmsTypes.h"
#include "Midi.h"
#include "volume.h"

#include <cmath>

namespace lmms
{

inline constexpr panning_t PanningRight = 100;
inline constexpr panning_t PanningLeft = -PanningRight;
inline constexpr panning_t PanningCenter = 0;
inline constexpr panning_t DefaultPanning = PanningCenter;

inline StereoVolumeVector panningToVolumeVector( panning_t _p,
							float _scale = 1.0f )
{
	StereoVolumeVector v = { { _scale, _scale } };
	const float pf = _p / 100.0f;
	v.vol[_p >= PanningCenter ? 0 : 1] *= 1.0f - std::abs(pf);
	return v;
}


inline int panningToMidi( panning_t _p )
{
	return MidiMinPanning + (int) (
			  ( (float)( _p - PanningLeft ) ) /
			  ( (float)( PanningRight - PanningLeft ) ) *
			  ( (float)( MidiMaxPanning - MidiMinPanning ) ) );
}


} // namespace lmms

#endif // LMMS_PANNING_H
