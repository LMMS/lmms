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

	template <typename V, typename K,
		typename Enable
		= std::enable_if_t<std::conjunction_v<std::is_base_of<Resource, V>, std::is_default_constructible<V>>>,
		typename... Args>
	static auto fetch(const K& key, Args&&... args) -> std::shared_ptr<const V>
	{
		const auto digest = hash(key);
		if (digest.empty()) { return std::make_shared<V>(); }

		auto& resource = s_resources[digest];

		if (resource == nullptr)
		{
			if (s_resources.size() == CacheSize)
			{
				const auto it = std::min_element(s_resources.begin(), s_resources.end(),
					[](const auto& first, const auto& second) { return first.second->m_age < first.second->m_age; });
				s_resources.erase(it);
			}

			try
			{
				resource = std::make_shared<V>(key, std::forward<Args>(args)...);
			}
			catch (const std::runtime_error& error)
			{
				return std::make_shared<V>();
			}

			return std::static_pointer_cast<const V>(resource);
		}

		++resource->m_age;
		return std::static_pointer_cast<const V>(resource);
	}

	static auto instance() -> ResourceCache&
	{
		static auto s_inst = ResourceCache{};
		return s_inst;
	}

private:
	static auto hash(const std::filesystem::path& path) -> std::string
	{
		if (!std::filesystem::exists(path)) { return std::string{}; }

		static constexpr auto blockSize = 8192;
		static auto block = std::array<char, blockSize>{};
		static auto hash = QCryptographicHash{QCryptographicHash::Sha256};

		auto fstream = std::fstream{path, std::ios::in | std::ios::binary};
		if (!fstream.is_open()) { return std::string{}; }

		do
		{
			fstream.read(block.data(), blockSize);
			hash.addData(block.data(), fstream.gcount());
		} while (fstream.gcount() > 0);

		return hash.result().toStdString();
	}

	static auto hash(const std::string& key) -> std::string
	{
		const auto data = QByteArray::fromStdString(key);
		const auto hash = QCryptographicHash::hash(data, QCryptographicHash::Sha256).toStdString();
		return hash;
	}

	ResourceCache() { s_resources.reserve(CacheSize); }
	inline static std::unordered_map<std::string, std::shared_ptr<Resource>> s_resources;
};
} // namespace lmms

#endif // LMMS_RESOURCE_CACHE_H
