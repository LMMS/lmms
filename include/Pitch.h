/*
 * Pitch.h - declaration of some constants and types concerning instrument pitch
 *
 * Copyright (c) 2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 *
 * This file is part of LMMS - http://lmms.io
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

#ifndef _PITCH_H
#define _PITCH_H

#include "lmms_basics.h"
#include "Midi.h"

typedef int16_t pitch_t;

const pitch_t CentsPerSemitone = 100;
const pitch_t MinPitchDefault = -CentsPerSemitone;
const pitch_t MaxPitchDefault = CentsPerSemitone;
const pitch_t DefaultPitch = 0;

#endif
