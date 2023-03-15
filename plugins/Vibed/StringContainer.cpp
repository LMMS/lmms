/*
 * StringContainer.cpp - contains a collection of strings
 *
 * Copyright (c) 2006 Danny McRae <khjklujn/at/yahoo/com>
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

#include "StringContainer.h"

namespace lmms
{


StringContainer::StringContainer(float pitch, sample_rate_t sampleRate, int bufferLength, int strings) :
	m_pitch(pitch),
	m_sampleRate(sampleRate),
	m_bufferLength(bufferLength),
	m_exists(strings, false)
{
}

void StringContainer::addString(int harm, float pick, float pickup, const float* impulse, float randomize,
	float stringLoss, float detune, int oversample, bool state, int id)
{
	float harmFloat = 1.0f;
	switch (harm)
	{
	case 0: harmFloat = 0.25f; break;
	case 1: harmFloat = 0.5f; break;
	case 2: harmFloat = 1.0f; break;
	case 3: harmFloat = 2.0f; break;
	case 4: harmFloat = 3.0f; break;
	case 5: harmFloat = 4.0f; break;
	case 6: harmFloat = 5.0f; break;
	case 7: harmFloat = 6.0f; break;
	case 8: harmFloat = 7.0f; break;
	default: break;
	}

	m_strings.emplace_back(
		m_pitch * harmFloat,
		pick,
		pickup,
		impulse,
		m_bufferLength,
		m_sampleRate,
		oversample,
		randomize,
		stringLoss,
		detune,
		state);

	m_exists[id] = true;
}


} // namespace lmms