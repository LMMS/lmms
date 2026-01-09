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

#include <QObject>
#include <cassert>
#include <filesystem>
#include <clap/entry.h>

#include "ClapLog.h"
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

class ClapPresetDatabase::MetadataReceiver
{
public:
	MetadataReceiver() = delete;
	MetadataReceiver(const Indexer& indexer);

	//! For PLUGIN presets
	auto query(PresetMetadata::Flags flags = PresetMetadata::Flag::None)
		-> std::optional<std::vector<Preset>>;

	//! For FILE presets; `file` is the full path of the preset file
	auto query(std::string_view file,
		PresetMetadata::Flags flags = PresetMetadata::Flag::None) -> std::optional<std::vector<Preset>>;

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
	bool m_skip = false; //!< Used to ignore remaining metadata if the current preset is invalid
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

auto ClapPresetDatabase::discoverSetup() -> bool
{
	if (!m_factory)
	{
		// This .clap file doesn't provide preset support
		return false;
	}

	const auto providerCount = m_factory->count(m_factory);
	if (providerCount == 0) { return false; }

	bool success = false;
	for (std::uint32_t idx = 0; idx < providerCount; ++idx)
	{
		if (auto indexer = Indexer::create(*m_factory, idx))
		{
			success = true;
			auto& indexerRef = m_indexers.emplace_back(std::move(indexer));
			for (auto& filetype : indexerRef->filetypes())
			{
				m_filetypeIndexerMap[filetype.extension].push_back(indexerRef.get());
			}
		}
		else
		{
			ClapLog::globalLog(CLAP_LOG_WARNING, "Failed to create preset indexer");
		}
	}

	return success;
}

auto ClapPresetDatabase::discoverFiletypes(std::vector<Filetype>& filetypes) -> bool
{
	for (const auto& indexer : m_indexers)
	{
		for (const auto& filetype : indexer->filetypes())
		{
			filetypes.push_back(filetype);
		}
	}
	return true;
}

auto ClapPresetDatabase::discoverLocations(const SetLocations& func) -> bool
{
	for (const auto& indexer : m_indexers)
	{
		for (const auto& location : indexer->locations())
		{
			func(location);
		}
	}
	return true;
}

auto ClapPresetDatabase::discoverPresets(const Location& location, std::set<Preset>& presets) -> bool
{
	const auto preferredIndexer = getIndexerFor(location);

	// Handle PLUGIN presets (internal presets)
	if (PathUtil::hasBase(location.location, PathUtil::Base::Internal))
	{
		if (!preferredIndexer)
		{
			ClapLog::globalLog(CLAP_LOG_ERROR, "No known indexer supports internal presets");
			return false;
		}

		auto newPresets = preferredIndexer->query(location.flags);
		if (!newPresets) { return false; }
		for (auto& preset : *newPresets)
		{
			presets.insert(std::move(preset));
		}

		return true;
	}

	// Presets must be FILE presets
	const auto fullPath = PathUtil::toAbsolute(location.location).value_or("");
	if (fullPath.empty())
	{
		// Shouldn't ever happen?
		ClapLog::globalLog(CLAP_LOG_ERROR, "Failed to get absolute path for preset directory");
		return false;
	}

	// First handle case where location is a file
	if (std::error_code ec; std::filesystem::is_regular_file(fullPath, ec))
	{
		// Use preferred indexer if possible
		if (preferredIndexer)
		{
			auto newPresets = preferredIndexer->query(fullPath, location.flags);
			if (!newPresets) { return false; }
			for (auto& preset : *newPresets)
			{
				presets.insert(std::move(preset));
			}
			return true;
		}

		// Else, just try whichever indexer works
		bool success = false;
		for (const auto& indexer : m_indexers)
		{
			auto newPresets = indexer->query(fullPath, location.flags);
			if (!newPresets) { continue; }
			success = true;
			for (auto& preset : *newPresets)
			{
				presets.insert(std::move(preset));
			}
		}
		return success;
	}

	// Location is a directory - need to search for preset files
	if (std::error_code ec; !std::filesystem::is_directory(fullPath, ec))
	{
		ClapLog::globalLog(CLAP_LOG_WARNING, "Preset directory \"" + fullPath + "\" does not exist");
		return false;
	}

	// TODO: Move to Indexer?
	auto getPresets = [&](Indexer& indexer) -> bool {
		bool success = false;

		for (const auto& entry : std::filesystem::recursive_directory_iterator{fullPath})
		{
			const auto entryPath = entry.path().string();

			if (std::error_code ec; !entry.is_regular_file(ec)) { continue; }

			if (!indexer.filetypeSupported(entryPath)) { continue; }

			auto newPresets = indexer.query(entryPath, location.flags);
			if (!newPresets) { continue; }
			success = true;
			for (auto& preset : *newPresets)
			{
				presets.insert(std::move(preset));
			}
		}

		return success;
	};


	// TODO: Use m_filetypeIndexerMap instead of preferredIndexer?

	// Use preferred indexer if possible
	if (preferredIndexer)
	{
		return getPresets(*preferredIndexer);
	}

	// Else, just try whichever indexer works
	bool success = false;
	for (const auto& indexer : m_indexers)
	{
		success = getPresets(*indexer) || success;
	}
	return success;
}

auto ClapPresetDatabase::loadPresets(const Location& location, std::string_view file,
	std::set<Preset>& presets) -> std::vector<const Preset*>
{
	const auto filePath = PathUtil::u8path(file);
	if (std::error_code ec; !std::filesystem::is_regular_file(filePath, ec)) { return {}; }

	auto getPresets = [&](Indexer& indexer) -> std::vector<const Preset*> {
		if (!indexer.filetypeSupported(filePath)) { return {}; }

		auto newPresets = indexer.query(file, location.flags);
		if (!newPresets) { return {}; }

		std::vector<const Preset*> results;
		for (auto& preset : *newPresets)
		{
			auto [it, added] = presets.emplace(std::move(preset));
			if (added)
			{
				results.push_back(&*it);
			}
			else
			{
				std::string msg = "The preset \"" + it->metadata().displayName + "\" was already loaded";
				ClapLog::globalLog(CLAP_LOG_WARNING, msg);
			}
		}
		return results;
	};

	if (const auto preferredIndexer = getIndexerFor(location))
	{
		return getPresets(*preferredIndexer);
	}

	std::vector<const Preset*> results;
	for (const auto& indexer : m_indexers)
	{
		results = getPresets(*indexer);
		if (!results.empty()) { break; }
	}

	return results;
}

auto ClapPresetDatabase::getIndexerFor(const Location& location) const -> Indexer*
{
	for (const auto& indexer : m_indexers)
	{
		const auto& locations = indexer->locations();
		const auto it = std::find(locations.begin(), locations.end(), location);
		if (it == locations.end()) { return nullptr; }
		return indexer.get(); // exact match
	}

	return nullptr;
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
	const char* location, const char* loadKey) -> std::optional<PresetLoadData>
{
	if (!location && !loadKey) { return std::nullopt; }

	return PresetLoadData {
		fromClapLocation(location),
		loadKey ? loadKey : std::string{}
	};
}

auto ClapPresetDatabase::Indexer::create(const clap_preset_discovery_factory& factory, std::uint32_t index)
	-> std::unique_ptr<Indexer>
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

auto ClapPresetDatabase::Indexer::query(PresetMetadata::Flags flags)
	-> std::optional<std::vector<Preset>>
{
	auto receiver = MetadataReceiver{*this};
	return receiver.query(flags);
}

auto ClapPresetDatabase::Indexer::query(std::string_view file, PresetMetadata::Flags flags)
	-> std::optional<std::vector<Preset>>
{
	auto receiver = MetadataReceiver{*this};
	return receiver.query(file, flags);
}

auto ClapPresetDatabase::Indexer::filetypeSupported(const std::filesystem::path& path) const -> bool
{
	if (m_filetypes.empty() || m_filetypes[0].extension.empty()) { return true; }

	const auto extension = path.extension().string();
	if (extension.empty()) { return false; }

	auto extView = std::string_view{extension};
	extView.remove_prefix(1); // remove dot
	return std::find_if(m_filetypes.begin(), m_filetypes.end(),
		[&](const Filetype& f) { return f.extension == extView; }) != m_filetypes.end();
}

auto ClapPresetDatabase::Indexer::clapDeclareFiletype(const clap_preset_discovery_indexer* indexer,
	const clap_preset_discovery_filetype* filetype) -> bool
{
	if (!indexer || !filetype)
	{
		ClapLog::globalLog(CLAP_LOG_PLUGIN_MISBEHAVING, "Preset provider declared a file type incorrectly");
		return false;
	}

	auto self = static_cast<Indexer*>(indexer->indexer_data);
	if (!self)
	{
		ClapLog::globalLog(CLAP_LOG_PLUGIN_MISBEHAVING, "Plugin modified the preset indexer's context pointer");
		return false;
	}

	auto ft = Filetype{};
	ft.name = filetype->name ? filetype->name : QObject::tr("Preset").toStdString();
	if (filetype->description) { ft.description = filetype->description; }
	if (filetype->file_extension) { ft.extension = filetype->file_extension; }

	self->m_filetypes.push_back(std::move(ft));
	return true;
}

auto ClapPresetDatabase::Indexer::clapDeclareLocation(const clap_preset_discovery_indexer* indexer,
	const clap_preset_discovery_location* location) -> bool
{
	if (!indexer || !location)
	{
		ClapLog::globalLog(CLAP_LOG_PLUGIN_MISBEHAVING, "Preset provider declared a location incorrectly");
		return false;
	}

	auto self = static_cast<Indexer*>(indexer->indexer_data);
	if (!self)
	{
		ClapLog::globalLog(CLAP_LOG_PLUGIN_MISBEHAVING, "Plugin modified the preset indexer's context pointer");
		return false;
	}

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

			// A FILE location could be a directory or a file
			if (std::error_code ec; !std::filesystem::exists(location->location, ec))
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

	auto loc = Location {
		location->name ? location->name : std::string{},
		fromClapLocation(location->location),
		convertFlags(location->flags)
	};

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

auto ClapPresetDatabase::MetadataReceiver::query(PresetMetadata::Flags flags)
	-> std::optional<std::vector<Preset>>
{
	const auto provider = m_indexer->provider();
	if (!provider) { return std::nullopt; }

	m_location = PathUtil::basePrefix(PathUtil::Base::Internal);
	m_flags = flags;
	m_presets.clear();
	if (!provider->get_metadata(provider, CLAP_PRESET_DISCOVERY_LOCATION_PLUGIN, nullptr, &m_receiver))
	{
		std::string msg = "Failed to get metadata from the preset discovery provider";
		if (!errorMessage().empty()) { msg += ": " + errorMessage(); }
		ClapLog::globalLog(CLAP_LOG_ERROR, msg.c_str());
		return std::nullopt;
	}

	return std::move(m_presets);
}

auto ClapPresetDatabase::MetadataReceiver::query(std::string_view file,
	PresetMetadata::Flags flags) -> std::optional<std::vector<Preset>>
{
	const auto provider = m_indexer->provider();
	if (!provider) { return std::nullopt; }

	/*
	 * For preset files, PresetLoadData will usually be { file, "" },
	 * but if the preset file contains multiple presets, it will be { file, loadKey }.
	 */

	m_location = file;
	m_flags = flags;
	m_presets.clear();
	if (!provider->get_metadata(provider, CLAP_PRESET_DISCOVERY_LOCATION_FILE, file.data(), &m_receiver))
	{
		std::string msg = "Failed to get metadata from the preset discovery provider";
		if (!errorMessage().empty()) { msg += ": " + errorMessage(); }
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
		self->m_error = "{ os_error=" + std::to_string(osError) + ", error_message=\""
			+ (errorMessage ? errorMessage : std::string{}) + "\" }";
	}
}

auto ClapPresetDatabase::MetadataReceiver::clapBeginPreset(
	const clap_preset_discovery_metadata_receiver* receiver, const char* name, const char* loadKey) -> bool
{
	if (!loadKey) { return false; }
	auto self = from(receiver);
	if (!self) { return false; }

	self->m_skip = false;

	auto& preset = self->m_presets.emplace_back();
	preset.metadata().displayName = name ? name : std::string{};
	preset.metadata().flags = self->m_flags; // may be overridden by clapSetFlags()
	preset.loadData().location = self->m_location;
	preset.loadData().loadKey = loadKey ? loadKey : std::string{};

	return true;
}

void ClapPresetDatabase::MetadataReceiver::clapAddPluginId(
	const clap_preset_discovery_metadata_receiver* receiver, const clap_universal_plugin_id* pluginId)
{
	if (!pluginId || !pluginId->abi || !pluginId->id)
	{
		ClapLog::globalLog(CLAP_LOG_WARNING,
			"Plugin called clap_preset_discovery_metadata_receiver.add_plugin_id() with invalid arguments");
		return;
	}

	auto self = from(receiver);
	if (!self || self->m_skip) { return; }

	auto& presets = self->m_presets;
	if (presets.empty())
	{
		ClapLog::globalLog(CLAP_LOG_PLUGIN_MISBEHAVING,
			"Preset discovery provider called add_plugin_id() called before begin_preset()");
		return;
	}

	if (pluginId->abi != std::string_view{"clap"})
	{
		ClapLog::globalLog(CLAP_LOG_WARNING, "Preset must use the \"clap\" abi");

		// Remove the current preset
		self->m_skip = true; // Skip any more metadata calls until next begin_preset()
		presets.pop_back();
		return;
	}

	presets.back().keys().push_back(pluginId->id);
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
	if (!self || self->m_skip) { return; }

	auto& presets = self->m_presets;
	if (presets.empty())
	{
		ClapLog::globalLog(CLAP_LOG_PLUGIN_MISBEHAVING,
			"Preset discovery provider called set_flags() called before begin_preset()");
		return;
	}

	presets.back().metadata().flags = convertFlags(flags);
}

void ClapPresetDatabase::MetadataReceiver::clapAddCreator(
	const clap_preset_discovery_metadata_receiver* receiver, const char* creator)
{
	if (!creator) { return; }
	auto self = from(receiver);
	if (!self || self->m_skip) { return; }

	auto& presets = self->m_presets;
	if (presets.empty())
	{
		ClapLog::globalLog(CLAP_LOG_PLUGIN_MISBEHAVING,
			"Preset discovery provider called add_creator() called before begin_preset()");
		return;
	}

	presets.back().metadata().creator = creator;
}

void ClapPresetDatabase::MetadataReceiver::clapSetDescription(
	const clap_preset_discovery_metadata_receiver* receiver, const char* description)
{
	if (!description) { return; }
	auto self = from(receiver);
	if (!self || self->m_skip) { return; }

	auto& presets = self->m_presets;
	if (presets.empty())
	{
		ClapLog::globalLog(CLAP_LOG_PLUGIN_MISBEHAVING,
			"Preset discovery provider called set_description() called before begin_preset()");
		return;
	}

	presets.back().metadata().description = description;
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
	if (!self || self->m_skip) { return; }

	auto& presets = self->m_presets;
	if (presets.empty())
	{
		ClapLog::globalLog(CLAP_LOG_PLUGIN_MISBEHAVING,
			"Preset discovery provider called add_feature() called before begin_preset()");
		return;
	}

	presets.back().metadata().categories.push_back(feature);
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
