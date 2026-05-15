/*
 * SfzSampleBuffer.cpp - Custom sample data class
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

#include "SfzSampleBuffer.h"
#include "interpolation.h"

namespace lmms
{


SfzSampleBuffer::SfzSampleBuffer(const std::vector<SampleFrame>& data, const float sampleRate)
	: m_data(new float[data.size() * NUM_CHANNELS])
	, m_size(data.size())
	, m_sampleRate(sampleRate)
{
	for (f_cnt_t f = 0; f < m_size; ++f)
	{
		for (size_t channel = 0; channel < NUM_CHANNELS; ++channel)
		{
			m_data[f * NUM_CHANNELS + channel] = data[f][channel];
		}
	}
}

SfzSampleBuffer::~SfzSampleBuffer()
{
	delete[] m_data;
}


float SfzSampleBuffer::at(const float index, const size_t channel) const
{
	if (index < 0 || index >= m_size) { return 0.0f; }

	const f_cnt_t indexFloor = static_cast<f_cnt_t>(index);

	float frac = index - indexFloor;

	f_cnt_t i0 = indexFloor == 0 ? 0 : indexFloor - 1;
	f_cnt_t i1 = indexFloor;
	f_cnt_t i2 = std::min(indexFloor + 1, m_size - 1);
	f_cnt_t i3 = std::min(indexFloor + 2, m_size - 1);
	
	float v0 = m_data[NUM_CHANNELS * i0 + channel];
	float v1 = m_data[NUM_CHANNELS * i1 + channel];
	float v2 = m_data[NUM_CHANNELS * i2 + channel];
	float v3 = m_data[NUM_CHANNELS * i3 + channel];

	return hermiteInterpolate(v0, v1, v2, v3, frac);
}


} // namespace lmms
