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

namespace lmms
{

template<typename T> constexpr T Pi = T(3.14159265358979323846264338327950288419716939937510);
template<typename T> constexpr T Pi2 = T(Pi<T> / 2.0);
template<typename T> constexpr T PiR = T(1.0 / Pi<T>);
template<typename T> constexpr T PiSqr = T(Pi<T> * Pi<T>);
template<typename T> constexpr T Tau = T(Pi<T> * 2.0);
template<typename T> constexpr T TauR = T(1.0 / Tau<T>);
template<typename T> constexpr T E = T(2.71828182845904523536028747135266249775724709369995);
template<typename T> constexpr T ER = T(1.0 / E<T>);
template<typename T> constexpr T Sqrt2 = T(1.41421356237309504880168872420969807856967187537695);

constexpr long double LD_2PI = Tau<long double>;
constexpr long double LD_PI = Pi<long double>;
constexpr double D_PI = Pi<double>;
constexpr double D_2PI = Tau<double>;
constexpr float F_PI = Pi<float>;
constexpr float F_2PI = Tau<float>;
constexpr float F_PI_2 = Pi2<float>;
constexpr float F_PI_SQR = PiSqr<float>;
constexpr float F_E = E<float>;
constexpr float F_SQRT_2 = Sqrt2<float>;

constexpr float F_EPSILON = 1.0e-10f; // 10^-10

// Microtuner
constexpr unsigned int MaxScaleCount = 10;  //!< number of scales per project
constexpr unsigned int MaxKeymapCount = 10; //!< number of keyboard mappings per project

// Frequency ranges (in Hz).
// Arbitrary low limit for logarithmic frequency scale; >1 Hz.
constexpr int LOWEST_LOG_FREQ = 5;

// Full range is defined by LOWEST_LOG_FREQ and current sample rate.
enum class FrequencyRange
{
	Full = 0,
	Audible,
	Bass,
	Mids,
	High
};

constexpr int FRANGE_AUDIBLE_START = 20;
constexpr int FRANGE_AUDIBLE_END = 20000;
constexpr int FRANGE_BASS_START = 20;
constexpr int FRANGE_BASS_END = 300;
constexpr int FRANGE_MIDS_START = 200;
constexpr int FRANGE_MIDS_END = 5000;
constexpr int FRANGE_HIGH_START = 4000;
constexpr int FRANGE_HIGH_END = 20000;

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

constexpr int ARANGE_EXTENDED_START = -80;
constexpr int ARANGE_EXTENDED_END = 20;
constexpr int ARANGE_AUDIBLE_START = -50;
constexpr int ARANGE_AUDIBLE_END = 0;
constexpr int ARANGE_LOUD_START = -30;
constexpr int ARANGE_LOUD_END = 0;
constexpr int ARANGE_SILENT_START = -60;
constexpr int ARANGE_SILENT_END = -10;


} // namespace lmms

#endif // LMMS_CONSTANTS_H
