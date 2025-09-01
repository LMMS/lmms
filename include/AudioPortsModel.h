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
 */
class LMMS_EXPORT AudioPortsModel
	: public Model
	, public SerializingObject
{
	Q_OBJECT

public:
	using PinMap = std::vector<std::vector<bool>>;

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
			return m_pins[trackChannel][processorChannel];
		}

		//! Sets a pin connector pin, updates the cache, then emits a dataChanged signal if needed
		void setPin(track_ch_t trackChannel, proc_ch_t processorChannel, bool value);

		/**
		 * Sets a pin connector pin without updating the cache or emitting a dataChanged signal.
		 * Meant for setting multiple pins in one batch efficiently.
		 *
		 * Remember to update the cache and emit the dataChanged signal afterwards!
		 */
		void setPinBatch(track_ch_t trackChannel, proc_ch_t processorChannel, bool value)
		{
			m_pins[trackChannel][processorChannel] = value;
		}

		auto isOutput() const -> bool { return m_isOutput; }

		//! Calls the parent's cache update methods
		void updateCache(track_ch_t trackChannel, proc_ch_t processorChannel);

		friend class AudioPortsModel;

	private:
		void setTrackChannelCount(track_ch_t count);
		void setChannelCount(proc_ch_t count);

		void setDefaultConnections();

		void saveSettings(QDomDocument& doc, QDomElement& elem) const;
		void loadSettings(const QDomElement& elem);

		PinMap m_pins;
		proc_ch_t m_channelCount = 0;
		const bool m_isOutput = false;
		AudioPortsModel* m_parent = nullptr;
	};

	AudioPortsModel(bool isInstrument, Model* parent = nullptr);
	AudioPortsModel(proc_ch_t channelCountIn, proc_ch_t channelCountOut, bool isInstrument, Model* parent = nullptr);

	/**
	 * Getters
	 */
	auto in() -> Matrix& { return m_in; }
	auto in() const -> const Matrix& { return m_in; }
	auto out() -> Matrix& { return m_out; }
	auto out() const -> const Matrix& { return m_out; }
	auto trackChannelCount() const -> track_ch_t { return m_totalTrackChannels; }

	/**
	 * The model is initialized once the number of in/out channels are known.
	 *
	 * Audio processors with a dynamic number of input or output channels must manually set
	 * the channel counts (i.e. with `setChannelCounts()`) to initialize the model.
	 */
	auto initialized() const -> bool { return m_in.m_channelCount != 0 || m_out.m_channelCount != 0; }

	auto isInstrument() const -> bool { return m_isInstrument; }

	/**
	 * Setters
	 */
	void setAllChannelCounts(track_ch_t trackChannels, proc_ch_t inCount, proc_ch_t outCount);
	void setTrackChannelCount(track_ch_t count);
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

protected:
	/**
	 * Called when channel counts or the frame count are changing.
	 *
	 * The parameters will contain the new values, but `in().channelCount()` and `out().channelCount()`
	 * will still return the old values until after this method is called.
	 */
	virtual void bufferPropertiesChanging(proc_ch_t inChannels, proc_ch_t outChannels, f_cnt_t frames) = 0;

	/**
	 * Audio port implementations can override this to provide custom channel names,
	 * otherwise the default channel names are used.
	 */
	virtual auto channelName(proc_ch_t channel, bool isOutput) const -> QString;

	/**
	 * Caches the highest indexed track channel in use, so that the audio ports router can
	 * loop over [0, trackChannelsUpperBound) rather than [0, totalTrackChannels).
	 *
	 * This value is always <= to the total number of track channels (currently always 2).
	 * TODO: Need to recalculate when pins are set/unset
	 */
	auto trackChannelsUpperBound() const -> track_ch_t { return m_trackChannelsUpperBound; }

	/**
	 * Caches whether any processor output channels are routed to a given track channel (meaning the
	 * track channel is used and not "bypassed"), which eliminates need for O(N) checking in
	 * `AudioPorts::Router::receive()`.
	 *
	 * This means usedTrackChannels()[i] == true if and only if out().enabled(i, x) == true
	 * for any audio processor channel x.
	 */
	auto usedTrackChannels() const -> const std::vector<bool>& { return m_usedTrackChannels; }

	/**
	 * Caches whether a given processor output channel is routed to any track channel (meaning the
	 * processor channel is being used), which eliminates need for O(N) checking when detecting
	 * quiet output buffers for a processor that returned `ProcessStatus::ContinueIfNotQuiet`.
	 *
	 * This means usedProcessorChannels()[i] == true if and only if out().enabled(x, i) == true
	 * for any track channel x.
	 */
	auto usedProcessorChannels() const -> const std::vector<bool>& { return m_usedProcessorChannels; }

	/**
	 * Any processor with 2-channel interleaved buffers connected to the track channels in the default
	 * pin configuration (L --> L, R --> R) can be connected directly without any complicated routing.
	 * This greatly simplifies the job of `AudioPorts::Router` and should bring a significant performance boost.
	 *
	 * In theory, the performance when this optimization is enabled (which should be the case for most processors
	 * most of the time) should be about the same as if the processor did not use the pin connector at all.
	 *
	 * This variable caches whether that optimization is currently possible.
	 * When std::nullopt, the optimization is disabled, otherwise the value equals the index of the track channel
	 * pair currently routed to/from the processor.
	 */
	auto directRouting() const -> std::optional<track_ch_t> { return m_directRouting; }

private:
	auto setTrackChannelCountImpl(track_ch_t count) -> bool;
	auto setProcessorChannelCountsImpl(proc_ch_t inCount, proc_ch_t outCount, bool silent) -> bool;

	/*
	 * Cache update methods
	 */

	void updateAllUsedChannels();
	void updateAllUsedTrackChannels();
	void updateAllUsedProcessorChannels();
	void updateUsedTrackChannels(track_ch_t trackChannel);
	void updateUsedProcessorChannels(proc_ch_t outChannel);
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

	/*
	 * The following are cached values meant to improve `AudioPorts::Router` performance
	 */

	track_ch_t m_trackChannelsUpperBound = DEFAULT_CHANNELS;
	std::vector<bool> m_usedTrackChannels;
	std::vector<bool> m_usedProcessorChannels;
	std::optional<track_ch_t> m_directRouting;
};

} // namespace lmms

#endif // LMMS_AUDIO_PORTS_MODEL_H
