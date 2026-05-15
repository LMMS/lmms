/*
 * SfzSamplePool.cpp - Helper class for handling loading of sample files
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

#include "SfzSamplePool.h"
#include "SampleBuffer.h"

namespace lmms
{


const SfzSampleBuffer* SfzSamplePool::loadSample(const QString& path)
{
	// If the sample has already been loaded before, just return a pointer to it
	if (m_samplePool.contains(path))
	{
		return m_samplePool.at(path).get();
	}
	else if (auto buffer = SampleBuffer::fromFile(path))
	{
		m_samplePool.insert({path, std::make_unique<SfzSampleBuffer>(buffer->data(), buffer->size(), buffer->sampleRate())});
		return m_samplePool.at(path).get();
	}
	else
	{
		return nullptr;
	}
}


} // namespace lmms

