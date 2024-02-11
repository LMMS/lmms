/*
 * ClapPresetDiscovery.h - Discovers CLAP presets
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

#ifndef LMMS_CLAP_PRESET_DISCOVERY_H
#define LMMS_CLAP_PRESET_DISCOVERY_H

#include "lmmsconfig.h"

#ifdef LMMS_HAVE_CLAP

#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <clap/factory/preset-discovery.h>

#include "ClapExtension.h"
#include "ClapPreset.h"
#include "NoCopyNoMove.h"

struct clap_plugin_entry;

namespace lmms
{

class ClapPresetDiscovery
{
public:
	ClapPresetDiscovery() = default;
	//ClapPresetDiscovery(const clap_plugin_entry* entry);

	auto discover(const clap_plugin_entry* entry) -> bool;

private:

	const clap_preset_discovery_factory* m_factory = nullptr;

	std::uint32_t m_providerCount = 0;

	class Indexer : public NoCopyNoMove
	{
	public:
		static auto create(const clap_preset_discovery_factory& factory, std::uint32_t index)
			-> std::unique_ptr<Indexer>;

		Indexer() = delete;
		Indexer(const clap_preset_discovery_factory& factory,
			const clap_preset_discovery_provider_descriptor& descriptor);
		~Indexer() = default;

	private:
		/**
		 * clap_preset_discovery_indexer implementation
		 */
		static auto clapDeclareFiletype(const clap_preset_discovery_indexer* indexer,
			const clap_preset_discovery_filetype* filetype) -> bool;
		static auto clapDeclareLocation(const clap_preset_discovery_indexer* indexer,
			const clap_preset_discovery_location* location) -> bool;
		static auto clapDeclareSoundpack(const clap_preset_discovery_indexer* indexer,
			const clap_preset_discovery_soundpack* soundpack) -> bool;
		static auto clapGetExtension(const clap_preset_discovery_indexer* indexer,
			const char* extensionId) -> const void*;

		struct ProviderDeleter
		{
			void operator()(const clap_preset_discovery_provider* p) const noexcept
			{
				p->destroy(p);
			}
		};

		clap_preset_discovery_indexer m_indexer;
		std::unique_ptr<const clap_preset_discovery_provider, ProviderDeleter> m_provider;

		std::shared_ptr<ClapPresets> m_presets;
	};

	std::vector<std::unique_ptr<Indexer>> m_indexers;

	// TODO: Move to .cpp file?
	class MetadataReceiver : public NoCopyNoMove
	{
	public:
		MetadataReceiver() = delete;
		MetadataReceiver(std::string_view location, std::vector<Preset>& presets);

		auto receiver() const -> const auto* { return &m_receiver; }
		auto errorMessage() const -> auto& { return m_error; }

	private:
		/**
		 * clap_preset_discovery_metadata_receiver implementation
		 */
		static void clapOnError(const clap_preset_discovery_metadata_receiver* receiver,
			std::int32_t osError, const char* errorMessage);
		static auto clapBeginPreset(const clap_preset_discovery_metadata_receiver* receiver,
			const char* name, const char* loadKey) -> bool;
		static void clapAddPluginId(const clap_preset_discovery_metadata_receiver* receiver,
			const clap_universal_plugin_id* pluginId);
		static void clapSetSoundpackId(const clap_preset_discovery_metadata_receiver* receiver,
			const char* soundpackId);
		static void clapSetFlags(const clap_preset_discovery_metadata_receiver* receiver, std::uint32_t flags);
		static void clapAddCreator(const clap_preset_discovery_metadata_receiver* receiver, const char* creator);
		static void clapSetDescription(const clap_preset_discovery_metadata_receiver* receiver,
			const char* description);
		static void clapSetTimestamps(const clap_preset_discovery_metadata_receiver* receiver,
			clap_timestamp creationTime, clap_timestamp modificationTime);
		static void clapAddFeature(const clap_preset_discovery_metadata_receiver* receiver,
			const char* feature);
		static void clapAddExtraInfo(const clap_preset_discovery_metadata_receiver* receiver,
			const char* key, const char* value);

		static auto from(const clap_preset_discovery_metadata_receiver* receiver) -> MetadataReceiver*;

		clap_preset_discovery_metadata_receiver m_receiver;

		std::string_view m_location;
		std::vector<Preset>* m_presets;

		std::string m_error;
	};



	// clap_preset_discovery_indexer              --> host implements this
	// clap_preset_discovery_provider_descriptor  --> plugin provides this; host probably just needs the id from it
	// clap_preset_discovery_provider             --> plugin provides this
	// clap_preset_discovery_metadata_receiver    --> host implements this


	// Info needed to later load a preset:
	//     const clap_plugin* plugin,
	//     uint32_t location_kind,
	//     const char* location,
	//     const char* load_key

};

} // namespace lmms

#endif // LMMS_HAVE_CLAP

#endif // LMMS_CLAP_PRESET_DISCOVERY_H
