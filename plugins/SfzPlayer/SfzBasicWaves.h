/*
 * SfzBasicWaves.h - Generators for audio wave shapes such as sine, saw, triangle, noise, etc
 *
 * Copyright (c) 2026 Keratin
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

#ifndef LMMS_SFZ_BASIC_WAVES_H
#define LMMS_SFZ_BASIC_WAVES_H

namespace lmms
{

class SfzBasicWaves
{
public:
	enum class Shape
	{
		Sine,
		Saw,
		Square,
		Triangle,
		Noise,
		Silence
	};
	//! Returns the calculated value of the waveform shape at the given number of frames from the start, given the sample rate.
	static float generate(Shape shape, float sampleRate, int frame);

private:
	static float sine(float t);
	static float saw(float t);
	static float square(float t);
	static float triangle(float t);
	static float noise(float t);
};

} // namespace lmms

#endif // LMMS_SFZ_BASIC_WAVES_H
