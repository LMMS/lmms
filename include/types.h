/*
 * types.h - typedefs for common types that are used in the whole app
 *
 * Copyright (c) 2004-2006 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifndef _TYPES_H
#define _TYPES_H

typedef unsigned char	Uint8;
typedef signed char	Sint8;
typedef unsigned short	Uint16;
typedef signed short	Sint16;
typedef unsigned int	Uint32;
typedef signed int	Sint32;


typedef Uint32 minute;
typedef Sint8 second;
typedef Sint32 tact;
typedef Sint8 tact64th;
typedef Uint8 volume;
typedef Sint8 panning;


typedef float sample_t;			// standard sample-type
typedef Sint16 int_sample_t;		// 16-bit-int-sample


typedef Uint32 sample_rate_t;		// sample-rate
typedef Sint16 fpab_t;			// frames per audio-buffer (0-16384)
typedef Sint32 f_cnt_t;			// standard frame-count
typedef Uint8 ch_cnt_t;			// channel-count (0-SURROUND_CHANNELS)
typedef Uint16 bpm_t;			// tempo (MIN_BPM to MAX_BPM)
typedef Uint16 bitrate_t;		// bitrate in kbps
typedef Sint8 fx_ch_t;			// FX-channel (0 to MAX_EFFECT_CHANNEL)

typedef Uint32 jo_id_t;			// (unique) ID of a journalling object


template<typename T>
struct valueRanges
{
	enum
	{
		max = static_cast<T>( static_cast<T>( ~0 ) > 0 ?
			~0 : ( ( ( T ) 1 << ( sizeof( T ) * 8 - 1 ) ) - 1 ) ),
		min = static_cast<T>( static_cast<T>( ~0 ) > 0 ?
							0 : ( -max - 1 ) )
	} ;
} ;


#endif
