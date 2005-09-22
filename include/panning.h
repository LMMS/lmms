/*
 * panning.h - declaration of some constants and types, concerning the
 *             panning of a note
 *
 * Linux MultiMedia Studio
 * Copyright (c) 2004-2005 Tobias Doerffel <tobydox@users.sourceforge.net>
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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#ifndef _PANNING_H
#define _PANNING_H

#include "types.h"

const panning PANNING_LEFT = ( 0 - 100);
const panning PANNING_RIGHT = ( 0 + 100);
const panning PANNING_CENTER = 0;
const panning DEFAULT_PANNING = PANNING_CENTER;

#endif
