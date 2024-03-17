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

#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QPushButton>

#include "embed.h"
#include "PixmapButton.h"
#include "PluginPresets.h"

namespace lmms::gui
{

PresetSelector::PresetSelector(PluginPresets* presets, QWidget* parent)
	: QToolBar{parent}
	, IntModelView{presets, parent}
	, m_presets{presets}
{
	setSizePolicy({QSizePolicy::MinimumExpanding, QSizePolicy::Fixed});

	m_activePreset = new QLabel{this};
	updateActivePreset();

	connect(m_presets, &PluginPresets::activePresetChanged, this, &PresetSelector::updateActivePreset);
	connect(m_presets, &PluginPresets::activePresetModified, this, &PresetSelector::updateActivePreset);

	m_prevPresetButton = new PixmapButton{this};
	m_prevPresetButton->setCheckable(false);
	m_prevPresetButton->setCursor(Qt::PointingHandCursor);
	m_prevPresetButton->setActiveGraphic(embed::getIconPixmap("stepper-left-press"));
	m_prevPresetButton->setInactiveGraphic(embed::getIconPixmap("stepper-left"));
	m_prevPresetButton->setToolTip(tr("Previous (-)"));
	m_prevPresetButton->setShortcut(Qt::Key_Minus);
	m_prevPresetButton->setMinimumWidth(16);
	m_prevPresetButton->setMaximumWidth(16);
	m_prevPresetButton->setMinimumHeight(16);
	m_prevPresetButton->setMaximumHeight(16);

	connect(m_prevPresetButton, &PixmapButton::clicked, this, [&] { m_presets->prevPreset(); });

	m_nextPresetButton = new PixmapButton{this};
	m_nextPresetButton->setCheckable(false);
	m_nextPresetButton->setCursor(Qt::PointingHandCursor);
	m_nextPresetButton->setActiveGraphic(embed::getIconPixmap("stepper-right-press"));
	m_nextPresetButton->setInactiveGraphic(embed::getIconPixmap("stepper-right"));
	m_nextPresetButton->setToolTip(tr("Next (+)"));
	m_nextPresetButton->setShortcut(Qt::Key_Plus);
	m_nextPresetButton->setMinimumWidth(16);
	m_nextPresetButton->setMaximumWidth(16);
	m_nextPresetButton->setMinimumHeight(16);
	m_nextPresetButton->setMaximumHeight(16);

	connect(m_nextPresetButton, &PixmapButton::clicked, this, [&] { m_presets->nextPreset(); });

	m_selectPresetButton = new QPushButton{this};
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

	auto widget = new QWidget{this};
	auto layout = new QHBoxLayout{widget};
	layout->addWidget(m_activePreset, Qt::AlignmentFlag::AlignLeft);
	layout->addStretch(1);
	layout->addWidget(m_prevPresetButton, Qt::AlignmentFlag::AlignRight);
	layout->addWidget(m_nextPresetButton);
	layout->addWidget(m_selectPresetButton);
	widget->setLayout(layout);

	addWidget(widget);
}

auto PresetSelector::sizeHint() const -> QSize
{
	// See InstrumentViewFixedSize
	return QSize{250, 32};
}

void PresetSelector::updateActivePreset()
{
	if (!m_presets) { return; }

	const auto preset = m_presets->activePreset();
	if (!preset)
	{
		m_activePreset->setText(tr("(No active preset)"));
		return;
	}

	const auto presetIndex = m_presets->presetIndex().value() + 1;
	const auto indexText = QString{"%1/%2"}.arg(presetIndex).arg(m_presets->presets().size());

	const auto displayName = QString::fromStdString(preset->metadata().displayName);
	const auto text = m_presets->isModified()
		? QString{"%1: %2<b><i>*</i></b>"}.arg(indexText).arg(displayName)
		: QString{"%1: %2"}.arg(indexText).arg(displayName);

	m_activePreset->setText(text);
}

void PresetSelector::updateMenu()
{
	if (!m_selectPresetButton) { return; }

	const auto menu = m_selectPresetButton->menu();
	menu->setStyleSheet("QMenu { menu-scrollable: 1; }");
	menu->clear();

	if (!m_presets) { return; }

	const auto& presets = m_presets->presets();
	for (int idx = 0; idx < static_cast<int>(presets.size()); ++idx)
	{
		auto presetAction = new QAction{this};
		connect(presetAction, &QAction::triggered, this, [=] { selectPreset(idx); });

		const auto isActive = m_presets->presetIndex().value_or(-1) == idx;
		if (isActive)
		{
			auto font = presetAction->font();
			font.setBold(true);
			font.setItalic(true);
			presetAction->setFont(font);
		}

		const auto name = QString::fromStdString(presets[idx]->metadata().displayName);
		presetAction->setText(QString{"%1. %2"}.arg(QString::number(idx + 1), name));

		const auto icon = isActive ? "sample_file" : "edit_copy";
		presetAction->setIcon(embed::getIconPixmap(icon, 16, 16));

		menu->addAction(presetAction);
	}

	// TODO: Scroll to active preset?
}

void PresetSelector::selectPreset(int pos)
{
	if (!m_presets || pos < 0) { return; }

	m_presets->activatePreset(static_cast<std::size_t>(pos));

	// QWidget::update();
}

} // namespace lmms::gui
