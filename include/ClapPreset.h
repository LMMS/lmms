/*
 * ClapPreset.h - Represents a CLAP preset
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

#ifndef LMMS_CLAP_PRESET_H
#define LMMS_CLAP_PRESET_H

#include "lmmsconfig.h"

#ifdef LMMS_HAVE_CLAP

#include <unordered_map>
#include <string>
#include <vector>
#include <clap/factory/preset-discovery.h>

#include "ClapExtension.h"
#include "lmms_math.h"
#include "Preset.h"

namespace lmms
{

class ClapPresets : public PluginPresets
{
public:
	using PluginPresets::PluginPresets;

	struct Filetype
	{
		// TODO: Can clap_preset_discovery_filetype be used? (Are the const char*'s safe to store?)
		std::string name;
		std::string description;
		std::string extension;
	};

	struct LocationMetadata
	{
		std::string name;
		clap_preset_discovery_flags flags;
		clap_preset_discovery_location_kind kind;
	};

	auto locationMetadata() const -> auto& { return m_locationMetadata; }

	// Used during construction

	void addLocationMetadata(std::string_view location, LocationMetadata&& metadata)
	{
		m_locationMetadata[location] = std::move(metadata);
	}

	void addFiletype(Filetype&& filetype)
	{
		m_filetypes.push_back(std::move(filetype));
	}

private:

	auto populate() -> bool { return true; } // TODO

	//! Maps locations to their metadata
	std::unordered_map<std::string_view, LocationMetadata> m_locationMetadata;

	std::vector<Filetype> m_filetypes;
};

} // namespace lmms

#endif // LMMS_HAVE_CLAP

#endif // LMMS_CLAP_PRESET_H
