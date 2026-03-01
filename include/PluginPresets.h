/*
 * PluginPresets.h - Preset collection and functionality for a plugin instance
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

#ifndef LMMS_PLUGIN_PRESETS_H
#define LMMS_PLUGIN_PRESETS_H

#include <QDomDocument>
#include <QDomElement>
#include <QObject>
#include <memory>

#include "AutomatableModel.h"
#include "LinkedModelGroups.h"
#include "lmms_export.h"
#include "PresetDatabase.h"

namespace lmms
{

/**
 * A collection of referenced presets sourced from a PresetDatabase
 */
class LMMS_EXPORT PluginPresets : public LinkedModelGroup
{
	Q_OBJECT
public:
	PluginPresets(Model* parent, PresetDatabase* database,
		std::string_view pluginKey = std::string_view{});
	virtual ~PluginPresets() = default;

	auto setPresetDatabase(PresetDatabase* database) -> bool;
	auto refreshPresetCollection() -> bool;

	auto activatePreset(const PresetLoadData& preset) -> bool;
	auto activatePreset(std::size_t index) -> bool;
	auto prevPreset() -> bool;
	auto nextPreset() -> bool;

	auto presetDatabase() const -> PresetDatabase* { return m_database; }
	auto presets() const -> const auto& { return m_presets; }
	auto presetIndex() const { return m_activePreset; }
	auto isModified() const { return m_modified; }
	auto activePreset() const -> const Preset*;
	auto activePresetModel() -> IntModel* { return &m_activePresetModel; }

	/**
	 * SerializingObject-like functionality
	 *
	 * The default implementation just saves/loads the PresetLoadKey
	 * and ignores any modifications that may have been made.
	 */
	virtual auto presetNodeName() const -> QString { return "preset"; } // TODO: use "preset#" for linked preset groups?
	virtual void saveActivePreset(QDomDocument& doc, QDomElement& element);
	virtual void loadActivePreset(const QDomElement& element);

	/**
	 * Signals
	 */
signals:
	void activePresetChanged();
	void activePresetModified();
	void presetCollectionChanged();

	// TODO: Should connect PresetDatabase's dataChanged() to this object's presetCollectionChanged().
	//       This way if there are two plugin instances, and a user manually loads a new preset
	//       in instance #1, it will be added to the preset collection that both PluginPresets share,
	//       then presetCollectionChanged() will be called, and both objects will update their lists.

protected:
	virtual auto activatePresetImpl(const PresetLoadData& preset) noexcept -> bool = 0;

	void setActivePreset(std::optional<std::size_t> index);

	auto findPreset(const PresetLoadData& preset) const -> std::optional<std::size_t>;
	auto preset(std::size_t index) const -> const Preset*;

private:
	//! The source of the plugin's presets
	PresetDatabase* m_database = nullptr;

	//! Non-empty if this is a subplugin; Used to find compatible presets in database
	std::string m_pluginKey;

	//! A subset of presets from `m_database`
	std::vector<const Preset*> m_presets;

	//! `m_preset` index of the active preset
	std::optional<std::size_t> m_activePreset;

	IntModel m_activePresetModel; // TODO: Remove?

	//! Whether the active preset has been modified
	bool m_modified = false;
};

} // namespace lmms

#endif // LMMS_PLUGIN_PRESETS_H
