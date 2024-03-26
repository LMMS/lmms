/*
 * MonoPluginConfiguration.cpp - Settings for plugins with a mono input
 *                               and/or output
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

#include <QDomDocument>
#include <QDomElement>

#include "MonoPluginConfiguration.h"

namespace lmms
{

MonoPluginConfiguration::MonoPluginConfiguration(PluginType pluginType, QObject* parent)
	: QObject{parent}
	, m_pluginType{pluginType}
	, m_monoConfig{0, 0, 2, nullptr, "Mono configuration"}
{
}

auto MonoPluginConfiguration::hasStereoInput() const -> bool
{
	return m_pluginType == PluginType::StereoInOut
		|| m_pluginType == PluginType::StereoInMonoOut;
}

auto MonoPluginConfiguration::hasStereoOutput() const -> bool
{
	return m_pluginType == PluginType::StereoInOut
		|| m_pluginType == PluginType::MonoInStereoOut;
}

void MonoPluginConfiguration::setPluginType(PluginType pluginType)
{
	m_pluginType = pluginType;
}

auto MonoPluginConfiguration::monoConfig(bool isInput) const -> Config
{
	switch (m_pluginType)
	{
		default: [[fallthrough]];
		case PluginType::StereoInOut:
			return Config::Stereo;
		case PluginType::MonoInStereoOut:
			if (!isInput) { return Config::Stereo; }
			return static_cast<Config>(m_monoConfig.value() + 1);
		case PluginType::StereoInMonoOut:
			if (isInput) { return Config::Stereo; }
			return static_cast<Config>(m_monoConfig.value() + 1);
		case PluginType::MonoInOut:
			return static_cast<Config>(m_monoConfig.value() + 1);
	}

	return Config::Stereo; // unreachable
}

auto MonoPluginConfiguration::monoConfigOfModifiableSide() const -> Config
{
	return m_pluginType == PluginType::StereoInOut
		? Config::Stereo
		: static_cast<Config>(m_monoConfig.value() + 1);
}

auto MonoPluginConfiguration::setMonoConfig(Config config) -> bool
{
	if (m_pluginType == PluginType::StereoInOut)
	{
		if (config != Config::Stereo) { return false; }
		m_monoConfig.setValue(0);
	}
	else
	{
		if (config == Config::Stereo) { return false; }
		m_monoConfig.setValue(static_cast<int>(config) - 1);
	}

	return true;
}

auto MonoPluginConfiguration::setMonoConfigIndex(int config) -> bool
{
	if (config < 0 || config > 2) { return false; }
	if (m_pluginType == PluginType::StereoInOut && config != 0) { return false; }
	m_monoConfig.setValue(config);
	return true;
}

void MonoPluginConfiguration::saveSettings(QDomDocument& doc, QDomElement& elem)
{
	if (m_pluginType == PluginType::StereoInOut) { return; }

	elem.setAttribute("type", static_cast<int>(m_pluginType));
	m_monoConfig.saveSettings(doc, elem, "config");
}

void MonoPluginConfiguration::loadSettings(const QDomElement& elem)
{
	m_pluginType = static_cast<PluginType>(elem.attribute("type", "0").toInt());
	if (m_pluginType == PluginType::StereoInOut) { return; }

	m_monoConfig.loadSettings(elem, "config");
}

} // namespace lmms
