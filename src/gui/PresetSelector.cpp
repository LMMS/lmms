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

// TODO: Use this for LV2 and VST presets

#include "PresetSelector.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>

#include "embed.h"
#include "FileDialog.h"
#include "PathUtil.h"
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
	m_prevPresetButton->setToolTip(tr("Previous preset"));
	m_prevPresetButton->setFixedSize(16, 16);

	connect(m_prevPresetButton, &PixmapButton::clicked, this, [&] { m_presets->prevPreset(); });

	m_nextPresetButton = new PixmapButton{this};
	m_nextPresetButton->setCheckable(false);
	m_nextPresetButton->setCursor(Qt::PointingHandCursor);
	m_nextPresetButton->setActiveGraphic(embed::getIconPixmap("stepper-right-press"));
	m_nextPresetButton->setInactiveGraphic(embed::getIconPixmap("stepper-right"));
	m_nextPresetButton->setToolTip(tr("Next preset"));
	m_nextPresetButton->setFixedSize(16, 16);

	connect(m_nextPresetButton, &PixmapButton::clicked, this, [&] { m_presets->nextPreset(); });

	m_selectPresetButton = new QPushButton{this};
	m_selectPresetButton->setCheckable(false);
	m_selectPresetButton->setCursor(Qt::PointingHandCursor);
	m_selectPresetButton->setIcon(embed::getIconPixmap("stepper-down"));
	m_selectPresetButton->setToolTip(tr("Select preset"));
	auto menu = new QMenu{};
	m_selectPresetButton->setMenu(menu);
	m_selectPresetButton->setFixedSize(16, 16);

	connect(menu, &QMenu::aboutToShow, this, &PresetSelector::updateMenu);
	connect(m_presets, &PluginPresets::presetCollectionChanged, this, &PresetSelector::updateMenu);

	m_loadPresetButton = new PixmapButton{this};
	m_loadPresetButton->setCheckable(false);
	m_loadPresetButton->setCursor(Qt::PointingHandCursor);
	m_loadPresetButton->setActiveGraphic(embed::getIconPixmap("project_open", 21, 21));
	m_loadPresetButton->setInactiveGraphic(embed::getIconPixmap("project_open", 21, 21));
	m_loadPresetButton->setToolTip(tr("Load preset"));
	m_loadPresetButton->setFixedSize(21, 21);

	connect(m_loadPresetButton, &PixmapButton::clicked, this, &PresetSelector::loadPreset);

	m_savePresetButton = new PixmapButton{this};
	m_savePresetButton->setCheckable(false);
	m_savePresetButton->setCursor(Qt::PointingHandCursor);
	m_savePresetButton->setActiveGraphic(embed::getIconPixmap("project_save", 21, 21));
	m_savePresetButton->setInactiveGraphic(embed::getIconPixmap("project_save", 21, 21));
	m_savePresetButton->setToolTip(tr("Save preset"));
	m_savePresetButton->setFixedSize(21, 21);

	connect(m_savePresetButton, &PixmapButton::clicked, this, &PresetSelector::savePreset);

	auto widget = new QWidget{this};
	auto layout = new QHBoxLayout{widget};
	layout->addWidget(m_activePreset, Qt::AlignmentFlag::AlignLeft);
	layout->addStretch(1);
	layout->addWidget(m_prevPresetButton, Qt::AlignmentFlag::AlignRight);
	layout->addWidget(m_nextPresetButton);
	layout->addWidget(m_selectPresetButton);
	layout->addWidget(m_loadPresetButton);
	layout->addWidget(m_savePresetButton);
	widget->setLayout(layout);

	addWidget(widget);
}

auto PresetSelector::sizeHint() const -> QSize
{
	// See InstrumentViewFixedSize
	return QSize{250, 24};
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
		connect(presetAction, &QAction::triggered, this, [this, idx] { selectPreset(idx); });

		const auto isActive = static_cast<int>(m_presets->presetIndex().value_or(-1)) == idx;
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

void PresetSelector::loadPreset()
{
	if (!m_presets) { return; }

	const auto database = m_presets->presetDatabase();
	if (!database) { return; }

	auto openFileDialog = gui::FileDialog(nullptr, QObject::tr("Open audio file"));

	// Change dir to position of previously opened file
	const auto recentFile = QString::fromUtf8(
		database->recentPresetFile().data(), database->recentPresetFile().size());
	openFileDialog.setDirectory(recentFile);
	openFileDialog.setFileMode(gui::FileDialog::ExistingFiles);

	// Set filters
	auto fileTypes = QStringList{};
	auto allFileTypes = QStringList{};
	auto nameFilters = QStringList{};

	for (const auto& filetype : database->filetypes())
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

	if (!recentFile.isEmpty())
	{
		// Select previously opened file
		openFileDialog.selectFile(QFileInfo{recentFile}.fileName());
	}

	if (openFileDialog.exec() != QDialog::Accepted) { return; }

	if (openFileDialog.selectedFiles().isEmpty()) { return; }

	auto presets = database->loadPresets(openFileDialog.selectedFiles()[0].toStdString());
	if (presets.empty())
	{
		QMessageBox::warning(this, tr("Preset load failure"),
			tr("Failed to load preset(s) or preset(s) were already loaded"));
	}
}

void PresetSelector::savePreset()
{
	// [NOT IMPLEMENTED YET]
	QMessageBox::warning(this, tr("Save preset"), tr("This feature is not implemented yet"));
}

void PresetSelector::selectPreset(int pos)
{
	if (!m_presets || pos < 0) { return; }

	m_presets->activatePreset(static_cast<std::size_t>(pos));

	// QWidget::update();
}

} // namespace lmms::gui
