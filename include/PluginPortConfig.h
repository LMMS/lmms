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

class QWidget;

namespace lmms
{

namespace gui
{

class ComboBox;

} // namespace gui

//! Configure channel routing for a plugin's mono/stereo in/out ports
class LMMS_EXPORT PluginPortConfig
	: public QObject
	, public SerializingObject
{
	Q_OBJECT

public:
	enum class Config
	{
		None       = -1,
		MonoMix    =  0, // mono ports only
		LeftOnly   =  1, // mono ports only
		RightOnly  =  2, // mono ports only
		Stereo     =  3
	};

	enum class MonoPluginType
	{
		None,
		Input,
		Output,
		Both
	};

	PluginPortConfig(Model* parent = nullptr);
	PluginPortConfig(int inCount, int outCount, Model* parent = nullptr);

	/**
	 * Getters
	 */
	auto portCountIn() const -> int { return m_portCountIn; }
	auto portCountOut() const -> int { return m_portCountOut; }

	auto portConfigIn() const -> Config
	{
		switch (m_portCountIn)
		{
			default: [[fallthrough]];
			case 0: return Config::None;
			case 1: return static_cast<Config>(m_config.value());
			case 2: return Config::Stereo;
		}
	}

	auto portConfigOut() const -> Config
	{
		switch (m_portCountOut)
		{
			default: [[fallthrough]];
			case 0: return Config::None;
			case 1: return static_cast<Config>(m_config.value());
			case 2: return Config::Stereo;
		}
	}

	auto hasMonoPort() const -> bool;
	auto monoPluginType() const -> MonoPluginType;
	auto model() -> ComboBoxModel* { return &m_config; }

	/**
	 * Setters
	 */
	void setPortCounts(int inCount, int outCount);
	void setPortCountIn(int inCount);
	void setPortCountOut(int outCount);
	auto setPortConfig(Config config) -> bool;

	/**
	 * SerializingObject implementation
	 */
	void saveSettings(QDomDocument& doc, QDomElement& elem) override;
	void loadSettings(const QDomElement& elem) override;
	auto nodeName() const -> QString override { return "port_config"; }

	auto instantiateView(QWidget* parent = nullptr) -> gui::ComboBox*;

signals:
	void portsChanged();

private:
	void updateOptions();

	int m_portCountIn = DEFAULT_CHANNELS;
	int m_portCountOut = DEFAULT_CHANNELS;

	//! Value is 0..2, which represents { MonoMix, LeftOnly, RightOnly } for non-Stereo plugins
	ComboBoxModel m_config;
};

} // namespace lmms

#endif // LMMS_PLUGIN_PORT_CONFIG_H
