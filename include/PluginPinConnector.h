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

#include <vector>

#include "AutomatableModel.h"
#include "lmms_export.h"
#include "SampleFrame.h"
#include "SerializingObject.h"

class QWidget;

namespace lmms
{

namespace gui
{

class PluginPinConnectorView;

} // namespace gui

//! Configure channel routing for a plugin's mono/stereo in/out ports
class LMMS_EXPORT PluginPinConnector
	: public Model
	, public SerializingObject
{
	Q_OBJECT

public:
	//! [LMMS track channel][plugin channel]
	using PinMap = std::vector<std::vector<BoolModel*>>;

	struct Matrix
	{
		explicit Matrix(bool isIn) : in{isIn} {}

		auto channelName(int channel) const -> QString;

		auto enabled(std::uint8_t trackChannel, unsigned pluginChannel) const -> bool
		{
			return pins[trackChannel][pluginChannel]->value();
		}

		PinMap pins;
		int channelCount = 0;
		std::vector<QString> channelNames; //!< optional
		bool in; //!< true: LMMS-to-plugin; false: plugin-to-LMMS

		// TODO: Channel groupings, port configurations, ...
	};

	PluginPinConnector(Model* parent = nullptr);
	PluginPinConnector(int pluginInCount, int pluginOutCount, Model* parent = nullptr);

	/**
	 * Getters
	 */
	auto in() const -> const Matrix& { return m_in; };
	auto out() const -> const Matrix& { return m_out; };
	auto trackChannelsCount() const -> std::size_t { return s_totalTrackChannels; }
	auto trackChannelsUsed() const -> unsigned int { return m_trackChannelsUsed; }

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

	auto instantiateView(QWidget* parent = nullptr) -> gui::PluginPinConnectorView*;
	auto getChannelCountText() const -> QString;

	static constexpr std::size_t MaxTrackChannels = 256; // TODO: Move somewhere else

public slots:
	void updateTrackChannels(int count);

private:
	static void saveSettings(const Matrix& matrix, QDomDocument& doc, QDomElement& elem);
	static void loadSettings(const QDomElement& elem, Matrix& matrix);

	void setChannelCount(int newCount, Matrix& matrix);

	Matrix m_in{true};   //!< LMMS --> Plugin
	Matrix m_out{false}; //!< Plugin --> LMMS

	//! TODO: Move this somewhere else; Will be >= 2 once there is support for adding new track channels
	static constexpr std::size_t s_totalTrackChannels = DEFAULT_CHANNELS;

	//! This value is <= to the total number of track channels (currently always 2)
	unsigned int m_trackChannelsUsed = DEFAULT_CHANNELS;

	// TODO: When full routing is added, get LMMS channel counts from bus or router class
};

} // namespace lmms

#endif // LMMS_PLUGIN_PIN_CONNECTOR_H
