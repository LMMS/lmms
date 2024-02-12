/*
 * ClapPresetLoader.cpp - Implements CLAP preset load extension
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

#include "ClapPresetLoader.h"

#ifdef LMMS_HAVE_CLAP

#include <cassert>

#include "ClapInstance.h"
#include "ClapPreset.h"
#include "PathUtil.h"

namespace lmms
{

auto ClapPresetLoader::load(const PresetLoadData& preset) -> bool
{
	assert(ClapThreadCheck::isMainThread());
	if (!supported()) { return false; }

	const auto [base, path] = PathUtil::parsePath(preset.location);

	clap_preset_discovery_location_kind locationKind;
	std::string location;
	switch (base)
	{
		case PathUtil::Base::Internal:
			locationKind = CLAP_PRESET_DISCOVERY_LOCATION_PLUGIN;
			break;
		case PathUtil::Base::Absolute:
			locationKind = CLAP_PRESET_DISCOVERY_LOCATION_FILE;
			location = path;
			break;
		default:
		{
			locationKind = CLAP_PRESET_DISCOVERY_LOCATION_FILE;
			if (auto temp = PathUtil::getBaseLocation(base))
			{
				location = temp.value();
			}
			else { return false; }
			break;
		}
	}

	if (!pluginExt()->from_location(plugin(), locationKind, location.c_str(), preset.loadKey.c_str()))
	{
		logger().log(CLAP_LOG_ERROR, "Failed to load preset");
		return false;
	}

	return true;
}

auto ClapPresetLoader::initImpl(const clap_host* host, const clap_plugin* plugin) noexcept -> bool
{
	return true;
}

auto ClapPresetLoader::hostExt() const -> const clap_host_preset_load*
{
	static clap_host_preset_load ext {
		&clapOnError,
		&clapLoaded
	};
	return &ext;
}

auto ClapPresetLoader::checkSupported(const clap_plugin_preset_load& ext) -> bool
{
	return ext.from_location;
}

void ClapPresetLoader::clapOnError(const clap_host* host, std::uint32_t locationKind,
	const char* location, const char* loadKey, std::int32_t osError, const char* msg)
{
	const auto h = fromHost(host);
	if (!h) { return; }

	const std::string text = "Preset load error: location kind: " + std::to_string(locationKind)
		+ "; location: \"" + std::string{location ? location : ""} + "\"; load key: \""
		+ std::string{loadKey ? loadKey : ""} + "\"; OS error: " + std::to_string(osError)
		+ "; msg: \"" + std::string{msg ? msg : ""} + "\"";

	h->logger().log(CLAP_LOG_ERROR, text);
}

void ClapPresetLoader::clapLoaded(const clap_host* host, std::uint32_t locationKind,
	const char* location, const char* loadKey)
{
	const auto h = fromHost(host);
	if (!h) { return; }

#if 1
	const std::string text = "Loaded preset: location kind: " + std::to_string(locationKind)
		+ "; location: \"" + std::string{location ? location : ""} + "\"; load key: \""
		+ std::string{loadKey ? loadKey : ""} + "\"";

	h->logger().log(CLAP_LOG_INFO, text);
#endif

	// TODO: Keep track of currently loaded preset
}

} // namespace lmms

#endif // LMMS_HAVE_CLAP
