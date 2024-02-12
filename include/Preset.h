/*
 * Preset.h - Classes for handling plugin presets
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

#ifndef LMMS_PRESET_H
#define LMMS_PRESET_H

#include <map>
#include <string>
#include <string_view>
#include <vector>

#include "NoCopyNoMove.h"
#include "Plugin.h"

namespace lmms
{

//! Contains all information needed for loading a preset associated with a specific plugin
struct PresetLoadData
{
	/**
	 * A string that can be parsed by PathUtil.
	 * Its meaning depends on the plugin, but it typically represents a preset directory.
	 *
	 * Valid examples:
	 * - "internal:" (for presets stored within the plugin's DSO)
	 * - "userprojects:"
	 * - "preset:MyPresetFolder" (TODO: disallow? Makes openPresetFile() difficult to implement)
	 * - "/my/absolute/directory"
	 * - "preset:my_plugin/my_preset_database.txt" (depending on the plugin, it could even be a file!)
	 *
	 * This is a string_view which refers to a key within `PluginPresets::m_presets` in order to save space.
	 */
	std::string_view location;

	/**
	 * Could be a preset filename, sub-path + preset filename, file offset, or any other kind of unique ID.
	 * Again, it's entirely up to the plugin.
	*/
	std::string loadKey;

	// Whether `location` and `loadKey` can be concatenated together
	// to form a single string depends entirely on the plugin.

	friend auto operator==(const PresetLoadData& lhs, const PresetLoadData& rhs) -> bool
	{
		return lhs.location == rhs.location && lhs.loadKey == rhs.loadKey;
	}
};

//! Stores metadata for use by the preset browser and for categorizing the preset
struct PresetMetadata
{
	std::string category; // TODO: Use vector?
	std::string creator;
	std::string description;

	// [...]
};

//! Generic preset
struct Preset
{
	std::string displayName;
	PresetLoadData loadData;
	PresetMetadata metadata;

	const Plugin::Descriptor::SubPluginFeatures::Key* key = nullptr;

	//! TODO: Some plugins may cache preset data for faster loading; empty = uncached
	//std::string data;
};

//! A fixed-size plugin-specific collection of presets
//! Plugins must implement the `populate` logic.
//template<class T = Preset>
class PluginPresets : public NoCopyNoMove
{
public:
	using PresetMap = std::map<std::string, std::vector<Preset>, std::less<>>;

	PluginPresets() = default;
	//PluginPresets(const Plugin::Descriptor::SubPluginFeatures::Key* key);
	virtual ~PluginPresets() = default;

	auto presets() const -> auto& { return m_presets; }

	auto presets(const std::string& location) const -> const std::vector<Preset>&
	{
		return m_presets.at(location);
	}

	auto presets(const std::string& location) -> std::vector<Preset>&
	{
		return m_presets.at(location);
	}

	auto presets(std::string_view location) -> PresetMap::iterator
	{
		return m_presets.find(location);
	}

	//! Used during construction; The returned iterator is used to add presets at the location
	auto addInternalLocation() -> PresetMap::iterator;

	//! Used during construction; The returned iterator is used to add presets at the location
	auto addLocation(std::string_view path) -> PresetMap::iterator;

	//! Returns all presets that can be loaded by the plugin with the given subplugin key
	virtual auto fromKey(const Plugin::Descriptor::SubPluginFeatures::Key* key) const
		-> std::vector<const Preset*> = 0;

	struct Filetype
	{
		std::string name;
		std::string description;
		std::string extension;
	};

	void addFiletype(Filetype&& filetype)
	{
		m_filetypes.push_back(std::move(filetype));
	}

	//! Open preset dialog
	auto openPresetFile(std::string_view previousFile) -> const Preset*;

	//! Save preset dialog
	auto savePresetFile(const Preset& preset) -> bool;

	//! Creates a Preset from PresetLoadData.
	//! Plugins should override this to provide better preset info.
	virtual auto queryPreset(const PresetLoadData& loadData,
		const Plugin::Descriptor::SubPluginFeatures::Key* key = nullptr) const -> Preset;

private:

	//! Maps locations to presets.
	//! PresetLoadData references this map's key (location), so this map must remain stable after construction.
	//! See `PresetLoadData::location` for more info.
	PresetMap m_presets;

	std::vector<Filetype> m_filetypes;

	//const Plugin::Descriptor::SubPluginFeatures::Key* m_key = nullptr;
};

} // namespace lmms

#endif // LMMS_PRESET_H
