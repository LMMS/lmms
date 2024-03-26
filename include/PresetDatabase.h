/*
 * PresetDatabase.h - Preset discovery, loading, storage, and query
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

#ifndef LMMS_PRESET_DATABASE_H
#define LMMS_PRESET_DATABASE_H

#include <map>
#include <optional>
#include <set>

#include "lmms_export.h"
#include "Preset.h"

namespace lmms
{

/**
 * A plugin-specific collection of presets
 *
 * Contains all the loaded presets for a plugin or its subplugins.
 *
 * Plugins are expected to inherit this class to implement preset discovery,
 *   metadata loading, and other functionality.
 */
class LMMS_EXPORT PresetDatabase
{
public:
	struct Filetype
	{
		std::string name;
		std::string description;
		std::string extension; //< without dot; if empty, any extension is supported
	};

	//! Represents a preset directory or internal location where presets can be found
	struct Location
	{
		std::string name;
		std::string location; //!< PathUtil-compatible
		PresetMetadata::Flags flags = PresetMetadata::Flag::None;

		friend auto operator==(const Location& lhs, const Location& rhs) noexcept -> bool
		{
			return lhs.location == rhs.location;
		}

		friend auto operator<(const Location& lhs, const Location& rhs) noexcept -> bool
		{
			return lhs.location < rhs.location;
		}

		friend auto operator<(std::string_view location, const Location& rhs) noexcept -> bool
		{
			return location < rhs.location;
		}

		friend auto operator<(const Location& lhs, std::string_view location) noexcept -> bool
		{
			return lhs.location < location;
		}
	};

	using PresetMap = std::map<Location, std::set<Preset>, std::less<>>;

	PresetDatabase();
	virtual ~PresetDatabase() = default;

	//! Discover presets and populate database; Returns true when successful
	auto discover() -> bool;

	//! Load a preset file from disk; Returns empty vector upon failure or if preset(s) were already added
	auto loadPresets(std::string_view file) -> std::vector<const Preset*>;

	/**
	 * Query
	 */

	//! Returns all presets that can be loaded by the plugin with the given subplugin key
	auto findPresets(std::string_view key) const -> std::vector<const Preset*>;

	auto findPreset(const PresetLoadData& loadData, std::string_view key = std::string_view{}) const -> const Preset*;

	//! Returns all presets from the given file, loading them if needed. Returns empty vector upon failure.
	auto findOrLoadPresets(std::string_view file) -> std::vector<const Preset*>;

	/**
	 * Accessors
	 */

	auto presets() const -> auto& { return m_presets; }
	auto presets(std::string_view location) const -> const std::set<Preset>*;
	auto presets(std::string_view location) -> std::set<Preset>*;
	auto filetypes() const -> auto& { return m_filetypes; }
	auto recentPresetFile() const -> std::string_view { return m_recentPresetFile; }

//signal:
	//! TODO: Need to use this
	//void presetLoaded(const Preset& preset);

protected:

	/**
	 * 1st step of `discover()` - optional setup
	 *
	 * Return true if successful
	 */
	virtual auto discoverSetup() -> bool { return true; }

	/**
	 * 2nd step of `discover()` - declare all supported file types
	 *
	 * Return true if successful
	 */
	virtual auto discoverFiletypes(std::vector<Filetype>& filetypes) -> bool = 0;

	//! Function object to make `discoverLocations()` implementation simpler and hide implementation details
	class SetLocations
	{
	public:
		friend class PresetDatabase;
		void operator()(const std::vector<Location>& locations) const;
		void operator()(Location location) const;
	private:
		SetLocations(PresetMap& presets) : m_map{&presets} {}
		PresetMap* m_map = nullptr;
	};

	/**
	 * 3rd step of `discover()` - declare all pre-established preset locations
	 *
	 * Return true if successful
	 */
	virtual auto discoverLocations(const SetLocations& func) -> bool = 0;

	/**
	 * 4th and final step of `discover()` - populate set of presets at the given location
	 *
	 * Return true if successful
	 */
	virtual auto discoverPresets(const Location& location, std::set<Preset>& presets) -> bool = 0;

	/**
	 * Loads presets for the given location from `file`, retrieves any metadata,
	 * and adds the new presets to `presets`. Returns all the new presets.
	 *
	 * Used when the user loads a new preset from disk.
	 * May also be used by `discoverPresets()`.
	 *
	 * The default implementation only works for simple preset files.
	 * Plugins that support preset containers (where multiple presets could potentially be returned
	 * from this method) or provide additional preset metadata should reimplement this method.
	 */
	virtual auto loadPresets(const Location& location, std::string_view file, std::set<Preset>& presets)
		-> std::vector<const Preset*>;

	/**
	 * Gets preset location from `m_presets` which contains `path`, optionally adding it if it doesn't exist
	 *
	 * Returns iterator to the `m_presets` location
	 */
	auto getLocation(std::string_view path, bool add = true) -> PresetMap::iterator;

private:

	//! Maps locations to presets
	PresetMap m_presets;

	std::vector<Filetype> m_filetypes;

	std::string m_recentPresetFile;
};

} // namespace lmms

#endif // LMMS_PRESET_DATABASE_H
