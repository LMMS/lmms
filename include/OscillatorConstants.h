/*
 * OscillatorConstants.h - declaration of constants used in Oscillator and SampleBuffer
 * for band limited wave tables
 *
 * Copyright (c) 2018      Dave French	<dave/dot/french3/at/googlemail/dot/com>
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

#ifndef LMMS_OSCILLATORCONSTANTS_H
#define LMMS_OSCILLATORCONSTANTS_H

#include <array>

#include "LmmsTypes.h"

namespace lmms::OscillatorConstants
{

	// Limit wavetables to the audible audio spectrum
	const int MAX_FREQ = 20000;
	// Minimum size of table to have all audible bands for midi note 1 (i.e. 20 000 Hz / 8.176 Hz)
	constexpr int WAVETABLE_LENGTH = static_cast<int>(MAX_FREQ / 8.176);

	//SEMITONES_PER_TABLE, the smaller the value the smoother the harmonics change on frequency sweeps
	// with the trade off of increased memory requirements to store the wave tables
	// require memory = NumberOfWaveShapes*WAVETABLE_LENGTH*(MidiNoteCount/SEMITONES_PER_TABLE)*BytePerSample_t
	// 7*2446*(128/1)*4 = 8766464 bytes
	const int SEMITONES_PER_TABLE = 1;
	const int WAVE_TABLES_PER_WAVEFORM_COUNT = 128 / SEMITONES_PER_TABLE;

	// There is some ambiguity around the use of "wavetable", "wavetable synthesis" or related terms.
	// The following meanings and definitions were selected for use in the Oscillator class:
	//  - wave shape: abstract and precise definition of the graph associated with a given type of wave;
	//  - waveform: digital representations the wave shape, a set of waves optimized for use at varying pitches;
	//  - wavetable: a table containing one period of a wave, with frequency content optimized for a specific pitch.
	using wavetable_t = std::array<sample_t, WAVETABLE_LENGTH>;
	using waveform_t = std::array<wavetable_t, WAVE_TABLES_PER_WAVEFORM_COUNT>;

} // namespace lmms::OscillatorConstants

#endif // LMMS_OSCILLATORCONSTANTS_H
