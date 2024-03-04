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

#include "Preset.h"

namespace lmms
{

/**
 * A plugin-specific collection of presets
 *
 * Contains all the loaded presets for a plugin, its subplugins, or a subset of its subplugins.
 *
 * Plugins are expected to inherit this class to implement preset discovery,
 *   metadata loading, and other functionality.
 */
class PresetDatabase //: public QObject //: public NoCopyNoMove
{
public:
	using PresetMap = std::map<std::string, std::set<Preset>, std::less<>>;

	struct Filetype
	{
		std::string name;
		std::string description;
		std::string extension;
	};

	PresetDatabase() = default;
	//using QObject::QObject;
	//PresetDatabase(std::string_view key);
	virtual ~PresetDatabase() = default;

	//! Discover presets and populate database; Returns true when successful
	virtual auto discover() -> bool = 0;

	//! Load a preset file from disk; Returns nullptr upon failure or if preset was already added
	auto addPreset(std::string_view path) -> const Preset*;

	/**
	 * Accessors
	 */

	auto presets() const -> auto& { return m_presets; }

	auto presets(const std::string& location) const -> const std::set<Preset>&
	{
		return m_presets.at(location);
	}

	auto presets(const std::string& location) -> std::set<Preset>&
	{
		return m_presets.at(location);
	}

	auto presets(std::string_view location) -> PresetMap::iterator
	{
		return m_presets.find(location);
	}

	auto filetypes() const -> auto& { return m_filetypes; }

	//! Returns all presets that can be loaded by the plugin with the given subplugin key
	auto findPresets(std::string_view key) const -> std::vector<const Preset*>;

	auto findPreset(const PresetLoadData& loadData, std::string_view key = std::string_view{}) const -> const Preset*;

	//! Open preset dialog TODO: Move to an lmms::gui class?
	auto openPresetFile(std::string_view previousFile) -> const Preset*;

	//! Save preset dialog TODO: Move to an lmms::gui class?
	auto savePresetFile(const Preset& preset) -> bool;


//signal:
	//! TODO: Need to use this
	//void presetLoaded(const Preset& preset);

protected:

	//! Creates a populated Preset object from Preset::LoadData.
	//! Plugins should override this to provide better preset metadata.
	virtual auto createPreset(const Preset::LoadData& loadData) const -> std::optional<Preset>;

	//! Creates a populated Preset object from Preset::LoadData and key(s).
	//! Plugins should override this to provide better preset metadata.
	virtual auto createPreset(const Preset::LoadData& loadData,
		const std::vector<std::string>& keys) const -> std::optional<Preset>;

	/**
	 * Adds a preset file to `m_presets` if it doesn't exist, and returns the load data.
	 *
	 * Checks all the combinations for splitting `path` into location and load key before adding.
	 * TODO: Return map iterator so that `m_presets` does not need to be searched again?
	 */
	auto addFile(std::string_view path) -> std::optional<Preset::LoadData>;

	//! Use during discover() call; The returned iterator is used to add presets at the location
	auto addLocation(std::string_view path) -> PresetMap::iterator;

	//! Use during discover() call
	void addFiletype(Filetype filetype)
	{
		m_filetypes.push_back(std::move(filetype));
	}

	//! Use during discover() call
	void setFiletypes(std::vector<Filetype> filetypes)
	{
		m_filetypes = std::move(filetypes);
	}

private:

	//! Maps locations to presets.
	//! PresetLoadData references this map's key (location), so this map must remain stable after construction.
	//! See `PresetLoadData::location` for more info.
	PresetMap m_presets;

	std::vector<Filetype> m_filetypes;
};

} // namespace lmms

#endif // LMMS_PRESET_DATABASE_H
