/*
 * PluginPresets.cpp - Preset collection and functionality for a plugin instance
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

#include "PluginPresets.h"

#include <algorithm>
#include <cassert>
#include <iterator>

namespace lmms
{

PluginPresets::PluginPresets(QObject* parent, PresetDatabase* database, std::string_view pluginKey)
	: QObject{parent}
	, m_pluginKey{pluginKey}
{
	setPresetDatabase(database);
}

auto PluginPresets::setPresetDatabase(PresetDatabase* database) -> bool
{
	if (!database) { return false; }

	m_database = database;
	m_presets = database->findPresets(m_pluginKey);
	setActivePreset(std::nullopt);

	return true;
}

auto PluginPresets::activatePreset(const PresetLoadData& preset) -> bool
{
	if (auto index = findPreset(preset))
	{
		return activatePreset(*index);
	}
	return false;
}

auto PluginPresets::activatePreset(std::size_t index) -> bool
{
	if (!m_database || m_presets.empty()) { return false; }
	assert(index < m_presets.size());

	const auto preset = m_presets[index];
	assert(preset != nullptr);

	// TODO: Check if current preset has been modified? (In case user wants to save it)

	if (!activatePresetImpl(preset->loadData)) { return false; }

	setActivePreset(index);

	return true;
}

auto PluginPresets::presetIndex() const -> std::optional<std::size_t>
{
	return m_activePreset
		? std::optional<std::size_t>{m_activePreset->second}
		: std::nullopt;
}

auto PluginPresets::nextPreset() -> bool
{
	if (!m_activePreset) { return activatePreset(0); }

	if (m_presets.empty()) { return false; }
	return activatePreset((m_activePreset->second + 1) % m_presets.size());
}

auto PluginPresets::prevPreset() -> bool
{
	if (!m_activePreset) { return activatePreset(0); }

	if (m_presets.empty()) { return false; }
	const auto newIndex = m_activePreset->second != 0
		? (m_activePreset->second - 1) % m_presets.size()
		: m_presets.size() - 1;

	return activatePreset(newIndex);
}

void PluginPresets::setActivePreset(std::optional<std::size_t> index)
{
	std::optional<std::size_t> oldIndex;
	if (m_activePreset) { oldIndex = m_activePreset->second; }

	const Preset* newPreset = nullptr;
	if (index)
	{
		newPreset = preset(*index);
		m_activePreset.emplace(newPreset, *index);
	}
	else
	{
		m_activePreset.reset();
	}

	m_modified = false;

	if (index != oldIndex)
	{
		emit activePresetChanged(newPreset);
	}
}

auto PluginPresets::findPreset(const PresetLoadData& preset) const -> std::optional<std::size_t>
{
	const auto it = std::find_if(m_presets.begin(), m_presets.end(), [&](const Preset* p) {
		return p && p->loadData == preset;
	});

	if (it == m_presets.end()) { return std::nullopt; }
	return std::distance(m_presets.begin(), it);
}

auto PluginPresets::preset(std::size_t index) const -> const Preset*
{
	if (index >= m_presets.size()) { return nullptr; }
	return m_presets[index];
}

} // namespace lmms
