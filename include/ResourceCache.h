/*
 * ResourceCache.h
 *
 * Copyright (c) 2025 Sotonye Atemie <sakertooth@gmail.com>
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

#ifndef LMMS_RESOURCE_CACHE_H
#define LMMS_RESOURCE_CACHE_H

#include <algorithm>
#include <memory>
#include <vector>

namespace lmms {
class ResourceCache
{
public:
	static constexpr auto CacheSize = 64;

	class Resource
	{
	public:
		virtual bool invalid() = 0;
		virtual void update() = 0;
	};

	template <typename T, typename... Args> static auto fetch(Args&&... args) -> std::shared_ptr<const T>
	{
		auto key = typeid(T).name();
		((key += "_" + std::to_string(std::hash<std::decay_t<Args>>{}(args))), ...);

		const auto it
			= std::find_if(s_resources.begin(), s_resources.end(), [](const auto& entry) { return entry.key == key; });

		if (it == s_resources.end())
		{
			if (s_resources.size() == CacheSize)
			{
				const auto entry = std::min_element(s_resources.begin(), s_resources.end(),
					[](const auto& first, const auto& second) { return first.age < second.age; });
				s_resources.erase(entry);
			}

			const auto resource = std::make_shared<T>(std::forward<Args>(args)...);
			s_resources.emplace_back(std::move(key), resource, 0);
			return resource;
		}

		const auto resource = it->resource;
		if (resource->invalid()) { resource->update(); }
		++resource->age;

		return resource;
	}

	static auto instance() -> ResourceCache&
	{
		static auto s_inst = ResourceCache{};
		return s_inst;
	}

private:
	struct Entry
	{
		std::string key;
		std::shared_ptr<Resource> resource;
		int age;
	};

	ResourceCache() { s_resources.reserve(CacheSize); }
	inline static std::vector<Entry> s_resources;
};
} // namespace lmms

#endif // LMMS_RESOURCE_CACHE_H
