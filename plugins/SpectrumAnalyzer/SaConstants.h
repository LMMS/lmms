/*
 * SaConstants.h - Some constants used in spectrum analyser
 *
 * Copyright (c) 2025 Roshan M R (Ross Maxx) <mrroshan127/at/gmail/dot/com>
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

#ifndef LMMS_SA_CONSTANTS_H
#define LMMS_SA_CONSTANTS_H

namespace lmms
{

// Frequency ranges (in Hz).
// Arbitrary low limit for logarithmic frequency scale; >1 Hz.
inline constexpr auto LowestLogFreq = 5;

// Full range is defined by LowestLogFreq and current sample rate.
enum class FrequencyRange
{
	Full = 0,
	Audible,
	Bass,
	Mids,
	High
};

inline constexpr auto FrequencyRangeAudibleStart =    20;
inline constexpr auto FrequencyRangeAudibleEnd   = 20000;
inline constexpr auto FrequencyRangeBassStart    =    20;
inline constexpr auto FrequencyRangeBassEnd      =   300;
inline constexpr auto FrequencyRangeMidsStart    =   200;
inline constexpr auto FrequencyRangeMidsEnd      =  5000;
inline constexpr auto FrequencyRangeHighStart    =  4000;
inline constexpr auto FrequencyRangeHighEnd      = 20000;

// Amplitude ranges (in dBFS).
// Reference: full scale sine wave (-1.0 to 1.0) is 0 dB.
// Doubling or halving the amplitude produces 3 dB difference.
enum class AmplitudeRange
{
	Extended = 0,
	Audible,
	Loud,
	Silent
};

inline constexpr auto AmplitudeRangeExtendedStart = -80;
inline constexpr auto AmplitudeRangeExtendedEnd   =  20;
inline constexpr auto AmplitudeRangeAudibleStart  = -50;
inline constexpr auto AmplitudeRangeAudibleEnd    =   0;
inline constexpr auto AmplitudeRangeLoudStart     = -30;
inline constexpr auto AmplitudeRangeLoudEnd       =   0;
inline constexpr auto AmplitudeRangeSilentStart   = -60;
inline constexpr auto AmplitudeRangeSilentEnd     = -10;

} // namespace lmms

#endif // LMMS_SA_CONSTANTS_H
