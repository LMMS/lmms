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

#include <QDebug>

#include <algorithm>
#include <cassert>
#include <iterator>

namespace lmms
{

PluginPresets::PluginPresets(Model* parent, PresetDatabase* database, std::string_view pluginKey)
	: LinkedModelGroup{parent}
	, m_pluginKey{pluginKey}
{
	setPresetDatabase(database);
	// TODO: Connect preset database changed signal to refreshPresetCollection()
}

auto PluginPresets::setPresetDatabase(PresetDatabase* database) -> bool
{
	if (!database) { return false; }

	m_database = database;
	m_presets = database->findPresets(m_pluginKey);

	m_activePresetModel.setRange(-1, m_presets.size() - 1);

	emit presetCollectionChanged();

	setActivePreset(std::nullopt);
	return true;
}

auto PluginPresets::refreshPresetCollection() -> bool
{
	if (!m_database) { return false; }

	std::optional<PresetLoadData> loadData;
	if (m_activePreset) { loadData = m_presets.at(*m_activePreset)->loadData(); }

	m_presets = m_database->findPresets(m_pluginKey);

	m_activePresetModel.setRange(-1, m_presets.size() - 1);

	// Find the previously active preset in the new presets vector
	if (loadData)
	{
		const auto newIndex = findPreset(*loadData);
		if (!newIndex)
		{
			// Failed to find preset in new vector
			setActivePreset(std::nullopt);
			return false;
		}

		m_activePreset = newIndex;
		m_activePresetModel.setValue(*newIndex);
	}
	else
	{
		m_activePresetModel.setValue(-1);
	}

	emit presetCollectionChanged();

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
	if (!m_database || index >= m_presets.size()) { return false; }

	const auto preset = m_presets[index];
	assert(preset != nullptr);

	// TODO: Check if current preset has been modified? (In case user wants to save it)

	if (!activatePresetImpl(preset->loadData())) { return false; }

	setActivePreset(index);

	return true;
}

auto PluginPresets::prevPreset() -> bool
{
	if (!m_activePreset) { return activatePreset(0); }

	if (m_presets.empty()) { return false; }
	const auto newIndex = *m_activePreset != 0
		? (*m_activePreset - 1) % m_presets.size()
		: m_presets.size() - 1;

	return activatePreset(newIndex);
}

auto PluginPresets::nextPreset() -> bool
{
	if (!m_activePreset) { return activatePreset(0); }

	if (m_presets.empty()) { return false; }
	return activatePreset((*m_activePreset + 1) % m_presets.size());
}

auto PluginPresets::activePreset() const -> const Preset*
{
	if (!m_activePreset) { return nullptr; }
	return m_presets.at(*m_activePreset);
}

void PluginPresets::saveActivePreset(QDomDocument& doc, QDomElement& element)
{
	if (auto preset = activePreset())
	{
		const auto& [location, loadKey] = preset->loadData();

		QDomElement presetElement = doc.createElement(presetNodeName());
		element.appendChild(presetElement);
#if 0
		qDebug().nospace() << "Saving active preset: location: \"" << location.data() << "\" loadKey: \""
			<< loadKey.data() << "\"";
#endif
		presetElement.setAttribute("location", QString::fromUtf8(location.data(), location.size()));
		presetElement.setAttribute("loadKey", QString::fromUtf8(loadKey.data(), loadKey.size()));
	}
}

void PluginPresets::loadActivePreset(const QDomElement& element)
{
	QDomElement presetElement = element.firstChildElement(presetNodeName());
	if (presetElement.isNull())
	{
		setActivePreset(std::nullopt);
		return;
	}

	const auto location = presetElement.attribute("location");
	const auto loadKey = presetElement.attribute("loadKey");

	if (location.isEmpty() && loadKey.isEmpty())
	{
		setActivePreset(std::nullopt);
		return;
	}

#if 0
	qDebug().nospace() << "Loading active preset: location: \"" << location.data() << "\" loadKey: \""
			<< loadKey.data() << "\"";
#endif

	// TODO: The needed preset may not be discovered at this point

	const auto loadData = PresetLoadData{location.toStdString(), loadKey.toStdString()};
	if (auto index = findPreset(loadData))
	{
		if (!activatePreset(*index))
		{
			qWarning() << "Failed to load preset!";
		}
	}
	else
	{
		qWarning() << "Failed to find preset!";
	}
}

void PluginPresets::setActivePreset(std::optional<std::size_t> index)
{
	const auto oldIndex = m_activePreset;
	m_activePreset = index;
	m_activePresetModel.setValue(index ? *index : -1);
	m_modified = false;

	if (index != oldIndex)
	{
		emit activePresetChanged();
	}
}

auto PluginPresets::findPreset(const PresetLoadData& preset) const -> std::optional<std::size_t>
{
	const auto it = std::find_if(m_presets.begin(), m_presets.end(), [&](const Preset* p) {
		return p && p->loadData() == preset;
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
