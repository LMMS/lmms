/*
 * FileCache.h
 *
 * Copyright (c) 2025 saker <sakertooth@gmail.com>
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

#ifndef LMMS_FILE_CACHE_H
#define LMMS_FILE_CACHE_H

#include <algorithm>
#include <filesystem>
#include <unordered_map>

namespace lmms {

template <typename T>
class FileCache
{
public:
	auto get(const std::filesystem::path& path) -> std::shared_ptr<const T>
	{
		const auto latestWriteTime = std::filesystem::last_write_time(path);
		auto& entry = m_cache[path];

		if (entry.resource && latestWriteTime == entry.time)
		{
			++s_nextAge;
            ++entry.age;
			return entry.resource;
		}

		if (m_cache.size() == Capacity) { evict(); }

		entry.resource = std::make_shared<T>(path);
		entry.time = latestWriteTime;
		entry.age = s_nextAge++;
		return entry.resource;
	}

private:
	void evict()
	{
		const auto it = std::min_element(m_cache.begin(), m_cache.end(),
			[&](const auto& first, const auto& second) { return first.second.age < second.second.age; });
        if (it != m_cache.end()) { m_cache.erase(it); }
	}

	struct Entry
	{
		std::shared_ptr<T> resource;
		std::filesystem::file_time_type time;
		int age = 0;

        friend bool operator==(const Entry& first, const Entry& second)
        {
			return first.resource == second.resource && first.time == second.time && first.age == second.age;
		}
	};

	struct Hash
	{
		std::size_t operator()(const std::filesystem::path& path) const
		{
			return std::filesystem::hash_value(path);
		}
	};

	std::unordered_map<std::filesystem::path, Entry, Hash> m_cache;
    inline static int s_nextAge = 0;
	static constexpr auto Capacity = 32;
};
} // namespace lmms

#endif // LMMS_FILE_CACHE_H
