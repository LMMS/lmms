/*
 * lmms_constants.h - defines system constants
 *
 * Copyright (c) 2006 Danny McRae <khjklujn/at/users.sourceforge.net>
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

#ifndef LMMS_CONSTANTS_H
#define LMMS_CONSTANTS_H

#include "lmmsconfig.h"
#include "LmmsTypes.h"

namespace lmms
{
// Prefer using `approximatelyEqual()` from lmms_math.h rather than
// using this directly
inline constexpr float F_EPSILON = 1.0e-10f; // 10^-10

inline constexpr ch_cnt_t DEFAULT_CHANNELS = 2;

// Microtuner
inline constexpr unsigned MaxScaleCount = 10;  //!< number of scales per project
inline constexpr unsigned MaxKeymapCount = 10; //!< number of keyboard mappings per project

constexpr char LADSPA_PATH_SEPERATOR =
#ifdef LMMS_BUILD_WIN32
';';
#else
':';
#endif

} // namespace lmms

#endif // LMMS_CONSTANTS_H
