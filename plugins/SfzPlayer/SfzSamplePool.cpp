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

// Default initialize the static shared pool pointer with no parameters, essentially like passing nullptr. s_instance.expired() will be true at the start.
std::weak_ptr<SfzSamplePool> SfzSamplePool::s_instance = std::weak_ptr<SfzSamplePool>();

const std::shared_ptr<SfzSamplePool> SfzSamplePool::instance()
{
	if (s_instance.expired())
	{
		// Return the shared_ptr to the SfzPlayer, but keep a weak_ptr copy so that we can make more shared_ptr copies to anyone who asks later, but it won't prevent it from being automatically deleted when all the SfzPlayers are destroyed.
		std::shared_ptr<SfzSamplePool> inst = std::make_shared<SfzSamplePool>();
		s_instance = std::weak_ptr<SfzSamplePool>(inst);
		return inst;
	}
	else
	{
		return std::shared_ptr<SfzSamplePool>(s_instance);
	}
}


const int SfzSamplePool::sampleMemoryUsage() const
{
	int total = 0;
	for (const auto& [key, value] : m_samplePool)
	{
		if (!value.expired())
		{
			const int sizeOfSampleData = value.lock()->size() * SfzSampleBuffer::NUM_CHANNELS * sizeof(float);
			const int sizeOfSampleBufferObject = sizeof(SfzSampleBuffer);
			total += sizeOfSampleData + sizeOfSampleBufferObject;
		}
	}
	return total;
}


const std::shared_ptr<SfzSampleBuffer> SfzSamplePool::loadSample(const QString& path, bool* sampleInPool)
{
	// If the sample has already been loaded before, return a shared pointer to it
	if (m_samplePool.contains(path) && !m_samplePool.at(path).expired())
	{
		*sampleInPool = true;
		return m_samplePool.at(path).lock();
	}
	else if (auto buffer = SampleBuffer::fromFile(path))
	{
		// Otherwise, load the sample file and create a shared_ptr. Store a weak_ptr in the sample pool so that it can be freely deleted when no more SfzPlayers are using it.
		*sampleInPool = false;
		auto sampleSharedPtr = std::make_shared<SfzSampleBuffer>(buffer->data(), buffer->size(), buffer->sampleRate());
		m_samplePool.insert({path, std::weak_ptr<SfzSampleBuffer>(sampleSharedPtr)});
		return sampleSharedPtr;
	}
	else
	{
		*sampleInPool = false;
		return nullptr;
	}
}

void SfzSamplePool::clearExpiredWeakPtrs()
{
	std::erase_if(m_samplePool, [](const auto& entry){
		const auto& [key, value] = entry;
		return value.expired();
	});
}


} // namespace lmms

