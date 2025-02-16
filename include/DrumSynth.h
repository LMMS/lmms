/*
 * DrumSynth.h - DrumSynth DS file renderer
 *
 * Copyright (c) 1998-2000 Paul Kellett (mda-vst.com)
 * Copyright (c) 2007 Paul Giblock <drfaygo/at/gmail.com>
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

#ifndef LMMS_DRUM_SYNTH_H
#define LMMS_DRUM_SYNTH_H

#include <stdint.h>

#include "LmmsTypes.h"

class QString;

namespace lmms {

class DrumSynth
{
public:
	DrumSynth() = default;
	int GetDSFileSamples(QString dsfile, int16_t*& wave, int channels, sample_rate_t Fs);

private:
	float LoudestEnv();
	int LongestEnv();
	void UpdateEnv(int e, long t);
	void GetEnv(int env, const char* sec, const char* key, QString ini);

	float waveform(float ph, int form);

	int GetPrivateProfileString(
		const char* sec, const char* key, const char* def, char* buffer, int size, QString file);
	int GetPrivateProfileInt(const char* sec, const char* key, int def, QString file);
	float GetPrivateProfileFloat(const char* sec, const char* key, float def, QString file);
};

} // namespace lmms

#endif // LMMS_DRUM_SYNTH_H
