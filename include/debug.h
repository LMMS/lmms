/*
 * debug.h - header file to be included for debugging purposes
 *
 * Copyright (c) 2004-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef LMMS_DEBUG_H
#define LMMS_DEBUG_H

#include "lmmsconfig.h"

// Define standard macro NDEBUG when building without debug flag to make sure asserts become no-ops.
#ifndef LMMS_DEBUG
#ifndef NDEBUG
	#define NDEBUG
#endif
#endif // LMMS_DEBUG

#include <cassert>
#include <cstdio>


#endif // LMMS_DEBUG_H
