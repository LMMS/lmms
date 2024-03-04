/*
 * ClapPresetDatabase.h - Implementation of PresetDatabase for CLAP presets
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

#ifndef LMMS_CLAP_PRESET_DATABASE_H
#define LMMS_CLAP_PRESET_DATABASE_H

#include "lmmsconfig.h"

#ifdef LMMS_HAVE_CLAP

#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <clap/factory/preset-discovery.h>

#include "ClapExtension.h"
#include "NoCopyNoMove.h"
#include "PresetDatabase.h"

struct clap_plugin_entry;

namespace lmms
{

class ClapPresetDatabase : public PresetDatabase
{
public:
	using PresetDatabase::PresetDatabase;

	auto init(const clap_plugin_entry* entry) -> bool;
	void deinit();

	auto discover() -> bool override;

	auto supported() const -> bool { return m_factory; }

	static auto toClapLocation(std::string_view location, std::string& ref)
		-> std::optional<std::pair<clap_preset_discovery_location_kind, const char*>>;
	static auto fromClapLocation(const char* location) -> std::string;
	static auto fromClapLocation(clap_preset_discovery_location_kind kind,
		const char* location, const char* loadKey) -> std::optional<PresetLoadData>;

private:
	auto createPreset(const Preset::LoadData& loadData) const -> std::optional<Preset> override;

	class Indexer : public NoCopyNoMove
	{
	public:
		static auto create(const clap_preset_discovery_factory& factory, std::uint32_t index)
			-> std::unique_ptr<Indexer>;

		Indexer() = delete;
		Indexer(const clap_preset_discovery_factory& factory,
			const clap_preset_discovery_provider_descriptor& descriptor);
		~Indexer() = default;

		auto query(std::string_view location, PresetMetadata::Flags flags = PresetMetadata::Flag::None)
			-> std::optional<std::vector<Preset>>;

		auto provider() const -> const clap_preset_discovery_provider* { return m_provider.get(); }

		//! Like clap_preset_discovery_location, for storing declare_location() data
		struct Location
		{
			PresetMetadata::Flags flags;
			std::string name;
			clap_preset_discovery_location_kind kind;
			std::string location; //!< PathUtil-compatible
		};

		auto locations() -> auto& { return m_locations; }
		auto filetypes() -> auto& { return m_filetypes; }

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

		std::vector<Location> m_locations;
		std::vector<PresetDatabase::Filetype> m_filetypes;
	};

	class MetadataReceiver;

	const clap_preset_discovery_factory* m_factory = nullptr;
	std::vector<std::unique_ptr<Indexer>> m_indexers;
	std::map<std::string, std::vector<Indexer*>, std::less<>> m_filetypeIndexerMap;
};

} // namespace lmms

#endif // LMMS_HAVE_CLAP

#endif // LMMS_CLAP_PRESET_DATABASE_H
