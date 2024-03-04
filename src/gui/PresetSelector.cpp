/*
 * PresetSelector.cpp - A simple preset selector for PluginPresets
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

// TODO: Use this for VST instruments/effects

#include "PresetSelector.h"

#include <QMenu>
#include <QPushButton>
#include <QToolBar>

#include "embed.h"
#include "PixmapButton.h"
#include "PluginPresets.h"

namespace lmms::gui
{

PresetSelector::PresetSelector(PluginPresets* presets, QWidget* parent)
	: QWidget{parent}
	, IntModelView{presets, parent}
	, m_presets{presets}
{
	m_prevPresetButton = new PixmapButton{this};
	m_prevPresetButton->setCheckable(false);
	m_prevPresetButton->setCursor(Qt::PointingHandCursor);
	m_prevPresetButton->setActiveGraphic(embed::getIconPixmap("stepper-left-press"));
	m_prevPresetButton->setInactiveGraphic(embed::getIconPixmap("stepper-left"));
	m_prevPresetButton->setToolTip(QObject::tr("Previous (-)"));
	m_prevPresetButton->setShortcut(Qt::Key_Minus);
	m_prevPresetButton->setMinimumWidth(16);
	m_prevPresetButton->setMaximumWidth(16);
	m_prevPresetButton->setMinimumHeight(16);
	m_prevPresetButton->setMaximumHeight(16);

	m_nextPresetButton = new PixmapButton{this};
	m_nextPresetButton->setCheckable(false);
	m_nextPresetButton->setCursor(Qt::PointingHandCursor);
	m_nextPresetButton->setActiveGraphic(embed::getIconPixmap("stepper-right-press"));
	m_nextPresetButton->setInactiveGraphic(embed::getIconPixmap("stepper-right"));
	m_nextPresetButton->setToolTip(QObject::tr("Next (+)"));
	m_nextPresetButton->setShortcut(Qt::Key_Plus);
	m_nextPresetButton->setMinimumWidth(16);
	m_nextPresetButton->setMaximumWidth(16);
	m_nextPresetButton->setMinimumHeight(16);
	m_nextPresetButton->setMaximumHeight(16);

	m_selectPresetButton = new QPushButton(this);
	m_selectPresetButton->setCheckable(false);
	m_selectPresetButton->setCursor(Qt::PointingHandCursor);
	m_selectPresetButton->setIcon(embed::getIconPixmap("stepper-down"));

	auto menu = new QMenu{};
	m_selectPresetButton->setMenu(menu);
	m_selectPresetButton->setMinimumWidth(16);
	m_selectPresetButton->setMaximumWidth(16);
	m_selectPresetButton->setMinimumHeight(16);
	m_selectPresetButton->setMaximumHeight(16);

	connect(menu, &QMenu::aboutToShow, this, &PresetSelector::updateMenu);
	connect(m_presets, &PluginPresets::presetCollectionChanged, this, &PresetSelector::updateMenu);

	auto tb = new QToolBar{this};
	tb->resize(100, 32); // TODO: Adjust size
	tb->addWidget(m_prevPresetButton);
	tb->addWidget(m_nextPresetButton);
	tb->addWidget(m_selectPresetButton);
	//tb->addWidget(m_openPresetButton);
	//tb->addWidget(m_savePresetButton);
	//tb->addWidget(m_managePluginButton);
}

void PresetSelector::updateMenu()
{
	if (!m_selectPresetButton) { return; }

	const auto menu = m_selectPresetButton->menu();
	menu->clear();

	if (!m_presets) { return; }

	const auto& presets = m_presets->presets();
	for (int idx = 0; idx < static_cast<int>(presets.size()); ++idx)
	{
		auto presetAction = new QAction{this};
		connect(presetAction, &QAction::triggered, this, &PresetSelector::selectPreset);

		const auto name = QString::fromStdString(presets[idx]->metadata().displayName);
		presetAction->setText(QString("%1. %2").arg(QString::number(idx + 1), name));
		presetAction->setData(idx);

		presetAction->setIcon(embed::getIconPixmap(
			idx == m_lastPosInMenu ? "sample_file" : "edit_copy", 16, 16));

		menu->addAction(presetAction);
	}
}

void PresetSelector::selectPreset()
{
	if (!m_presets) { return; }

	const auto action = qobject_cast<QAction*>(sender());
	if (!action) { return; }

	m_lastPosInMenu = action->data().toInt();
	if (m_lastPosInMenu < 0) { return; }
	m_presets->activatePreset(static_cast<std::size_t>(m_lastPosInMenu));

	// QWidget::update();
}

} // namespace lmms::gui
