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

#include <QCryptographicHash>
#include <array>
#include <filesystem>
#include <fstream>
#include <memory>
#include <unordered_map>

namespace lmms {
class ResourceCache
{
public:
	static constexpr auto CacheSize = 32;

	class Resource
	{
		int m_age = 0;
		friend class ResourceCache;
	};

	template <typename T, typename _Enable = std::enable_if<std::is_base_of_v<Resource, T>>, typename... Args>
	static auto fetch(const std::filesystem::path& key, Args&&... args) -> std::shared_ptr<const Resource>
	{
		if (!std::filesystem::exists(key)) { return nullptr; }

		auto fstream = std::fstream{key, std::ios::binary};

		static constexpr auto blockSize = 4096;
		auto block = std::array<char, blockSize>{};
		auto hash = QCryptographicHash{QCryptographicHash::Sha256};

		while (!fstream.eof())
		{
			fstream.read(block.data(), blockSize);
			hash.addData(block.data(), blockSize);
		}

		const auto digest = hash.result().toStdString();
		auto& resource = s_resources[digest];

		if (resource == nullptr)
		{
			if (s_resources.size() == CacheSize) { evict(); }
			resource = std::make_shared<T>(key, std::forward<Args>(args)...);
			return resource;
		}

		++resource->m_age;
		return resource;
	}

	template <typename T, typename... Args>
	static auto fetch(const std::string& key, Args&&... args) -> std::shared_ptr<const Resource>
	{
		const auto data = QByteArray::fromStdString(key);
		const auto digest = QCryptographicHash::hash(data, QCryptographicHash::Sha256).toStdString();
		auto& resource = s_resources[digest];

		if (resource == nullptr)
		{
			if (s_resources.size() == CacheSize) { evict(); }
			resource = std::make_shared<T>(key, std::forward<Args>(args)...);
			return resource;
		}

		++resource->m_age;
		return resource;
	}

	static auto instance() -> ResourceCache&
	{
		static auto s_inst = ResourceCache{};
		return s_inst;
	}

private:
	static void evict()
	{
		const auto it = std::min_element(s_resources.begin(), s_resources.end(),
			[](const auto& first, const auto& second) { return first.second->m_age < first.second->m_age; });
		s_resources.erase(it);
	}

	ResourceCache() { s_resources.reserve(CacheSize); }
	inline static std::unordered_map<std::string, std::shared_ptr<Resource>> s_resources;
};
} // namespace lmms

#endif // LMMS_RESOURCE_CACHE_H
