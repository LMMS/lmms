/*
 * PresetDatabase.cpp - Preset discovery, loading, storage, and query
 *
 * Copyright (c) 2024 Dalton Messmer <messmer.dalton/at/gmail.com>
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

#include "PresetDatabase.h"

#include <algorithm>
#include <cassert>
#include <filesystem>

#include "ConfigManager.h"
#include "PathUtil.h"

namespace lmms
{

PresetDatabase::PresetDatabase()
	: m_recentPresetFile{ConfigManager::inst()->userPresetsDir().toStdString()}
{
}

auto PresetDatabase::discover() -> bool
{
	if (!discoverSetup()) { return false; }
	if (!discoverFiletypes(m_filetypes)) { return false; }

	auto func = SetLocations{m_presets};
	if (!discoverLocations(func)) { return false; }

	bool success = false;
	for (auto& [location, presets] : m_presets)
	{
		success = discoverPresets(location, presets) || success;
	}

	return success;
}

auto PresetDatabase::loadPresets(std::string_view file) -> std::vector<const Preset*>
{
	m_recentPresetFile = file;

	auto& [location, presets] = *getLocation(file);
	return loadPresets(location, file, presets);
}

auto PresetDatabase::loadPresets(const Location& location, std::string_view file, std::set<Preset>& presets)
	-> std::vector<const Preset*>
{
	// This is the default method - plugins should override this

	auto preset = Preset{};
	preset.loadData() = { std::string{file}, "" };

	preset.metadata().displayName = PathUtil::pathToString(PathUtil::u8path(file).filename());

	auto [it, added] = presets.emplace(std::move(preset));
	if (!added) { return {}; }

	return { &*it };
}

auto PresetDatabase::findPresets(std::string_view key) const -> std::vector<const Preset*>
{
	std::vector<const Preset*> ret;
	for (const auto& mapPair : m_presets)
	{
		for (const auto& preset : mapPair.second)
		{
			if (preset.supportsPlugin(key))
			{
				ret.push_back(&preset);
			}
		}
	}
	return ret;
}

auto PresetDatabase::findPreset(const PresetLoadData& loadData, std::string_view key) const -> const Preset*
{
	if (auto it = m_presets.find(loadData.location); it != m_presets.end())
	{
		auto it2 = std::find_if(it->second.begin(), it->second.end(), [&](const Preset& p) {
			return p.loadData().loadKey == loadData.loadKey && p.supportsPlugin(key);
		});
		return it2 != it->second.end() ? &*it2 : nullptr; // TODO: Is it2.base() standard?
	}
	return nullptr;
}

auto PresetDatabase::findOrLoadPresets(std::string_view file) -> std::vector<const Preset*>
{
	m_recentPresetFile = file;

	auto& [location, presets] = *getLocation(file);
	loadPresets(location, file, presets);

	std::vector<const Preset*> results;
	results.reserve(presets.size());
	for (const auto& preset : presets)
	{
		results.push_back(&preset);
	}
	return results;
}

auto PresetDatabase::getLocation(std::string_view path, bool add) -> PresetMap::iterator
{
	auto isSubpath = [](std::string_view path, std::string_view base) -> bool {
		const auto mismatchPair = std::mismatch(path.begin(), path.end(), base.begin(), base.end());
		return mismatchPair.second == base.end();
	};

	// First, create shortened path
	const auto shortPath = PathUtil::toShortestRelative(path);

	// Next, find the preset directory it belongs to (if any)
	std::vector<PresetMap::iterator> matches;
	for (auto it = m_presets.begin(); it != m_presets.end(); ++it)
	{
		const auto& location = it->first.location;
		if (isSubpath(shortPath, location))
		{
			matches.push_back(it);
		}
	}

	if (!matches.empty())
	{
		// Location already exists - return the longest (most specific) directory
		return *std::max_element(matches.begin(), matches.end(),
			[](PresetMap::iterator a, PresetMap::iterator b) {
				return a->first.location.size() < b->first.location.size();
			});
	}

	// Else, need to add new location
	if (!add) { return m_presets.end(); }

	// Use parent directory
	const auto parentPath = PathUtil::u8path(PathUtil::toAbsolute(path).value()).parent_path().u8string();
	auto newLocation = Location {
		.name = {},
		.location = PathUtil::toShortestRelative(PathUtil::toStdStringView(parentPath)), // directory
		.flags = PresetMetadata::Flag::UserContent // assume unknown directories are user content
	};

	return m_presets.emplace(std::move(newLocation), std::set<Preset>{}).first;
}

auto PresetDatabase::presets(std::string_view location) const -> const std::set<Preset>*
{
	const auto it = m_presets.find(location);
	return it != m_presets.end() ? &it->second : nullptr;
}

auto PresetDatabase::presets(std::string_view location) -> std::set<Preset>*
{
	const auto it = m_presets.find(location);
	return it != m_presets.end() ? &it->second : nullptr;
}

void PresetDatabase::SetLocations::operator()(const std::vector<Location>& locations) const
{
	for (auto& location : locations)
	{
		m_map->emplace(location, std::set<Preset>{});
	}
}

void PresetDatabase::SetLocations::operator()(Location location) const
{
	m_map->emplace(std::move(location), std::set<Preset>{});
}

} // namespace lmms
