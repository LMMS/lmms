/*
 * FileResourceCache.h
 *
 * Copyright (c) 2026 saker <sakertooth@gmail.com>
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

#ifndef LMMS_FILE_RESOURCE_CACHE_H
#define LMMS_FILE_RESOURCE_CACHE_H

#include <filesystem>
#include <memory>
#include <type_traits>
#include <unordered_map>

namespace lmms {

template <typename T> class FileResourceCache
{
public:
	template <typename Factory>
		requires(std::is_invocable_r_v<std::shared_ptr<const T>, Factory>)
	auto fetch(const std::filesystem::path& path, Factory&& factory) -> std::shared_ptr<const T>
	{
		auto [it, inserted] = m_entries.try_emplace(path, Entry{});
		auto& entry = it->second;
		const auto lastWriteTime = std::filesystem::last_write_time(path);

		if (!inserted && entry.lastWriteTime == lastWriteTime)
		{
			const auto resource = entry.resource.lock();
			if (resource) { return resource; }
		}

		const auto resource = factory();
		if (!resource) { return nullptr; }

		entry.resource = resource;
		entry.lastWriteTime = lastWriteTime;
		return resource;
	}

private:
	struct Entry
	{
		std::weak_ptr<const T> resource;
		std::filesystem::file_time_type lastWriteTime;
	};

	std::unordered_map<std::filesystem::path, Entry> m_entries;
};
} // namespace lmms

#endif // LMMS_FILE_RESOURCE_CACHE_H
