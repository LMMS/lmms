/*
 * SampleCache.h
 *
 * Copyright (c) 2024 saker <sakertooth@gmail.com>
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

#ifndef LMMS_SAMPLE_CACHE_H
#define LMMS_SAMPLE_CACHE_H

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace lmms {
class SampleBuffer;

/*
 * A cache for `SampleBuffer`.
 * Useful for preventing usage of the same buffer in multiple places.
 */
class SampleCache
{
public:
	/*
	 * A manager that can be added to the `SampleCache` to evict its entries when needed.
	 */
	class Evictor
	{
	public:
		virtual ~Evictor() = default;

	private:
		//! Setup functionality to start evicting entries from the cache.
		virtual void setup(SampleCache& cache) = 0;

		//! Notify to the evictor that an entry has been added to the cache.
		virtual void notifyAdd(const std::string& key) = 0;

		friend class SampleCache;
	};

	//! Add an entry with the given `key` and `buffer` to the cache.
	void add(const std::string& key, std::shared_ptr<const SampleBuffer> buffer);

	//! Remove an entry with the given `key` from the cache if one exists.
	void remove(const std::string& key);

	//! Get an entry with the given `key` from the cache.
	//! Returns `nullopt` if the entry does not exist.
	auto get(const std::string& key) -> std::optional<std::shared_ptr<const SampleBuffer>>;

	//! Checks if an entry with the given `key` exists within the cache.
	auto contains(const std::string& key) -> bool;

	//! Add an evictor to the cache.
	//! Returns a reference to the evictor.
	auto addEvictor(std::unique_ptr<Evictor> evictor) -> const std::unique_ptr<Evictor>&;

	//! Remove an evictor from the cache.
	void removeEvictor(const std::unique_ptr<Evictor>& evictor);

private:
	std::unordered_map<std::string, std::shared_ptr<const SampleBuffer>> m_entries;
	std::vector<std::unique_ptr<Evictor>> m_evictors;
};
} // namespace lmms

#endif // LMMS_SAMPLE_CACHE_H
