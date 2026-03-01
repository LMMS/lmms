/*
 * PresetSelector.h - A preset selector widget
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

#ifndef LMMS_GUI_PRESET_SELECTOR_H
#define LMMS_GUI_PRESET_SELECTOR_H

#include <QToolBar>

#include "lmms_export.h"
#include "AutomatableModelView.h"

class QLabel;
class QPushButton;

namespace lmms
{

class PluginPresets;

namespace gui
{

class PixmapButton;

class LMMS_EXPORT PresetSelector : public QToolBar, public IntModelView
{
	Q_OBJECT
public:
	PresetSelector(PluginPresets* presets, QWidget* parent = nullptr);

	auto sizeHint() const -> QSize override;

protected slots:
	void updateActivePreset();
	void updateMenu();
	void loadPreset();
	void savePreset();
	void selectPreset(int pos);

private:

	PluginPresets* m_presets = nullptr;
	int m_lastPosInMenu = 0;

	QLabel* m_activePreset = nullptr;
	PixmapButton* m_prevPresetButton = nullptr;
	PixmapButton* m_nextPresetButton = nullptr;
	QPushButton* m_selectPresetButton = nullptr;
	PixmapButton* m_loadPresetButton = nullptr;
	PixmapButton* m_savePresetButton = nullptr;
};

} // namespace gui

} // namespace lmms

#endif // LMMS_GUI_PRESET_SELECTOR_H
