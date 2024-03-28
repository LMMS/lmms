/*
 * PluginPortConfigSelector.cpp - PluginPortConfig selector
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

#include "PluginPortConfigSelector.h"

#include <QComboBox>

#include "PluginPortConfig.h"

namespace lmms::gui
{

PluginPortConfigSelector::PluginPortConfigSelector(PluginPortConfig* config, QWidget* parent)
	: QWidget{parent}
	, m_config{config}
{
	m_comboBox = new QComboBox{this};

	if (!m_config) { return; }

	connect(m_config, &PluginPortConfig::portsChanged, this, &PluginPortConfigSelector::updateOptions);
	connect(m_config->model(), &IntModel::dataChanged, this, &PluginPortConfigSelector::updateSelection);
	connect(m_comboBox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
		this, &PluginPortConfigSelector::onIndexChanged);

	updateOptions();
}

auto PluginPortConfigSelector::sizeHint() const -> QSize
{
	return m_comboBox->sizeHint();
}

void PluginPortConfigSelector::onIndexChanged(int index)
{
	if (!m_config) { return; }
	const auto config = static_cast<PluginPortConfig::Config>(m_comboBox->itemData(index).toInt());
	m_config->setPortConfig(config);
}

void PluginPortConfigSelector::updateOptions()
{
	m_comboBox->clear();
	if (!m_config) { return; }

	const auto monoType = m_config->monoPluginType();
	if (monoType == PluginPortConfig::MonoPluginType::None)
	{
		m_comboBox->addItem(tr("Stereo in/out"));
		m_comboBox->setDisabled(true);
		return;
	}

	const auto hasInputPort = m_config->inputPortType() != PluginPortConfig::PortType::None;
	const auto hasOutputPort = m_config->outputPortType() != PluginPortConfig::PortType::None;

	// 1. Mono mix
	QString itemText;
	switch (monoType)
	{
		case PluginPortConfig::MonoPluginType::Input:
			itemText = tr("Downmix to mono input"); break;
		case PluginPortConfig::MonoPluginType::Output:
			itemText = tr("Upmix to stereo output"); break;
		case PluginPortConfig::MonoPluginType::Both:
			itemText = tr("Mono mix"); break;
		default: break;
	}
	m_comboBox->addItem(itemText, static_cast<int>(PluginPortConfig::Config::MonoMix));

	// 2. Left only
	itemText = QString{};
	switch (monoType)
	{
		case PluginPortConfig::MonoPluginType::Input:
			itemText = hasOutputPort
				? tr("Left in / Right bypass")
				: tr("Left channel in");
			break;
		case PluginPortConfig::MonoPluginType::Output:
			itemText = hasInputPort
				? tr("Left in / Right bypass")
				: tr("Left channel out");
			break;
		case PluginPortConfig::MonoPluginType::Both:
			itemText = tr("Left in/out, Right bypass");
			break;
		default: break;
	}
	m_comboBox->addItem(itemText, static_cast<int>(PluginPortConfig::Config::LeftOnly));

	// 3. Right only
	itemText = QString{};
	switch (monoType)
	{
		case PluginPortConfig::MonoPluginType::Input:
			itemText = hasOutputPort
				? tr("Right in / Left bypass")
				: tr("Right channel in");
			break;
		case PluginPortConfig::MonoPluginType::Output:
			itemText = hasInputPort
				? tr("Right in / Left bypass")
				: tr("Right channel out");
			break;
		case PluginPortConfig::MonoPluginType::Both:
			itemText = tr("Right in/out, Left bypass");
			break;
		default: break;
	}
	m_comboBox->addItem(itemText, static_cast<int>(PluginPortConfig::Config::RightOnly));

	m_comboBox->setDisabled(false);
}

void PluginPortConfigSelector::updateSelection()
{
	if (!m_config) { return; }
	m_comboBox->setCurrentIndex(m_config->model()->value());
}

} // namespace lmms::gui
