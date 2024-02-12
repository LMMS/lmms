/*
 * ClapPresetDiscovery.cpp - Discovers CLAP presets
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

#include "ClapPresetDiscovery.h"

#ifdef LMMS_HAVE_CLAP

#include <QDebug>
#include <cassert>
#include <clap/entry.h>

#include "ClapLog.h"
#include "lmms_filesystem.h"
#include "lmmsversion.h"

namespace lmms
{

auto ClapPresetDiscovery::discover(const clap_plugin_entry* entry) -> bool
{
	if (!entry) { return false; }

	m_factory = static_cast<const clap_preset_discovery_factory*>(
		entry->get_factory(CLAP_PRESET_DISCOVERY_FACTORY_ID));

	if (!m_factory)
	{
		// Try using compatibility ID if it exists
		m_factory = static_cast<const clap_preset_discovery_factory*>(
			entry->get_factory(CLAP_PRESET_DISCOVERY_FACTORY_ID_COMPAT));
	}

	if (!m_factory) { return false; }

	m_providerCount = m_factory->count(m_factory);
	qDebug() << "m_providerCount:" << m_providerCount;
	if (m_providerCount == 0) { return false; }

	for (std::uint32_t idx = 0; idx < m_providerCount; ++idx)
	{
		if (auto indexer = Indexer::create(*m_factory, idx))
		{
			qDebug() << "Indexer successfully created";
			m_indexers.push_back(std::move(indexer));
		} else { qDebug() << "Indexer creation FAILED"; }
	}

	return true;
}

auto ClapPresetDiscovery::Indexer::create(const clap_preset_discovery_factory& factory, std::uint32_t index)
	-> std::unique_ptr<Indexer>
{
	const auto desc = factory.get_descriptor(&factory, index);
	if (!desc || !desc->id) { return nullptr; }

	if (!clap_version_is_compatible(desc->clap_version)) { return nullptr; }

	auto indexer = std::make_unique<Indexer>(factory, *desc);
	return indexer->m_provider ? std::move(indexer) : nullptr;
}

ClapPresetDiscovery::Indexer::Indexer(const clap_preset_discovery_factory& factory,
	const clap_preset_discovery_provider_descriptor& descriptor)
	: m_indexer {
		CLAP_VERSION,
		"LMMS",
		"LMMS contributors",
		"https://lmms.io/",
		LMMS_VERSION,
		this,
		&clapDeclareFiletype,
		&clapDeclareLocation,
		&clapDeclareSoundpack,
		&clapGetExtension
	}
	, m_provider{factory.create(&factory, &m_indexer, descriptor.id), ProviderDeleter{}}
{
	std::string tempMsg = "PRESET DISCOVERY PROVIDER DESCRIPTOR ID" + std::string{descriptor.id};
	ClapLog::globalLog(CLAP_LOG_ERROR, tempMsg);

	if (!m_provider)
	{
		ClapLog::globalLog(CLAP_LOG_ERROR, "Failed to create preset discovery provider");
		return;
	}

	if (!m_provider->init || !m_provider->get_metadata)
	{
		ClapLog::globalLog(CLAP_LOG_ERROR, "Plugin does not fully implement preset discovery provider");
		m_provider.reset();
		return;
	}

	m_presets = std::make_shared<ClapPresets>();

	if (!m_provider->init(m_provider.get()))
	{
		ClapLog::globalLog(CLAP_LOG_ERROR, "Failed to initialize preset discovery provider");
		m_provider.reset();
		return;
	}

	if (m_presets->presets().empty())
	{
		// No presets
		return;
	}

	qDebug() << "Iterating over locations...";
	for (const auto& [loc, metadata] : m_presets->locationMetadata())
	{
		qDebug() << "loc:" << loc.data();
		qDebug() << "loc metadata: name:" << metadata.name.c_str()
			<< "flags:" << metadata.flags << "kind:" << metadata.kind;

		auto receiver = MetadataReceiver{m_presets->presets(loc)};
		if (!m_provider->get_metadata(m_provider.get(), metadata.kind, loc.data(), receiver.receiver()))
		{
			std::string msg = "Failed to get metadata for preset discovery provider: " + receiver.errorMessage();
			ClapLog::globalLog(CLAP_LOG_ERROR, msg.c_str());
			continue;
		}
	}
}

auto ClapPresetDiscovery::Indexer::clapDeclareFiletype(const clap_preset_discovery_indexer* indexer,
	const clap_preset_discovery_filetype* filetype) -> bool
{
	if (!indexer || !indexer->indexer_data || !filetype || !filetype->name) { return false; }

	auto self = static_cast<Indexer*>(indexer->indexer_data);
	if (!self) { return false; }

	auto ft = ClapPresets::Filetype{};
	ft.name = filetype->name;
	if (filetype->description) { ft.description = filetype->description; }
	if (filetype->file_extension) { ft.extension = filetype->file_extension; }

	self->m_presets->addFiletype(std::move(ft));
	return true;
}

auto ClapPresetDiscovery::Indexer::clapDeclareLocation(const clap_preset_discovery_indexer* indexer,
	const clap_preset_discovery_location* location) -> bool
{
	if (!indexer || !indexer->indexer_data || !location) { return false; }

	auto self = static_cast<Indexer*>(indexer->indexer_data);
	if (!self) { return false; }

	PluginPresets::PresetMap::iterator iter;
	switch (location->kind)
	{
		case CLAP_PRESET_DISCOVERY_LOCATION_PLUGIN:
			if (location->location) { return false; } // PLUGIN kind must have null location
			iter = self->m_presets->addInternalLocation();
			break;
		case CLAP_PRESET_DISCOVERY_LOCATION_FILE:
		{
			if (!location->location)
			{
				ClapLog::globalLog(CLAP_LOG_PLUGIN_MISBEHAVING,
					"Preset with FILE location kind cannot have null location");
				return false;
			}

			if (std::error_code ec; !fs::is_regular_file(location->location, ec) || ec)
			{
				std::string msg = "Preset location \"" + std::string{location->location}
					+ "\" does not exist";
				ClapLog::globalLog(CLAP_LOG_WARNING, msg);
				return false;
			}

			iter = self->m_presets->addLocation(location->location);
			break;
		}
		default:
			ClapLog::globalLog(CLAP_LOG_PLUGIN_MISBEHAVING, "Invalid preset location kind");
			return false;
	}

	const auto& locationRef = iter->first;

	auto metadata = ClapPresets::LocationMetadata{};
	if (location->name) { metadata.name = location->name; }
	metadata.flags = static_cast<clap_preset_discovery_flags>(location->flags);
	metadata.kind = static_cast<clap_preset_discovery_location_kind>(location->kind);

	self->m_presets->addLocationMetadata(locationRef, std::move(metadata));
	return true;
}

auto ClapPresetDiscovery::Indexer::clapDeclareSoundpack(const clap_preset_discovery_indexer* indexer,
	const clap_preset_discovery_soundpack* soundpack) -> bool
{
	// TODO: Implement later?
	return true;
}

auto ClapPresetDiscovery::Indexer::clapGetExtension(const clap_preset_discovery_indexer* indexer,
	const char* extensionId) -> const void*
{
	// LMMS does not have any custom indexer extensions
	return nullptr;
}

ClapPresetDiscovery::MetadataReceiver::MetadataReceiver(PluginPresets::PresetMap::iterator presets)
	: m_receiver {
		this,
		&clapOnError,
		&clapBeginPreset,
		&clapAddPluginId,
		&clapSetSoundpackId,
		&clapSetFlags,
		&clapAddCreator,
		&clapSetDescription,
		&clapSetTimestamps,
		&clapAddFeature,
		&clapAddExtraInfo
	}
	, m_presets{presets}
{
}

void ClapPresetDiscovery::MetadataReceiver::clapOnError(
	const clap_preset_discovery_metadata_receiver* receiver, std::int32_t osError, const char* errorMessage)
{
	if (auto rec = from(receiver))
	{
		rec->m_error = "{os_error=" + std::to_string(osError) + ", error_message=\""
			+ (errorMessage ? errorMessage : std::string{}) + "\"}";
	}
}

auto ClapPresetDiscovery::MetadataReceiver::clapBeginPreset(
	const clap_preset_discovery_metadata_receiver* receiver, const char* name, const char* loadKey) -> bool
{
	if (!loadKey) { return false; }
	auto rec = from(receiver);
	if (!rec) { return false; }

	auto& [location, presets] = *rec->m_presets;

	auto& preset = presets.emplace_back();
	preset.displayName = name ? name : std::string{};
	preset.loadData.location = location; // references the map's key
	preset.loadData.loadKey =  loadKey ? loadKey : std::string{};

	qDebug() << "clapBeginPreset: display name:" << preset.displayName.c_str() << "load key:" << preset.loadData.loadKey.c_str();

	// TODO

	return true;
}

void ClapPresetDiscovery::MetadataReceiver::clapAddPluginId(
	const clap_preset_discovery_metadata_receiver* receiver, const clap_universal_plugin_id* pluginId)
{
	if (!pluginId || !pluginId->abi || !pluginId->id)
	{
		ClapLog::globalLog(CLAP_LOG_WARNING, "Plugin called clap_preset_discovery_metadata_receiver.add_plugin_id() with invalid arguments");
		return;
	}

	// TODO: How to handle this?
	if (pluginId->abi != std::string_view{"clap"})
	{
		ClapLog::globalLog(CLAP_LOG_WARNING, "Preset must use the \"clap\" abi");
		// TODO: Remove the preset?
		return;
	}

	// TODO
}

void ClapPresetDiscovery::MetadataReceiver::clapSetSoundpackId(
	const clap_preset_discovery_metadata_receiver* receiver, const char* soundpackId)
{
	// [UNIMPLEMENTED]
}

void ClapPresetDiscovery::MetadataReceiver::clapSetFlags(
	const clap_preset_discovery_metadata_receiver* receiver, std::uint32_t flags)
{
	// [UNIMPLEMENTED]
}

void ClapPresetDiscovery::MetadataReceiver::clapAddCreator(
	const clap_preset_discovery_metadata_receiver* receiver, const char* creator)
{
	// [UNIMPLEMENTED]
}

void ClapPresetDiscovery::MetadataReceiver::clapSetDescription(
	const clap_preset_discovery_metadata_receiver* receiver, const char* description)
{
	// [UNIMPLEMENTED]
}

void ClapPresetDiscovery::MetadataReceiver::clapSetTimestamps(
	const clap_preset_discovery_metadata_receiver* receiver,
	clap_timestamp creationTime, clap_timestamp modificationTime)
{
	// [UNIMPLEMENTED]
}

void ClapPresetDiscovery::MetadataReceiver::clapAddFeature(
	const clap_preset_discovery_metadata_receiver* receiver, const char* feature)
{
	// [UNIMPLEMENTED]
}

void ClapPresetDiscovery::MetadataReceiver::clapAddExtraInfo(
	const clap_preset_discovery_metadata_receiver* receiver, const char* key, const char* value)
{
	// [UNIMPLEMENTED]
}

auto ClapPresetDiscovery::MetadataReceiver::from(
	const clap_preset_discovery_metadata_receiver* receiver) -> MetadataReceiver*
{
	if (!receiver || !receiver->receiver_data)
	{
		ClapLog::globalLog(CLAP_LOG_ERROR,
			"Preset discovery metadata receiver's context pointer was invalidated by the plugin.");
		return nullptr;
	}
	return static_cast<MetadataReceiver*>(receiver->receiver_data);
}

} // namespace lmms

#endif // LMMS_HAVE_CLAP
