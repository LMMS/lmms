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

#include <QObject>
#include <memory>

#include "PresetDatabase.h"

namespace lmms
{

namespace gui
{
	class PluginPresetsView {}; // TODO: Add later
} // namespace gui

/**
 * A collection of referenced plugins sourced from a PresetDatabase.
 */
class PluginPresets : public QObject
{
	Q_OBJECT
public:
	using QObject::QObject;
	PluginPresets(QObject* parent, PresetDatabase* database,
		std::string_view pluginKey = std::string_view{});
	virtual ~PluginPresets() = default;

	auto setPresetDatabase(PresetDatabase* database) -> bool;
	auto refreshPresetCollection() -> bool;

	auto activatePreset(const PresetLoadData& preset) -> bool;

	auto activatePreset(std::size_t index) -> bool;
	auto presetIndex() const -> std::optional<std::size_t>;

	virtual auto createView() const -> std::unique_ptr<gui::PluginPresetsView>
	{
		// TODO: Implement later
		return std::make_unique<gui::PluginPresetsView>();
	}

	//! Open preset dialog TODO: Move to an lmms::gui class?
	//auto openPresetFile(std::string_view previousFile) -> const Preset*;

	//! Save preset dialog TODO: Move to an lmms::gui class?
	//auto savePresetFile(const Preset& preset) -> bool;

signals:

	void activePresetChanged(const Preset* preset);
	void presetCollectionChanged();

	// TODO: Should connect PresetDatabase's dataChanged() to this object's presetCollectionChanged().
	//       This way if there are two plugin instances, and a user manually loads a new preset
	//       in instance #1, it will be added to the preset collection that both PluginPresets share,
	//       then presetCollectionChanged() will be called, and both objects will update their lists.

public:

	void presetModified();

	auto nextPreset() -> bool;
	auto prevPreset() -> bool;

protected:
	virtual auto activatePresetImpl(const PresetLoadData& preset) noexcept -> bool = 0;

	void setActivePreset(std::optional<std::size_t> index);

	auto findPreset(const PresetLoadData& preset) const -> std::optional<std::size_t>;
	auto preset(std::size_t index) const -> const Preset*;

private:

	PresetDatabase* m_database = nullptr;

	//! Non-empty if this is a subplugin; Used to find compatible presets in database
	std::string m_pluginKey;

	//! A subset of presets from `m_database`
	std::vector<const Preset*> m_presets;

	//! Preset and `m_preset` index pair
	std::optional<std::pair<const Preset*, std::size_t>> m_activePreset;

	//! Whether the active preset has been modified
	bool m_modified = false;
};

} // namespace lmms

#endif // LMMS_PLUGIN_PRESETS_H
