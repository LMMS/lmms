/*
 * OscillatorConstants.h - declaration of constants used in  Oscillator and SampleBuffer
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

#ifndef OSCILLATORCONSTANTS_H
#define OSCILLATORCONSTANTS_H

class OscillatorConstants
{
public:
	static const int WAVETABLE_LENGTH = 2446; //minimum size of table to have all bands for midi note 1
	static const int MAX_FREQ = 20000; //limit to the audio spectrum

	//SEMITONES_PER_TABLE, the smaller the value the smoother the harmonics change on frequency sweeps
	// with the trade off of increased memory requirements to store the wave tables
	// require memory = NumberOfWaveShapes*WAVETABLE_LENGTH*(MidiNoteCount/SEMITONES_PER_TABLE)*BytePerSample_t
	// 7*2446*(128/1)*4 = 8766464 bytes
	static const int SEMITONES_PER_TABLE = 1;
	static const int WAVE_TABLES_PER_WAVEFORM_COUNT = 128 / SEMITONES_PER_TABLE;

};


#endif // OSCILLATORCONSTANTS_H
