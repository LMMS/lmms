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

PluginPortConfig::PluginPortConfig(PortType in, PortType out, Model* parent)
	: QObject{parent}
	, m_inPort{in}
	, m_outPort{out}
	, m_config{parent, tr("L/R channel configuration")}
{
}

auto PluginPortConfig::hasMonoPort() const -> bool
{
	return m_inPort == PortType::Mono || m_outPort == PortType::Mono;
}

auto PluginPortConfig::monoPluginType() const -> MonoPluginType
{
	if (m_inPort == PortType::Mono)
	{
		if (m_outPort == PortType::Mono)
		{
			return MonoPluginType::Both;
		}
		return MonoPluginType::Input;
	}
	else if (m_outPort == PortType::Mono)
	{
		return MonoPluginType::Output;
	}
	return MonoPluginType::None;
}

void PluginPortConfig::setPortType(unsigned inCount, unsigned outCount)
{
	PortType in;
	PortType out;
	switch (inCount)
	{
		case 0: in = PortType::None; break;
		case 1: in = PortType::Mono; break;
		case 2: in = PortType::Stereo; break;
		default: throw std::invalid_argument{"Invalid input count"};
	}
	switch (outCount)
	{
		case 0: out = PortType::None; break;
		case 1: out = PortType::Mono; break;
		case 2: out = PortType::Stereo; break;
		default: throw std::invalid_argument{"Invalid output count"};
	}
	setPortType(in, out);
}

void PluginPortConfig::setPortType(PortType in, PortType out)
{
	if (in == PortType::None && out == PortType::None) { return; }
	if (m_inPort == in && m_outPort == out) { return; }

	m_inPort = in;
	m_outPort = out;

	updateOptions();

	emit portsChanged();
}

auto PluginPortConfig::setPortConfig(Config config) -> bool
{
	assert(config != Config::None);
	if (m_inPort != PortType::Mono && m_outPort != PortType::Mono)
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
	//if (m_inPort != PortType::Mono && m_outPort != PortType::Mono) { return; }

	elem.setAttribute("in", static_cast<int>(m_inPort));   // probably not needed, but just in case
	elem.setAttribute("out", static_cast<int>(m_outPort)); // ditto
	m_config.saveSettings(doc, elem, "config");
}

void PluginPortConfig::loadSettings(const QDomElement& elem)
{
	//const auto inPort = static_cast<PortType>(elem.attribute("in", "0").toInt());
	//const auto outPort = static_cast<PortType>(elem.attribute("out", "0").toInt());
	m_config.loadSettings(elem, "config");
}

auto PluginPortConfig::instantiateView(QWidget* parent) -> gui::ComboBox*
{
	auto view = new gui::ComboBox{parent};

	QString inputType;
	switch (inputPortType())
	{
		case PluginPortConfig::PortType::None: break;
		case PluginPortConfig::PortType::Mono:
			inputType += QObject::tr("mono in"); break;
		case PluginPortConfig::PortType::Stereo:
			inputType += QObject::tr("stereo in"); break;
		default: break;
	}

	QString outputType;
	switch (outputPortType())
	{
		case PluginPortConfig::PortType::None: break;
		case PluginPortConfig::PortType::Mono:
			outputType += QObject::tr("mono out"); break;
		case PluginPortConfig::PortType::Stereo:
			outputType += QObject::tr("stereo out"); break;
		default: break;
	}

	QString pluginType;
	if (inputType.isEmpty()) { pluginType = outputType; }
	else if (outputType.isEmpty()) { pluginType = inputType; }
	else { pluginType = QString{"%1, %2"}.arg(inputType, outputType); }

	view->setToolTip(QObject::tr("L/R channel config for %1 plugin").arg(pluginType));
	view->setModel(model());

	return view;
}

void PluginPortConfig::updateOptions()
{
	m_config.clear();

	const auto monoType = monoPluginType();
	if (monoType == PluginPortConfig::MonoPluginType::None)
	{
		m_config.addItem(tr("Stereo"));
		return;
	}

	const auto hasInputPort = inputPortType() != PluginPortConfig::PortType::None;
	const auto hasOutputPort = outputPortType() != PluginPortConfig::PortType::None;

	// 1. Mono mix
	QString itemText;
	switch (monoType)
	{
		case PluginPortConfig::MonoPluginType::Input:
			itemText = tr("Downmix to mono"); break;
		case PluginPortConfig::MonoPluginType::Output:
			itemText = tr("Upmix to stereo"); break;
		case PluginPortConfig::MonoPluginType::Both:
			itemText = tr("Mono mix"); break;
		default: break;
	}
	m_config.addItem(itemText);

	// 2. Left only
	itemText = QString{};
	switch (monoType)
	{
		case PluginPortConfig::MonoPluginType::Input:
			itemText = hasOutputPort
				? tr("L in (R bypass)")
				: tr("Left in");
			break;
		case PluginPortConfig::MonoPluginType::Output:
			itemText = hasInputPort
				? tr("L out (R bypass)")
				: tr("Left only");
			break;
		case PluginPortConfig::MonoPluginType::Both:
			itemText = tr("L only (R bypass)");
			break;
		default: break;
	}
	m_config.addItem(itemText);

	// 3. Right only
	itemText = QString{};
	switch (monoType)
	{
		case PluginPortConfig::MonoPluginType::Input:
			itemText = hasOutputPort
				? tr("R in (L bypass)")
				: tr("Right in");
			break;
		case PluginPortConfig::MonoPluginType::Output:
			itemText = hasInputPort
				? tr("R out (L bypass)")
				: tr("Right only");
			break;
		case PluginPortConfig::MonoPluginType::Both:
			itemText = tr("R only (L bypass)");
			break;
		default: break;
	}
	m_config.addItem(itemText);
}

} // namespace lmms
