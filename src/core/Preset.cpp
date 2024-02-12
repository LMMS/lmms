/*
 * Preset.cpp - Classes for handling plugin presets
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

#include "Preset.h"

#include <algorithm>
#include <cassert>

#include "ConfigManager.h"
#include "FileDialog.h"
#include "lmms_filesystem.h"
#include "PathUtil.h"

namespace lmms
{

/*
PluginPresets::PluginPresets(const Plugin::Descriptor::SubPluginFeatures::Key* key)
	: m_key{key}
{
}
*/

auto PluginPresets::addInternalLocation() -> PresetMap::iterator
{
	return m_presets.emplace(PathUtil::basePrefix(PathUtil::Base::Internal), std::vector<Preset>{}).first;
}

auto PluginPresets::addLocation(std::string_view path) -> PresetMap::iterator
{
	return m_presets.emplace(PathUtil::toShortestRelative(path), std::vector<Preset>{}).first;
}

// TODO: Move to separate GUI-related file?
auto PluginPresets::openPresetFile(std::string_view previousFile) -> const Preset*
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
	//const auto& supportedAudioTypes = SampleDecoder::supportedAudioTypes();

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

	// Checks all possibilities for `location` to see if this preset has already been added to `m_presets`
	auto findInPresets = [&](const std::string& input) -> std::optional<PresetLoadData> {
		// First, check full string
		if (auto it = m_presets.find(input); it != m_presets.end())
		{
			return PresetLoadData{it->first, ""};
		}

		// Must have at least one directory separator
		auto pos = input.find_last_of("/\\");
		if (pos == std::string::npos) { return std::nullopt; } // error

		// Next, split input into each possible (location, loadKey) pair starting from right end
		auto locationView = std::string_view{input};
		auto loadKeyView = std::string_view{input};
		do
		{
			locationView = std::string_view{input};
			loadKeyView = std::string_view{input};
			locationView.remove_suffix(locationView.size() - pos);
			loadKeyView.remove_prefix(pos + 1);
			if (auto it = m_presets.find(locationView); it != m_presets.end())
			{
				return PresetLoadData{it->first, std::string{loadKeyView}};
			}

			if (pos == 0) { break; }
			pos = input.find_last_of("/\\", pos - 1);
		} while (pos != std::string::npos);

		// Did not find the file in `m_presets` - check just the PathUtil::Base
		auto [base, loadKey] = PathUtil::parsePath(input);
		if (base != PathUtil::Base::Absolute)
		{
			if (auto it = m_presets.find(PathUtil::basePrefix(base)); it != m_presets.end())
			{
				return PresetLoadData{it->first, std::string{loadKey}};
			}
		}

		auto ret = m_presets.emplace(locationView, std::vector<Preset>{});
		assert(ret.second);

		return PresetLoadData{ret.first->first, std::string{loadKeyView}};
	};

	const auto shortestRelative = PathUtil::toShortestRelative(openFileDialog.selectedFiles()[0]).toStdString();
	if (auto it = findInPresets(shortestRelative))
	{
		auto& presets = m_presets.find(it->location)->second;
		auto it2 = std::find_if(presets.begin(), presets.end(), [&](const Preset& p) -> bool {
			return *it == p.loadData;
		});

		if (it2 != presets.end())
		{
			// The preset was already loaded
			return nullptr;
		}

		return &presets.emplace_back(queryPreset(*it));
	}
	else { return nullptr; }
}

auto PluginPresets::savePresetFile(const Preset& preset) -> bool
{
	// [NOT IMPLEMENTED YET]
	return false;
}

auto PluginPresets::queryPreset(const PresetLoadData& loadData,
	const Plugin::Descriptor::SubPluginFeatures::Key* key) const -> Preset
{
	// This is the default method - plugins should override this to provide better preset info

	auto preset = Preset{};
	preset.loadData = loadData;

	auto path = fs::path{loadData.loadKey};
	preset.displayName = path.filename().string();

	preset.key = key;

	return preset;
}

} // namespace lmms
