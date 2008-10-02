/*
 * panning.h - declaration of some constants and types, concerning the
 *             panning of a note
 *
 * Copyright (c) 2004-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef _PANNING_H
#define _PANNING_H

#include "types.h"
#include "volume.h"
#include "templates.h"

const panning PanningRight = ( 0 + 100 );
const panning PanningLeft = - PanningRight;
const panning PanningCenter = 0;
const panning DefaultPanning = PanningCenter;

inline stereoVolumeVector panningToVolumeVector( panning _p,
							float _scale = 1.0f )
{
	stereoVolumeVector v = { { _scale, _scale } };
	const float pf = _p / 100.0f;
	v.vol[_p >= PanningCenter ? 0 : 1] *= 1.0f - qAbs<float>( pf );
	return v;
}

#endif
