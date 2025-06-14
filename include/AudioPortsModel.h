/*
 * AudioPortsModel.h - The model for audio ports used by the
 *                     pin connector
 *
 * Copyright (c) 2025 Dalton Messmer <messmer.dalton/at/gmail.com>
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

#ifndef LMMS_AUDIO_PORTS_MODEL_H
#define LMMS_AUDIO_PORTS_MODEL_H

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <optional>
#include <type_traits>
#include <vector>

#include "AutomatableModel.h"
#include "LmmsTypes.h"
#include "SerializingObject.h"
#include "lmms_constants.h"
#include "lmms_export.h"

#ifdef LMMS_TESTING
class AudioPortsTest;
#endif

/**
 * NOTE: The automatable pin functionality is currently disabled out of an abundance
 * of caution (support cannot really be removed once users start using it) and also
 * due to potential performance issues and journalling issues when loading/saving projects
 * (each pin triggers the routed channels and direct routing caches to update).
 *
 * Need a way to update the models of every pin as a single "batch".
 *
 * Set this macro to 1 to enable automatable pin connector pins.
 */
#ifndef PIN_CONNECTOR_AUTOMATABLE_PINS
#define PIN_CONNECTOR_AUTOMATABLE_PINS 0
#endif

namespace lmms
{

namespace gui
{

class PinConnector;

} // namespace gui


/**
 * The model for audio ports used by the pin connector.
 *
 * Contains:
 * - Pin connections for audio routing in/out of an audio processor
 * - Audio port channel counts
 * - Audio port channel names
 *
 * Also provides a nested `Router` class which uses the model
 * to perform routing of audio in and out of the audio processor.
 */
class LMMS_EXPORT AudioPortsModel
	: public Model
	, public SerializingObject
{
	Q_OBJECT

public:
	//! [track channel][audio processor channel]
#if PIN_CONNECTOR_AUTOMATABLE_PINS
	using PinMap = std::vector<std::vector<BoolModel*>>;
#else
	using PinMap = std::vector<std::vector<bool>>;
#endif

	//! A processor's input or output connections and other info
	class Matrix
	{
	public:
		explicit Matrix(AudioPortsModel* parent, bool isOutput)
			: m_isOutput{isOutput}
			, m_parent{parent}
		{
		}

		auto pins() const -> const PinMap& { return m_pins; }
		auto pins(track_ch_t trackChannel) -> const auto& { return m_pins[trackChannel]; }

		auto channelCount() const -> proc_ch_t { return m_channelCount; }
		auto trackChannelCount() const -> track_ch_t { return m_pins.size(); }

		auto channelName(proc_ch_t channel) const -> QString;

		auto enabled(track_ch_t trackChannel, proc_ch_t processorChannel) const -> bool
		{
#if PIN_CONNECTOR_AUTOMATABLE_PINS
			return m_pins[trackChannel][processorChannel]->value();
#else
			return m_pins[trackChannel][processorChannel];
#endif
		}

		//! Sets a pin connector pin, updates the cache, then emits a dataChanged signal if needed
		void setPin(track_ch_t trackChannel, proc_ch_t processorChannel, bool value);

#if !PIN_CONNECTOR_AUTOMATABLE_PINS
		//! Sets a pin connector pin without updating the cache or emitting a dataChanged signal
		void setPinSilent(track_ch_t trackChannel, proc_ch_t processorChannel, bool value)
		{
			// TODO: Is there a way to do this when using AutomatableModel?
			m_pins[trackChannel][processorChannel] = value;
		}
#endif

		auto isOutput() const -> bool { return m_isOutput; }

		//! Calls the parent's updateRoutedChannels and updateDirectRouting methods
		void updateCache(track_ch_t trackChannel);

		friend class AudioPortsModel;

	private:
		void setTrackChannelCount(track_ch_t count, const QString& nameFormat);
		void setChannelCount(proc_ch_t count, const QString& nameFormat);

		void setDefaultConnections();

		void saveSettings(QDomDocument& doc, QDomElement& elem) const;
		void loadSettings(const QDomElement& elem);

		PinMap m_pins;
		proc_ch_t m_channelCount = 0;
		const bool m_isOutput = false;
		AudioPortsModel* m_parent = nullptr;
	};

	AudioPortsModel(bool isInstrument, Model* parent);
	AudioPortsModel(proc_ch_t channelCountIn, proc_ch_t channelCountOut, bool isInstrument, Model* parent);

	/**
	 * Getters
	 */
	auto in() -> Matrix& { return m_in; }
	auto in() const -> const Matrix& { return m_in; }
	auto out() -> Matrix& { return m_out; }
	auto out() const -> const Matrix& { return m_out; }
	auto trackChannelCount() const -> track_ch_t { return m_totalTrackChannels; }

	//! This class is initialized once the number of in/out channels are known TODO: Remove?
	auto initialized() const -> bool { return m_in.m_channelCount != 0 || m_out.m_channelCount != 0; }

	auto isInstrument() const -> bool { return m_isInstrument; }

	/**
	 * Setters
	 */
	void setChannelCounts(proc_ch_t inCount, proc_ch_t outCount);
	void setChannelCountIn(proc_ch_t inCount);
	void setChannelCountOut(proc_ch_t outCount);

	/**
	 * SerializingObject implementation
	 */
	void saveSettings(QDomDocument& doc, QDomElement& elem) override;
	void loadSettings(const QDomElement& elem) override;
	auto nodeName() const -> QString override { return "pins"; }

	virtual auto instantiateView() const -> gui::PinConnector*;

	auto getChannelCountText() const -> QString;

	static constexpr track_ch_t MaxTrackChannels = 256; // TODO: Move somewhere else

#ifdef LMMS_TESTING
	friend class ::AudioPortsTest;
#endif

signals:
	//! Called when channel counts change (whether audio processor or track channel counts)
	//void propertiesChanged(); // from Model

public slots:
	void setTrackChannelCount(track_ch_t count);
	void updateRoutedChannels(track_ch_t trackChannel);

protected:
	/**
	 * To be implemented by the audio ports class.
	 * Called when channel counts or sample rate changes.
	 *
	 * NOTE: Virtual method, do not call in constructor.
	 */
	virtual void bufferPropertiesChanged(proc_ch_t inChannels, proc_ch_t outChannels, f_cnt_t frames) {}

	/**
	 * Audio port implementations can override this to provide custom channel names,
	 * otherwise the default channel names are used.
	 */
	virtual auto channelName(proc_ch_t channel, bool isOutput) const -> QString;

	//! This value is <= to the total number of track channels (currently always 2)
	track_ch_t m_trackChannelsUpperBound = DEFAULT_CHANNELS; // TODO: Need to recalculate when pins are set/unset

	/**
	 * Caches whether any output channels are routed to a given track channel (meaning the
	 * track channel is not "bypassed"), which eliminates need for O(N) checking in `AudioPorts::Router::receive()`.
	 *
	 * This means m_routedChannels[i] == true if and only if m_out.enabled(i, x) == true
	 * for any audio processor channel x.
	 */
	std::vector<bool> m_routedChannels;

	/**
	 * Any SampleFrame-based audio processor (2-channel interleaved) connected to the track channels in the default
	 * pin configuration (L --> L, R --> R) can be connected directly without the need for the audio port
	 * buffers or the audio port router, which allows a significant performance optimization.
	 *
	 * In theory, the performance when this optimization is enabled (which should be the case for most processors
	 * most of the time) should be about the same as if the processor did not use the pin connector at all.
	 *
	 * This variable caches whether that optimization is currently possible.
	 * When std::nullopt, the optimization is disabled, otherwise the value equals the index of the track channel
	 * pair currently routed to/from the processor.
	 */
	std::optional<track_ch_t> m_directRouting;

private:
	void setChannelCountsImpl(proc_ch_t inCount, proc_ch_t outCount);
	void updateAllRoutedChannels();
	void updateDirectRouting();

	Matrix m_in{this, false}; //!< LMMS --> audio processor
	Matrix m_out{this, true}; //!< audio processor --> LMMS

	// TODO: When full routing is added, get LMMS channel counts from bus or audio router class
	track_ch_t m_totalTrackChannels = DEFAULT_CHANNELS;

	/**
	 * This needs to be known because the default connections (and view?) for instruments with sidechain
	 * inputs is different from effects, even though they may both have the same channel counts.
	 */
	const bool m_isInstrument = false;
};

} // namespace lmms

#endif // LMMS_AUDIO_PORTS_MODEL_H
