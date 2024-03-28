/*
 * PluginPortConfigSelector.h - PluginPortConfig selector
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

#ifndef LMMS_GUI_PLUGIN_PORT_CONFIG_SELECTOR_H
#define LMMS_GUI_PLUGIN_PORT_CONFIG_SELECTOR_H

#include <QWidget>

#include "lmms_export.h"
#include "PluginPortConfig.h"

class QComboBox;

namespace lmms
{

class PluginPortConfig;

namespace gui
{

class LMMS_EXPORT PluginPortConfigSelector
	: public QWidget
{
	Q_OBJECT

public:
	PluginPortConfigSelector(PluginPortConfig* config, QWidget* parent = nullptr);

	auto sizeHint() const -> QSize override;

protected slots:
	void onIndexChanged(int index);
	void updateOptions();
	void updateSelection();

private:
	PluginPortConfig* m_config = nullptr;

	QComboBox* m_comboBox = nullptr;
};

} // namespace gui

} // namespace lmms

#endif // LMMS_GUI_PLUGIN_PORT_CONFIG_SELECTOR_H
