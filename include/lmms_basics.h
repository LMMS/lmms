/*
 * lmms_basics.h - typedefs for common types that are used in the whole app
 *
 * Copyright (c) 2004-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef LMMS_TYPES_H
#define LMMS_TYPES_H

#include <cstddef>

#include "lmmsconfig.h"

#include <cstdint>



namespace lmms
{

using bar_t = int32_t;
using tick_t = int32_t;
using volume_t = uint8_t;
using panning_t = int8_t;

using sample_t = float;		  // standard sample-type
using int_sample_t = int16_t; // 16-bit-int-sample

using sample_rate_t = uint32_t; // sample-rate
using fpp_t = size_t;			// frames per period (0-16384)
using f_cnt_t = size_t;			// standard frame-count
using ch_cnt_t = uint8_t;		// channel-count (0-DEFAULT_CHANNELS)
using bpm_t = uint16_t;			// tempo (MIN_BPM to MAX_BPM)
using bitrate_t = uint16_t;		// bitrate in kbps
using mix_ch_t = uint16_t;		// Mixer-channel (0 to MAX_CHANNEL)

using jo_id_t = uint32_t; // (unique) ID of a journalling object


constexpr ch_cnt_t DEFAULT_CHANNELS = 2;

constexpr char LADSPA_PATH_SEPERATOR =
#ifdef LMMS_BUILD_WIN32
';';
#else
':';
#endif



#define LMMS_STRINGIFY(s) LMMS_STR(s)
#define LMMS_STR(PN)	#PN

} // namespace lmms

#endif // LMMS_TYPES_H
