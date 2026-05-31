/*
 * SfzSamplePool.h - Helper class for handling loading of sample files
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

#ifndef LMMS_SFZ_SAMPLE_POOL_H
#define LMMS_SFZ_SAMPLE_POOL_H

#include "SfzSampleBuffer.h"

#include <QString>
#include <map>

namespace lmms
{

class SfzSamplePool
{
public:
	//! Returns a shared_ptr to the global sample pool instance, shared across all SfzPlayers.
	//! If no sample pool has been created yet, or the previous one was deleted because all active SfzPlayer were closed, it will initialize a new one
	static const std::shared_ptr<SfzSamplePool> instance();

	//! Attempts to load the sample from the given path. It checks if it has already been loaded into the sample pool, otherwise it will load it from disk.
	//! Sets `sampleInPool` to true if the sample was already loaded, false if not.
	//! Returns a std::shard_ptr to the sample object if successful. Returns nullptr if it could not load the sample from disk.
	const std::shared_ptr<SfzSampleBuffer> loadSample(const QString& path, bool* sampleInPool);

	//! Returns the number of samples currently loaded in the pool
	const int sampleCount() const { return m_samplePool.size(); }

	//! Returns the total number of bytes used by the sample objects. Technically this might not be the total memory used, depending on padding or extra minor objects I missed, but it should be the bulk of the data.
	const int sampleMemoryUsage() const;

	//! Removes any samples pointers from the buffer which no longer exist. Because the pool stores them as weak_ptrs, they may have been deleted when a previous SfzPlayer was deleted, but not removed from the pool. This method goes through and cleans it up so that only active samples remain.
	void clearExpiredWeakPtrs();

private:
	//! Stores a list of std::weak_ptrs to the SfzSampleBuffer objects for each loaded sample. Weak pointers are used so that the pool can be shared among multiple instances of the SfzPlayer, each one's regions holding a std::shared_ptr, which when all destroyed, allow the object to be automatically deleted.
	std::map<QString, std::weak_ptr<SfzSampleBuffer>> m_samplePool;

	//! Global pointer to the sample pool instance shared across all SfzPlayers.
	//! This is a weak_ptr, but the individual SfzPlayers will use shared_ptrs of it, so that when the last SfzPlayer is deleted, the sample pool will also be deleted.
	static std::weak_ptr<SfzSamplePool> s_instance;
};

} // namespace lmms

#endif // LMMS_SFZ_SAMPLE_POOL_H
