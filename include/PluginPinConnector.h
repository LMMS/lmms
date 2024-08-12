/*
 * PluginPinConnector.h - Specifies how to route audio channels
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

#ifndef LMMS_PLUGIN_PIN_CONNECTOR_H
#define LMMS_PLUGIN_PIN_CONNECTOR_H

#include <QObject>
#include <vector>

#include "AutomatableModel.h"
#include "lmms_basics.h"
#include "lmms_export.h"
#include "SerializingObject.h"

class QWidget;

namespace lmms
{

namespace gui
{

class PluginPinConnectorWidget;

} // namespace gui

//! Configure channel routing for a plugin's mono/stereo in/out ports
class LMMS_EXPORT PluginPinConnector
	: public QObject
	, public SerializingObject
{
	Q_OBJECT

public:
	//! [LMMS track channel][plugin channel]
	using PinMap = std::vector<std::vector<BoolModel*>>;

	PluginPinConnector(Model* parent = nullptr);
	PluginPinConnector(int pluginInCount, int pluginOutCount, Model* parent = nullptr);

	/**
	 * Getters
	 */
	//auto coreCountIn() const -> int { return m_coreInCount; }
	//auto coreCountOut() const -> int { return m_coreOutCount; }
	auto channelCountIn() const -> int { return m_pluginInCount; }
	auto channelCountOut() const -> int { return m_pluginOutCount; }

	auto pinMapIn() const -> const PinMap& { return m_inModels; }
	auto pinMapOut() const -> const PinMap& { return m_outModels; }

	auto inputEnabled(std::uint8_t trackChannel, unsigned pluginChannel) const -> bool
	{
		return m_inModels[trackChannel][pluginChannel]->value();
	}

	auto outputEnabled(std::uint8_t trackChannel, unsigned pluginChannel) const -> bool
	{
		return m_outModels[trackChannel][pluginChannel]->value();
	}


	/**
	 * Setters
	 */
	void setChannelCounts(int inCount, int outCount);
	void setChannelCountIn(int inCount);
	void setChannelCountOut(int outCount);

	void setDefaultConnections();

	/*
	 * Routes audio from LMMS track channels to plugin inputs according to the plugin pin connector configuration.
	 *
	 * Iterates through each output channel, mixing together all input audio routed to the output channel.
	 * If no audio is routed to an output channel, the output channel's buffer is zeroed.
	 *
	 * `frames` : number of frames in each `in`/`out` audio buffer
	 * `in`     : track channels from LMMS core (currently just the main track channel pair)
	 * `out`    : plugin input channels in Split form
	 */
	void routeToPlugin(f_cnt_t frames, CoreAudioData in, SplitAudioData<sample_t> out);

	/*
	 * Routes audio from plugin outputs to LMMS track channels according to the plugin pin connector configuration.
	 *
	 * Iterates through each output channel, mixing together all input audio routed to the output channel.
	 * If no audio is routed to an output channel, `inOut` remains unchanged for audio bypass.
	 *
	 * `frames`  : number of frames in each `in`/`out` audio buffer
	 * `in`      : plugin output channels in Split form
	 * `inOut`   : track channels from/to LMMS core (inplace processing)
	 */
	void routeFromPlugin(f_cnt_t frames, SplitAudioData<const sample_t> in, CoreAudioDataMut inOut);


	/**
	 * SerializingObject implementation
	 */
	void saveSettings(QDomDocument& doc, QDomElement& elem) override;
	void loadSettings(const QDomElement& elem) override;
	auto nodeName() const -> QString override { return "pins"; }

	auto instantiateView(QWidget* parent = nullptr) -> gui::PluginPinConnectorWidget*;

signals:
	void channelCountsChanged();

private:
	void updateOptions();

	static void saveSettings(const PinMap& pins, QDomDocument& doc, QDomElement& elem);
	static void loadSettings(const QDomElement& elem, PinMap& pins);

	void setChannelCount(int newCount, PluginPinConnector::PinMap& models, int& oldCount);

	int m_coreInCount = DEFAULT_CHANNELS;
	int m_coreOutCount = DEFAULT_CHANNELS;
	int m_pluginInCount = 0;
	int m_pluginOutCount = 0;

	// TODO: When full routing is added, get LMMS channel counts from bus or router class

	//! Maps LMMS core input to plugin input
	PinMap m_inModels;

	//! Maps LMMS core output to plugin output
	PinMap m_outModels;

	//! Cached values to quickly determine whether a given track channel bypasses the plugin
	//std::vector<bool> m_bypassed; // TODO!

	//! Specifies channel groupings for plugin input channels
	//std::vector<ch_cnt_t> m_pluginInChannelCounts; // TODO!

	//! Specifies channel groupings for plugin output channels
	//std::vector<ch_cnt_t> m_pluginOutChannelCounts; // TODO!
};

} // namespace lmms

#endif // LMMS_PLUGIN_PIN_CONNECTOR_H
