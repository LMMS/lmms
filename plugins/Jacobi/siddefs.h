#define VERSION "0.16"

//  ---------------------------------------------------------------------------
//  This file is part of reSID, a MOS6581 SID emulator engine.
//  Copyright (C) 1999  Dag Lem <resid@nimrod.no>
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//  ---------------------------------------------------------------------------

#ifndef __SIDDEFS_H__
#define __SIDDEFS_H__

// Define bool, true, and false for C++ compilers that lack these keywords.
#define RESID_HAVE_BOOL 1

#if !RESID_HAVE_BOOL
typedef int bool;
const bool true = 1;
const bool false = 0;
#endif

// We could have used the smallest possible data type for each SID register,
// however this would give a slower engine because of data type conversions.
// An int is assumed to be at least 32 bits (necessary in the types reg24,
// cycle_count, and sound_sample). GNU does not support 16-bit machines
// (GNU Coding Standards: Portability between CPUs), so this should be
// a valid assumption.

typedef unsigned int reg4;
typedef unsigned int reg8;
typedef unsigned int reg12;
typedef unsigned int reg16;
typedef unsigned int reg24;

typedef int cycle_count;
typedef int sound_sample;
typedef sound_sample fc_point[2];

enum chip_model { MOS6581, MOS8580 };

enum sampling_method { SAMPLE_FAST, SAMPLE_INTERPOLATE,
		       SAMPLE_RESAMPLE_INTERPOLATE, SAMPLE_RESAMPLE_FAST };

extern "C"
{
#ifndef __VERSION_CC__
extern const char* resid_version_string;
#else
const char* resid_version_string = VERSION;
#endif
}

// Inlining on/off.
#define RESID_INLINING 1
#define RESID_INLINE inline

#endif // not __SIDDEFS_H__
