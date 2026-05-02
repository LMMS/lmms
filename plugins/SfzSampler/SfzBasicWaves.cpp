/*
 * SfzBasicWaves.cpp - Generators of basic wave shapes
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

#include "SfzBasicWaves.h"
#include "lmms_math.h"
#include <numbers>

namespace lmms
{

const float SfzBasicWaves::generate(Shape shape, float sampleRate, int frame)
{
	// Amount through period
	// By default, the basic waves are played at C4 (midi key 60). This isn't in the spec afaik, but it makes things simple since pitch_keycenter defaults to 60.
	constexpr float baseFreq = 261.625565301f;
	const float t = absFraction(frame * baseFreq / sampleRate);
	// The region playback state handles incrementing the frame count at different rates based on the pitch, so we don't need to handle the actual frequency calculation here.

	switch (shape)
	{
	case Shape::Sine:
		return sine(t);
	case Shape::Saw:
		return saw(t);
	case Shape::Square:
		return square(t);
	case Shape::Triangle:
		return triangle(t);
	case Shape::Noise:
		return noise(t);
	case Shape::Silence:
		return 0.0f;
	}
	return 0.0f;
}


const float SfzBasicWaves::sine(float t)
{
	return sin(t * 2 * std::numbers::pi);
}

const float SfzBasicWaves::saw(float t)
{
	return 2.0f * t - 1.0f;
}

const float SfzBasicWaves::square(float t)
{
	return t > 0.5 ? 1.0f : -1.0f;
}

const float SfzBasicWaves::triangle(float t)
{
	return t < 0.5f
		? 4.0f * t - 1.0f
		: -(4.0f * (t - 0.5f) - 1.0f);
}

const float SfzBasicWaves::noise(float t)
{
	return fastRand(-1.0f, 1.0f);
}


} // namespace lmms
