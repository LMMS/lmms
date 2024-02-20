/*
 * ClapPresetLoader.h - Implements CLAP preset load extension
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

#ifndef LMMS_CLAP_PRESET_LOADER_H
#define LMMS_CLAP_PRESET_LOADER_H

#include "lmmsconfig.h"

#ifdef LMMS_HAVE_CLAP

#include <optional>
#include <clap/ext/preset-load.h>

#include "ClapExtension.h"
#include "Preset.h"

namespace lmms
{

// TODO: Inherit from PluginPresets
class ClapPresetLoader final : public ClapExtension<clap_host_preset_load, clap_plugin_preset_load>
{
public:
	using ClapExtension::ClapExtension;
	~ClapPresetLoader() override = default;

	auto extensionId() const -> std::string_view override { return CLAP_EXT_PRESET_LOAD; }
	auto extensionIdCompat() const -> std::string_view override { return CLAP_EXT_PRESET_LOAD_COMPAT; }
	auto hostExt() const -> const clap_host_preset_load* override;

	auto load(const PresetLoadData& preset) -> bool;

private:
	auto initImpl(const clap_host* host, const clap_plugin* plugin) noexcept -> bool override;
	auto checkSupported(const clap_plugin_preset_load& ext) -> bool override;

	/**
	 * clap_host_preset_load implementation
	 */
	static void clapOnError(const clap_host* host, std::uint32_t locationKind,
		const char* location, const char* loadKey, std::int32_t osError, const char* msg);
	static void clapLoaded(const clap_host* host, std::uint32_t locationKind,
		const char* location, const char* loadKey);

	// TODO: Make these plugin-independent? A plugin-independent preset viewer will need them.
	std::optional<Preset> m_activePreset;
	bool m_dirty = false; //!< Whether the active preset has been modified
};

} // namespace lmms

#endif // LMMS_HAVE_CLAP

#endif // LMMS_CLAP_PRESET_LOADER_H
