/*
 * StringContainer.h - contains a collection of strings
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

#ifndef LMMS_STRING_CONTAINER_H
#define LMMS_STRING_CONTAINER_H

#include <vector>
#include <memory>

#include "VibratingString.h"
#include "MemoryManager.h"

namespace lmms
{


class StringContainer
{
	MM_OPERATORS
public:
	StringContainer(float pitch, sample_rate_t sampleRate, int bufferLength, int strings = 9);
	~StringContainer() = default;

	void addString(int harm, float pick, float pickup, const float* impulse, float randomize,
		float stringLoss, float detune, int oversample, bool state, int id);

	bool exists(int id) const { return m_exists[id]; }

	sample_t getStringSample(int str)
	{
		return m_strings[str].nextSample();
	}

private:
	std::vector<VibratingString> m_strings;
	const float m_pitch;
	const sample_rate_t m_sampleRate;
	const int m_bufferLength;
	std::vector<bool> m_exists;
};


} // namespace lmms

#endif // LMMS_STRING_CONTAINER_H
