/*
 * PresetDatabase.cpp - Preset discovery, loading, storage, and query
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

#include "PresetDatabase.h"

#include <algorithm>
#include <cassert>
#include <QDebug>

#include "ConfigManager.h"
#include "FileDialog.h"
#include "lmms_filesystem.h"
#include "PathUtil.h"

namespace lmms
{

auto PresetDatabase::addPreset(std::string_view path) -> const Preset*
{
	if (auto loadData = addFile(path))
	{
		auto& presets = m_presets.find(loadData->location)->second;
		auto it = std::find_if(presets.begin(), presets.end(), [&](const Preset& p) -> bool {
			return *loadData == p.loadData();
		});

		if (it != presets.end())
		{
			qWarning() << "Preset file was already loaded";
			return nullptr;
		}

		auto preset = createPreset(*loadData);
		if (!preset) { return nullptr; }

		const auto [iter, added] = presets.insert(std::move(*preset));
		if (!added) { return nullptr; }

		return &*iter;
	}
	return nullptr;
}


auto PresetDatabase::addLocation(std::string_view path) -> PresetMap::iterator
{
	return m_presets.emplace(PathUtil::toShortestRelative(path), std::set<Preset>{}).first;
}



// TODO: Move to separate GUI-related file?
auto PresetDatabase::openPresetFile(std::string_view previousFile) -> const Preset*
{
	auto openFileDialog = gui::FileDialog(nullptr, QObject::tr("Open audio file"));
	auto dir = !previousFile.empty()
		? PathUtil::toAbsolute(previousFile).value_or("")
		: ConfigManager::inst()->userSamplesDir().toStdString();

	// change dir to position of previously opened file
	openFileDialog.setDirectory(QString::fromUtf8(dir.data(), dir.size()));
	openFileDialog.setFileMode(gui::FileDialog::ExistingFiles);

	// set filters
	auto fileTypes = QStringList{};
	auto allFileTypes = QStringList{};
	auto nameFilters = QStringList{};

	for (const auto& filetype : m_filetypes)
	{
		const auto name = QString::fromStdString(filetype.name);
		const auto extension = QString::fromStdString(filetype.extension);
		const auto displayExtension = QString{"*.%1"}.arg(extension);
		fileTypes.append(QString{"%1 (%2)"}.arg(gui::FileDialog::tr("%1 files").arg(name), displayExtension));
		allFileTypes.append(displayExtension);
	}

	nameFilters.append(QString{"%1 (%2)"}.arg(gui::FileDialog::tr("All preset files"), allFileTypes.join(" ")));
	nameFilters.append(fileTypes);
	nameFilters.append(QString("%1 (*)").arg(gui::FileDialog::tr("Other files")));

	openFileDialog.setNameFilters(nameFilters);

	if (!previousFile.empty())
	{
		// select previously opened file
		openFileDialog.selectFile(QFileInfo{QString::fromUtf8(previousFile.data(), previousFile.size())}.fileName());
	}

	if (openFileDialog.exec() != QDialog::Accepted) { return nullptr; }

	if (openFileDialog.selectedFiles().isEmpty()) { return nullptr; }

	return addPreset(openFileDialog.selectedFiles()[0].toStdString());
}

auto PresetDatabase::savePresetFile(const Preset& preset) -> bool
{
	// [NOT IMPLEMENTED YET]
	return false;
}

auto PresetDatabase::createPreset(const Preset::LoadData& loadData) const -> std::optional<Preset>
{
	// This is the default method - plugins should override this to provide better preset info

	auto preset = Preset{};
	preset.loadData() = loadData;

	auto path = fs::path{loadData.loadKey};
	preset.metadata().displayName = path.filename().string();

	return preset;
}

auto PresetDatabase::createPreset(const Preset::LoadData& loadData,
	const std::vector<std::string>& keys) const -> std::optional<Preset>
{
	auto preset = createPreset(loadData);
	if (!preset) { return std::nullopt; }
	preset->keys() = keys;
	return preset;
}

auto PresetDatabase::findPresets(std::string_view key) const -> std::vector<const Preset*>
{
	std::vector<const Preset*> ret;
	for (const auto& mapPair : m_presets)
	{
		for (const auto& preset : mapPair.second)
		{
			if (preset.supportsPlugin(key))
			{
				ret.push_back(&preset);
			}
		}
	}
	return ret;
}

auto PresetDatabase::findPreset(const PresetLoadData& loadData, std::string_view key) const -> const Preset*
{
	if (auto it = m_presets.find(loadData.location); it != m_presets.end())
	{
		auto it2 = std::find_if(it->second.begin(), it->second.end(), [&](const Preset& p) {
			return p.loadData().loadKey == loadData.loadKey && p.supportsPlugin(key);
		});
		return it2 != it->second.end() ? &*it2 : nullptr; // TODO: Is it2.base() standard?
	}
	return nullptr;
}

auto PresetDatabase::addFile(std::string_view path) -> std::optional<Preset::LoadData>
{
	// First, create shortened path
	const auto shortPath = PathUtil::toShortestRelative(path);

	// Next, check full string
	if (auto it = m_presets.find(shortPath); it != m_presets.end())
	{
		return Preset::LoadData{it->first, ""};
	}

	// Must have at least one directory separator
	auto pos = shortPath.find_last_of("/\\");
	if (pos == std::string::npos) { return std::nullopt; } // error

	// Next, split input into each possible (location, loadKey) pair starting from right end
	auto locationView = std::string_view{shortPath};
	auto loadKeyView = std::string_view{shortPath};
	do
	{
		locationView = std::string_view{shortPath};
		loadKeyView = std::string_view{shortPath};
		locationView.remove_suffix(locationView.size() - pos);
		loadKeyView.remove_prefix(pos + 1);
		if (auto it = m_presets.find(locationView); it != m_presets.end())
		{
			return Preset::LoadData{it->first, std::string{loadKeyView}};
		}

		if (pos == 0) { break; }
		pos = shortPath.find_last_of("/\\", pos - 1);
	} while (pos != std::string::npos);

	// Did not find the file in `m_presets` - check just the PathUtil::Base
	auto [base, loadKey] = PathUtil::parsePath(shortPath);
	if (base != PathUtil::Base::Absolute)
	{
		if (auto it = m_presets.find(PathUtil::basePrefix(base)); it != m_presets.end())
		{
			return Preset::LoadData{it->first, std::string{loadKey}};
		}
	}

	auto ret = m_presets.emplace(locationView, std::set<Preset>{});
	assert(ret.second);

	return Preset::LoadData{ret.first->first, std::string{loadKeyView}};
}

} // namespace lmms
