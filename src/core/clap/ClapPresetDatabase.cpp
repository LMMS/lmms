/*
 * ClapPresetDatabase.cpp - Implementation of PresetDatabase for CLAP presets
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

#include "ClapPresetDatabase.h"

#ifdef LMMS_HAVE_CLAP

#include <QDebug>
#include <cassert>
#include <clap/entry.h>

#include "ClapLog.h"
#include "lmms_filesystem.h"
#include "lmmsversion.h"
#include "PathUtil.h"

namespace lmms
{

namespace
{
	//! Converts clap_preset_discovery_flags to PresetMetadata::Flags
	auto convertFlags(std::uint32_t flags) -> PresetMetadata::Flags
	{
		PresetMetadata::Flags result = PresetMetadata::Flag::None;
		if (flags & CLAP_PRESET_DISCOVERY_IS_FACTORY_CONTENT) { result |= PresetMetadata::Flag::FactoryContent; }
		if (flags & CLAP_PRESET_DISCOVERY_IS_USER_CONTENT) { result |= PresetMetadata::Flag::UserContent; }
		if (flags & CLAP_PRESET_DISCOVERY_IS_FAVORITE) { result |= PresetMetadata::Flag::UserFavorite; }
		return result;
	}
} // namespace

class ClapPresetDatabase::MetadataReceiver : public NoCopyNoMove
{
public:
	MetadataReceiver() = delete;
	MetadataReceiver(const Indexer& indexer);

	auto query(std::string_view location, PresetMetadata::Flags flags = PresetMetadata::Flag::None)
		-> std::optional<std::vector<Preset>>;

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

	const Indexer* m_indexer = nullptr;

	std::string_view m_location;   //!< Used during the call to query()
	PresetMetadata::Flags m_flags; //!< Used during the call to query()
	std::vector<Preset> m_presets; //!< Used during the call to query()

	std::string m_error;
};

auto ClapPresetDatabase::init(const clap_plugin_entry* entry) -> bool
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

	return m_factory;
}

void ClapPresetDatabase::deinit()
{
	m_indexers.clear();
}

auto ClapPresetDatabase::discover() -> bool
{
	if (!m_factory)
	{
		// This CLAP file doesn't provide preset support
		return false;
	}

	const auto providerCount = m_factory->count(m_factory);
	qDebug() << "providerCount:" << providerCount;
	if (providerCount == 0) { return false; }

	for (std::uint32_t idx = 0; idx < providerCount; ++idx)
	{
		if (auto indexer = Indexer::create(*m_factory, idx))
		{
			qDebug() << "Indexer successfully created";
			auto& indexerRef = m_indexers.emplace_back(std::move(indexer));
			for (auto& filetype : indexerRef->filetypes())
			{
				m_filetypeIndexerMap[filetype.extension].push_back(indexerRef.get());
			}
		} else { qDebug() << "Indexer creation FAILED"; }
	}

	for (auto& indexer : m_indexers)
	{
		setFiletypes(std::move(indexer->filetypes()));

		qDebug() << "Iterating over locations...";
		for (auto& location : indexer->locations())
		{
			qDebug() << "location:" << location.location.c_str();
			qDebug() << "location name:" << location.name.c_str()
				<< "flags:" << static_cast<int>(location.flags) << "kind:" << location.kind;

			auto& [locationRef, presets] = *addLocation(location.location);
			auto newPresets = indexer->query(locationRef, location.flags);
			if (!newPresets) { continue; }
			for (auto& preset : *newPresets)
			{
				presets.insert(std::move(preset));
			}
		}
	}

	return true;
}

auto ClapPresetDatabase::toClapLocation(std::string_view location, std::string& ref)
	-> std::optional<std::pair<clap_preset_discovery_location_kind, const char*>>
{
	const auto base = PathUtil::baseLookup(location);

	clap_preset_discovery_location_kind locationKind;
	const char* locationOut = nullptr;
	switch (base)
	{
		case PathUtil::Base::Internal:
			locationKind = CLAP_PRESET_DISCOVERY_LOCATION_PLUGIN;
			//locationOut = nullptr;
			break;
		case PathUtil::Base::Absolute:
			locationKind = CLAP_PRESET_DISCOVERY_LOCATION_FILE;
			locationOut = location.data();
			break;
		default:
		{
			locationKind = CLAP_PRESET_DISCOVERY_LOCATION_FILE;
			if (auto temp = PathUtil::toAbsolute(location))
			{
				ref = std::move(*temp);
				locationOut = ref.c_str();
			}
			else
			{
				ClapLog::globalLog(CLAP_LOG_ERROR, "Failed to get absolute path for preset");
				return std::nullopt;
			}
			break;
		}
	}

	return std::pair{ locationKind, locationOut };
}

auto ClapPresetDatabase::fromClapLocation(const char* location) -> std::string
{
	return location
		? PathUtil::toShortestRelative(std::string_view{location})
		: std::string{PathUtil::basePrefix(PathUtil::Base::Internal)};
}

auto ClapPresetDatabase::fromClapLocation([[maybe_unused]] clap_preset_discovery_location_kind kind,
	const char* location, const char* loadKey, std::string& ref) -> std::optional<PresetLoadData>
{
	if (!location && !loadKey) { return std::nullopt; }

	PresetLoadData data;
	ref = fromClapLocation(location);
	data.location = ref;
	data.loadKey = loadKey ? loadKey : std::string{};
	return data;
}

auto ClapPresetDatabase::createPreset(const PresetLoadData& loadData) const -> std::optional<Preset>
{
	// NOTE: Any time this method is called, PresetLoadData stores a path to a preset file (???)
	// TODO: Should all possible presets already be discovered at the start? Is it a bug if this is ever called for CLAP??

	const auto fullPath = fs::path{PathUtil::toAbsolute(loadData.location).value()} / loadData.loadKey;
	if (std::error_code ec; !fs::is_regular_file(fullPath, ec)) { return std::nullopt; }

	const auto extension = fullPath.extension().string();
	auto extensionView = std::string_view{extension};
	if (!extension.empty() && extension[0] == '.') { extensionView.remove_prefix(1); }

	if (auto it = m_filetypeIndexerMap.find(extensionView); it != m_filetypeIndexerMap.end())
	{
		for (auto indexer : it->second)
		{
			auto presets = indexer->query(fullPath.c_str(), PresetMetadata::Flag::UserContent);
			if (!presets || presets->empty()) { continue; } // Hopefully there's another indexer that will work
			if (presets->size() > 1)
			{
				ClapLog::globalLog(CLAP_LOG_HOST_MISBEHAVING,
					"Manually loading multiple presets from a single preset file is not supported yet!"
					"\n\tLoading just the first preset instead."); // TODO: Implement this feature
				return presets->at(0);
			}
			assert(presets->at(0).loadData.loadKey.empty());
			// TODO: If it's working correctly, the preset `location` should be the full path and the `loadKey`
			//       should be nullptr. (??? maybe not when the preset file is a container for many presets?)
			//       (see free-audio/clap-host's PluginHost::loadNativePluginPreset() method)
			return presets->at(0);
		}

		ClapLog::globalLog(CLAP_LOG_ERROR, "Failed to load preset");
		return std::nullopt;
	}

	return std::nullopt;
}

auto ClapPresetDatabase::Indexer::create(const clap_preset_discovery_factory& factory,
	std::uint32_t index) -> std::unique_ptr<Indexer>
{
	const auto desc = factory.get_descriptor(&factory, index);
	if (!desc || !desc->id) { return nullptr; }

	if (!clap_version_is_compatible(desc->clap_version)) { return nullptr; }

	auto indexer = std::make_unique<Indexer>(factory, *desc);
	return indexer->m_provider ? std::move(indexer) : nullptr;
}

ClapPresetDatabase::Indexer::Indexer(const clap_preset_discovery_factory& factory,
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
	std::string tempMsg = "PRESET DISCOVERY PROVIDER DESCRIPTOR ID: " + std::string{descriptor.id};
	ClapLog::globalLog(CLAP_LOG_INFO, tempMsg);

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

	if (!m_provider->init(m_provider.get()))
	{
		ClapLog::globalLog(CLAP_LOG_ERROR, "Failed to initialize preset discovery provider");
		m_provider.reset();
		return;
	}
}

auto ClapPresetDatabase::Indexer::query(std::string_view location, PresetMetadata::Flags flags)
	-> std::optional<std::vector<Preset>>
{
	auto receiver = MetadataReceiver{*this};
	return receiver.query(location, flags);
}

auto ClapPresetDatabase::Indexer::clapDeclareFiletype(const clap_preset_discovery_indexer* indexer,
	const clap_preset_discovery_filetype* filetype) -> bool
{
	if (!indexer || !indexer->indexer_data || !filetype || !filetype->name) { return false; }

	auto self = static_cast<Indexer*>(indexer->indexer_data);
	if (!self) { return false; }

	auto ft = Filetype{};
	ft.name = filetype->name;
	if (filetype->description) { ft.description = filetype->description; }
	if (filetype->file_extension) { ft.extension = filetype->file_extension; }

	qDebug().nospace() << "clapDeclareFiletype: name: \"" << ft.name.c_str() << "\" ext: \"" << ft.extension.c_str() << "\" desc: \"" << ft.description.c_str() << "\"";

	self->m_filetypes.push_back(std::move(ft));
	return true;
}

auto ClapPresetDatabase::Indexer::clapDeclareLocation(const clap_preset_discovery_indexer* indexer,
	const clap_preset_discovery_location* location) -> bool
{
	if (!indexer || !indexer->indexer_data || !location) { return false; }

	auto self = static_cast<Indexer*>(indexer->indexer_data);
	if (!self) { return false; }

	Location loc;
	loc.flags = convertFlags(location->flags);
	loc.kind = static_cast<clap_preset_discovery_location_kind>(location->kind);
	loc.name = location->name ? location->name : std::string{};

	switch (location->kind)
	{
		case CLAP_PRESET_DISCOVERY_LOCATION_PLUGIN:
			if (location->location)
			{
				ClapLog::globalLog(CLAP_LOG_PLUGIN_MISBEHAVING,
					"Preset with PLUGIN location kind must have null location");
				return false;
			}
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
				std::string msg = "Preset location \"" + std::string{location->location} + "\" does not exist";
				ClapLog::globalLog(CLAP_LOG_WARNING, msg);
				return false;
			}
			break;
		}
		default:
			ClapLog::globalLog(CLAP_LOG_PLUGIN_MISBEHAVING, "Invalid preset location kind");
			return false;
	}

	loc.location = fromClapLocation(location->location);

	qDebug().nospace() << "clapDeclareLocation: name: \"" << loc.name.c_str() << "\" loc: \"" << loc.location.c_str() << "\"";

	self->m_locations.push_back(std::move(loc));

	return true;
}

auto ClapPresetDatabase::Indexer::clapDeclareSoundpack(const clap_preset_discovery_indexer* indexer,
	const clap_preset_discovery_soundpack* soundpack) -> bool
{
	// TODO: Implement later?
	return true;
}

auto ClapPresetDatabase::Indexer::clapGetExtension(const clap_preset_discovery_indexer* indexer,
	const char* extensionId) -> const void*
{
	// LMMS does not have any custom indexer extensions
	qDebug().nospace() << "clapGetExtension: Plugin requested extension: \"" << extensionId << "\"";
	return nullptr;
}

ClapPresetDatabase::MetadataReceiver::MetadataReceiver(const Indexer& indexer)
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
	, m_indexer{&indexer}
{
}

auto ClapPresetDatabase::MetadataReceiver::query(std::string_view location, PresetMetadata::Flags flags)
	-> std::optional<std::vector<Preset>>
{
	const auto provider = m_indexer->provider();
	if (!provider) { return std::nullopt; }

	std::string temp;
	auto loc = toClapLocation(location, temp);
	if (!loc) { return std::nullopt; }

	m_location = location;
	m_flags = flags;
	m_presets.clear();
	if (!provider->get_metadata(provider, loc->first, loc->second, &m_receiver))
	{
		std::string msg = "Failed to get metadata from the preset discovery provider";
		if (!errorMessage().empty()) { msg += "; msg=\"" + errorMessage() + "\""; }
		ClapLog::globalLog(CLAP_LOG_ERROR, msg.c_str());
		return std::nullopt;
	}

	return std::move(m_presets);
}

void ClapPresetDatabase::MetadataReceiver::clapOnError(
	const clap_preset_discovery_metadata_receiver* receiver, std::int32_t osError, const char* errorMessage)
{
	if (auto self = from(receiver))
	{
		self->m_error = "{os_error=" + std::to_string(osError) + ", error_message=\""
			+ (errorMessage ? errorMessage : std::string{}) + "\"}";
	}
}

auto ClapPresetDatabase::MetadataReceiver::clapBeginPreset(
	const clap_preset_discovery_metadata_receiver* receiver, const char* name, const char* loadKey) -> bool
{
	if (!loadKey) { return false; }
	auto self = from(receiver);
	if (!self) { return false; }

	auto& preset = self->m_presets.emplace_back();
	preset.metadata.displayName = name ? name : std::string{};
	preset.metadata.flags = self->m_flags; // may be overridden by clapSetFlags()
	preset.loadData.location = self->m_location; // references the preset map's key
	preset.loadData.loadKey = loadKey ? loadKey : std::string{};

	qDebug().nospace() << "clapBeginPreset: display name: \"" << preset.metadata.displayName.c_str() << "\" load key: \"" << preset.loadData.loadKey.c_str() << "\"";

	// TODO

	return true;
}

void ClapPresetDatabase::MetadataReceiver::clapAddPluginId(
	const clap_preset_discovery_metadata_receiver* receiver, const clap_universal_plugin_id* pluginId)
{
	if (!pluginId || !pluginId->abi || !pluginId->id)
	{
		ClapLog::globalLog(CLAP_LOG_WARNING, "Plugin called clap_preset_discovery_metadata_receiver.add_plugin_id() with invalid arguments");
		return;
	}

	if (pluginId->abi != std::string_view{"clap"})
	{
		ClapLog::globalLog(CLAP_LOG_WARNING, "Preset must use the \"clap\" abi");
		// TODO: Remove the preset?
		return;
	}

	auto self = from(receiver);
	if (!self) { return; }

	auto& presets = self->m_presets;
	if (presets.empty())
	{
		ClapLog::globalLog(CLAP_LOG_PLUGIN_MISBEHAVING,
			"Preset discovery provider called add_plugin_id() called before begin_preset()");
		return;
	}

	presets.back().keys.push_back(pluginId->id);

	qDebug().nospace() << "clapAddPluginId: pluginId: \"" << pluginId->id << "\"";
}

void ClapPresetDatabase::MetadataReceiver::clapSetSoundpackId(
	const clap_preset_discovery_metadata_receiver* receiver, const char* soundpackId)
{
	// [UNIMPLEMENTED]
}

void ClapPresetDatabase::MetadataReceiver::clapSetFlags(
	const clap_preset_discovery_metadata_receiver* receiver, std::uint32_t flags)
{
	auto self = from(receiver);
	if (!self) { return; }

	auto& presets = self->m_presets;
	if (presets.empty())
	{
		ClapLog::globalLog(CLAP_LOG_PLUGIN_MISBEHAVING,
			"Preset discovery provider called set_flags() called before begin_preset()");
		return;
	}

	presets.back().metadata.flags = convertFlags(flags);
}

void ClapPresetDatabase::MetadataReceiver::clapAddCreator(
	const clap_preset_discovery_metadata_receiver* receiver, const char* creator)
{
	if (!creator) { return; }
	auto self = from(receiver);
	if (!self) { return; }

	auto& presets = self->m_presets;
	if (presets.empty())
	{
		ClapLog::globalLog(CLAP_LOG_PLUGIN_MISBEHAVING,
			"Preset discovery provider called add_creator() called before begin_preset()");
		return;
	}

	presets.back().metadata.creator = creator;

	qDebug().nospace() << "clapSetCreator: creator: \"" << creator << "\"";
}

void ClapPresetDatabase::MetadataReceiver::clapSetDescription(
	const clap_preset_discovery_metadata_receiver* receiver, const char* description)
{
	if (!description) { return; }
	auto self = from(receiver);
	if (!self) { return; }

	auto& presets = self->m_presets;
	if (presets.empty())
	{
		ClapLog::globalLog(CLAP_LOG_PLUGIN_MISBEHAVING,
			"Preset discovery provider called set_description() called before begin_preset()");
		return;
	}

	presets.back().metadata.description = description;

	qDebug().nospace() << "clapSetDescription: description: \"" << description << "\"";
}

void ClapPresetDatabase::MetadataReceiver::clapSetTimestamps(
	const clap_preset_discovery_metadata_receiver* receiver,
	clap_timestamp creationTime, clap_timestamp modificationTime)
{
	// [UNIMPLEMENTED]
}

void ClapPresetDatabase::MetadataReceiver::clapAddFeature(
	const clap_preset_discovery_metadata_receiver* receiver, const char* feature)
{
	if (!feature) { return; }
	auto self = from(receiver);
	if (!self) { return; }

	auto& presets = self->m_presets;
	if (presets.empty())
	{
		ClapLog::globalLog(CLAP_LOG_PLUGIN_MISBEHAVING,
			"Preset discovery provider called add_feature() called before begin_preset()");
		return;
	}

	presets.back().metadata.categories.push_back(feature);

	qDebug().nospace() << "clapAddFeature: feature: \"" << feature << "\"";
}

void ClapPresetDatabase::MetadataReceiver::clapAddExtraInfo(
	const clap_preset_discovery_metadata_receiver* receiver, const char* key, const char* value)
{
	// [UNIMPLEMENTED]
}

auto ClapPresetDatabase::MetadataReceiver::from(
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
