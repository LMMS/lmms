/*
 * PluginPortConfig.cpp - Specifies how to route audio channels
 *                        in and out of a plugin.
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

#include "PluginPortConfig.h"

#include <QDomDocument>
#include <QDomElement>
#include <cassert>
#include <stdexcept>

#include "ComboBox.h"
#include "Model.h"

namespace lmms
{

PluginPortConfig::PluginPortConfig(Model* parent)
	: QObject{parent}
	, m_config{parent, tr("L/R channel configuration")}
{
}

PluginPortConfig::PluginPortConfig(int inCount, int outCount, Model* parent)
	: QObject{parent}
	, m_portCountIn{inCount}
	, m_portCountOut{outCount}
	, m_config{parent, tr("L/R channel configuration")}
{
}

auto PluginPortConfig::hasMonoPort() const -> bool
{
	return m_portCountIn == 1 || m_portCountOut == 1;
}

auto PluginPortConfig::monoPluginType() const -> MonoPluginType
{
	if (m_portCountIn == 1)
	{
		if (m_portCountOut == 1)
		{
			return MonoPluginType::Both;
		}
		return MonoPluginType::Input;
	}
	else if (m_portCountOut == 1)
	{
		return MonoPluginType::Output;
	}
	return MonoPluginType::None;
}

void PluginPortConfig::setPortCounts(int inCount, int outCount)
{
	if (inCount < 0 || inCount > DEFAULT_CHANNELS)
	{
		throw std::invalid_argument{"Invalid input count"};
	}

	if (outCount < 0 || outCount > DEFAULT_CHANNELS)
	{
		throw std::invalid_argument{"Invalid output count"};
	}

	if (inCount == 0 && outCount == 0)
	{
		throw std::invalid_argument{"At least one port count must be non-zero"};
	}

	if (m_portCountIn == inCount && m_portCountOut == outCount)
	{
		// No action needed
		return;
	}

	m_portCountIn = inCount;
	m_portCountOut = outCount;

	updateOptions();

	emit portsChanged();
}

void PluginPortConfig::setPortCountIn(int inCount)
{
	setPortCounts(inCount, m_portCountOut);
}

void PluginPortConfig::setPortCountOut(int outCount)
{
	setPortCounts(m_portCountIn, outCount);
}

auto PluginPortConfig::setPortConfig(Config config) -> bool
{
	assert(config != Config::None);
	if (m_portCountIn != 1 && m_portCountOut != 1)
	{
		if (config != Config::Stereo) { return false; }
		m_config.setValue(0);
	}
	else
	{
		if (config == Config::Stereo) { return false; }
		m_config.setValue(static_cast<int>(config));
	}

	return true;
}

void PluginPortConfig::saveSettings(QDomDocument& doc, QDomElement& elem)
{
	// Only plugins with a mono in/out need to be saved
	//if (m_portCountIn != 1 && m_portCountOut != 1) { return; }

	elem.setAttribute("in", m_portCountIn);   // probably not needed, but just in case
	elem.setAttribute("out", m_portCountOut); // ditto
	m_config.saveSettings(doc, elem, "config");
}

void PluginPortConfig::loadSettings(const QDomElement& elem)
{
	// TODO: Assert port counts are what was expected?
	//const auto portCountIn = elem.attribute("in", "0").toInt();
	//const auto portCountOut = elem.attribute("out", "0").toInt();
	m_config.loadSettings(elem, "config");
}

auto PluginPortConfig::instantiateView(QWidget* parent) -> gui::ComboBox*
{
	auto view = new gui::ComboBox{parent};

	QString inputType;
	switch (m_portCountIn)
	{
		case 0: break;
		case 1: inputType += tr("mono in"); break;
		case 2: inputType += tr("stereo in"); break;
		default: break;
	}

	QString outputType;
	switch (m_portCountOut)
	{
		case 0: break;
		case 1: outputType += tr("mono out"); break;
		case 2: outputType += tr("stereo out"); break;
		default: break;
	}

	QString pluginType;
	if (inputType.isEmpty()) { pluginType = outputType; }
	else if (outputType.isEmpty()) { pluginType = inputType; }
	else { pluginType = tr("%1, %2").arg(inputType, outputType); }

	view->setToolTip(tr("L/R channel config for %1 plugin").arg(pluginType));
	view->setModel(model());

	return view;
}

void PluginPortConfig::updateOptions()
{
	m_config.clear();

	const auto monoType = monoPluginType();
	if (monoType == MonoPluginType::None)
	{
		m_config.addItem(tr("Stereo"));
		return;
	}

	const auto hasInputPort = m_portCountIn != 0;
	const auto hasOutputPort = m_portCountOut != 0;

	// 1. Mono mix
	QString itemText;
	switch (monoType)
	{
		case MonoPluginType::Input:
			itemText = tr("Downmix to mono"); break;
		case MonoPluginType::Output:
			itemText = tr("Upmix to stereo"); break;
		case MonoPluginType::Both:
			itemText = tr("Mono mix"); break;
		default: break;
	}
	m_config.addItem(itemText);

	// 2. Left only
	itemText = QString{};
	switch (monoType)
	{
		case MonoPluginType::Input:
			itemText = hasOutputPort
				? tr("L in (R bypass)")
				: tr("Left in");
			break;
		case MonoPluginType::Output:
			itemText = hasInputPort
				? tr("L out (R bypass)")
				: tr("Left only");
			break;
		case MonoPluginType::Both:
			itemText = tr("L only (R bypass)");
			break;
		default: break;
	}
	m_config.addItem(itemText);

	// 3. Right only
	itemText = QString{};
	switch (monoType)
	{
		case MonoPluginType::Input:
			itemText = hasOutputPort
				? tr("R in (L bypass)")
				: tr("Right in");
			break;
		case MonoPluginType::Output:
			itemText = hasInputPort
				? tr("R out (L bypass)")
				: tr("Right only");
			break;
		case MonoPluginType::Both:
			itemText = tr("R only (L bypass)");
			break;
		default: break;
	}
	m_config.addItem(itemText);
}

} // namespace lmms
