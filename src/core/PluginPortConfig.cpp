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

namespace lmms
{

PluginPortConfig::PluginPortConfig(PortType in, PortType out, QObject* parent)
	: QObject{parent}
	, m_inPort{in}
	, m_outPort{out}
	, m_config{0, 0, 2, nullptr, tr("Audio channel configuration")}
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

void PluginPortConfig::setPortType(PortType in, PortType out)
{
	if (in == PortType::None && out == PortType::None) { return; }
	if (m_inPort == in && m_outPort == out) { return; }

	m_inPort = in;
	m_outPort = out;

	if (m_inPort != PortType::Mono && m_outPort != PortType::Mono)
	{
		m_config.setRange(0, 0);
		m_config.setValue(0);
	}
	else
	{
		m_config.setRange(0, 2);
		m_config.setValue(static_cast<int>(Config::MonoMix));
	}

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

} // namespace lmms
