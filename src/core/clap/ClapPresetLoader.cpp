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
#include "ClapManager.h"
#include "ClapPresetDatabase.h"
#include "Engine.h"

namespace lmms
{

ClapPresetLoader::ClapPresetLoader(Model* parent, ClapInstance* inst)
	: ClapExtension{inst}
	, PluginPresets{parent, nullptr, inst->info().descriptor().id}
{
	assert(instance() && "ClapExtension was not constructed properly");

	const auto mgr = Engine::getClapManager();
	const auto id = instance()->info().descriptor().id;
	setPresetDatabase(mgr->presetDatabase(id));
}

auto ClapPresetLoader::hostExtImpl() const -> const clap_host_preset_load*
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

auto ClapPresetLoader::activatePresetImpl(const PresetLoadData& preset) noexcept -> bool
{
	assert(ClapThreadCheck::isMainThread());
	if (!supported()) { return false; }

	std::string temp;
	const auto location = ClapPresetDatabase::toClapLocation(preset.location, temp);
	if (!location) { return false; }

#if 0
	{
		std::string msg = "About to activate preset: kind:" + std::to_string(location->first)
			+ "; location: \"" + std::string{location->second ? location->second : "(NULL)"}
			+ "\"; load key: \"" + preset.loadKey + "\"";
		logger().log(CLAP_LOG_INFO, msg);
	}
#endif

	if (!pluginExt()->from_location(plugin(), location->first, location->second, preset.loadKey.c_str()))
	{
		logger().log(CLAP_LOG_ERROR, "Failed to load preset");
		return false;
	}

	if (!instance()->params().supported()) { return true; }

	return instance()->params().rescan(CLAP_PARAM_RESCAN_VALUES); // TODO: Is this correct?
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
	auto& self = h->presetLoader();

#if 0
	const std::string text = "Loaded preset: location kind: " + std::to_string(locationKind)
		+ "; location: \"" + std::string{location ? location : ""} + "\"; load key: \""
		+ std::string{loadKey ? loadKey : ""} + "\"";

	h->logger().log(CLAP_LOG_INFO, text);
#endif

	const auto loadData = ClapPresetDatabase::fromClapLocation(
		static_cast<clap_preset_discovery_location_kind>(locationKind), location, loadKey);
	if (!loadData) { return; }

	const auto index = self.findPreset(*loadData);
	if (!index)
	{
		// TODO: Can we assume preset always exists in `m_presets`?
		h->logger().log(CLAP_LOG_ERROR, "Could not find preset in database");
		return;
	}

	self.setActivePreset(*index);
}

} // namespace lmms

#endif // LMMS_HAVE_CLAP
