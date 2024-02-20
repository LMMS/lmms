/*
 * Preset.h - A generic preset class for plugins
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

#include <string>
#include <string_view>
#include <vector>

#include "Flags.h"
#include "NoCopyNoMove.h"
//#include "Plugin.h"

namespace lmms
{

//! Contains all information needed for loading a preset associated with a specific plugin or subplugin
struct PresetLoadData
{
	/**
	 * A string that can be parsed by PathUtil.
	 * Its meaning depends on the plugin, but it typically represents a preset directory.
	 *
	 * Valid examples:
	 * - "userprojects:"
	 * - "preset:MyPlugin/MyFavoritePresets"
	 * - "/my/absolute/directory"
	 * - "preset:my_plugin/my_preset_database.txt" (depending on the plugin, it could even be a file!)
	 * - "internal:" (for presets stored within the plugin's DSO rather than on disk)
	 *
	 * This is a string_view which references a key within `PresetDatabase::m_presets` in order to save space.
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
	std::string displayName;
	std::string creator;
	std::string description;
	std::vector<std::string> categories;

	enum class Flag
	{
		None           = 0,
		FactoryContent = 1 << 0,
		UserContent    = 1 << 1,
		UserFavorite   = 1 << 2
	};

	using Flags = lmms::Flags<Flag>;

	Flags flags;
};

LMMS_DECLARE_OPERATORS_FOR_FLAGS(PresetMetadata::Flag)

//! Generic preset
struct Preset
{
	PresetMetadata metadata;
	PresetLoadData loadData;

	//! Subplugin keys that support this preset
	//!   Empty if all subplugins support the preset or if there are no subplugins
	//std::vector<const Plugin::Descriptor::SubPluginFeatures::Key*> keys;
	std::vector<std::string> keys;

	//! TODO: Some plugins may cache preset data for faster loading; empty = uncached
	//std::string data;

	//! Enable std::set support
	friend auto operator<(const Preset& a, const Preset& b) noexcept -> bool
	{
		return a.loadData.loadKey < b.loadData.loadKey;
	}
};

} // namespace lmms

#endif // LMMS_PRESET_H
