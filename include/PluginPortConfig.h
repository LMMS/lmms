/*
 * PluginPortConfig.h - Specifies how to route audio channels
 *                      in and out of a plugin.
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

#ifndef LMMS_PLUGIN_PORT_CONFIG_H
#define LMMS_PLUGIN_PORT_CONFIG_H

#include <QObject>

#include "ComboBoxModel.h"
#include "lmms_export.h"
#include "SerializingObject.h"

namespace lmms
{

//! Configure channel routing for a plugin's mono/stereo in/out ports
class LMMS_EXPORT PluginPortConfig
	: public QObject
	, public SerializingObject
{
	Q_OBJECT

public:
	enum class PortType
	{
		None,
		Mono,
		Stereo
	};

	enum class Config
	{
		None = -1,
		MonoMix,    // mono ports only
		LeftOnly,   // mono ports only
		RightOnly,  // mono ports only
		Stereo
	};

	enum class MonoPluginType
	{
		None,
		Input,
		Output,
		Both
	};

	PluginPortConfig(Model* parent = nullptr);
	PluginPortConfig(PortType in, PortType out, Model* parent = nullptr);

	/**
	 * Getters
	 */
	auto inputPortType() const { return m_inPort; }
	auto outputPortType() const { return m_outPort; }

	template<bool isInput>
	auto portConfig() const -> Config
	{
		if constexpr (isInput)
		{
			switch (m_inPort)
			{
				default: [[fallthrough]];
				case PortType::None:
					return Config::None;
				case PortType::Mono:
					return static_cast<Config>(m_config.value());
				case PortType::Stereo:
					return Config::Stereo;
			}
		}
		else
		{
			switch (m_outPort)
			{
				default: [[fallthrough]];
				case PortType::None:
					return Config::None;
				case PortType::Mono:
					return static_cast<Config>(m_config.value());
				case PortType::Stereo:
					return Config::Stereo;
			}
		}
	}

	auto hasMonoPort() const -> bool;
	auto monoPluginType() const -> MonoPluginType;
	auto model() -> ComboBoxModel* { return &m_config; }

	/**
	 * Setters
	 */
	void setPortType(PortType in, PortType out);
	auto setPortConfig(Config config) -> bool;

	/**
	 * SerializingObject implementation
	 */
	void saveSettings(QDomDocument& doc, QDomElement& elem) override;
	void loadSettings(const QDomElement& elem) override;
	auto nodeName() const -> QString override { return "port_config"; }

signals:
	void portsChanged();

private:
	void updateOptions();

	PortType m_inPort = PortType::None;
	PortType m_outPort = PortType::None;

	//! Value is 0..2, which represents { MonoMix, LeftOnly, RightOnly } for non-Stereo plugins
	ComboBoxModel m_config;
};

} // namespace lmms

#endif // LMMS_PLUGIN_PORT_CONFIG_H
