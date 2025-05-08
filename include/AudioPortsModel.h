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
#include <memory>
#include <optional>
#include <type_traits>
#include <vector>

#include "AudioBuffer.h"
#include "AudioData.h"
#include "AudioPortsConfig.h"
#include "AutomatableModel.h"
#include "LmmsPolyfill.h"
#include "LmmsTypes.h"
#include "SampleFrame.h"
#include "SerializingObject.h"
#include "lmms_export.h"

#ifdef LMMS_TESTING
class AudioPortsModelTest;
#endif

/**
 * NOTE: The automatable pin functionality is currently disabled out of an abundance
 * of caution (support cannot really be removed once users start using it) and also
 * due to potential performance issues when loading/saving projects (each pin triggers
 * the routed channels and direct routing caches to update).
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
 * A non-owning span of std::span<const SampleFrame>.
 *
 * Access like this:
 *   myAudioBus[channel pair index][frame index]
 *
 * where
 *   0 <= channel pair index < channelPairs()
 *   0 <= frame index < frames()
 *
 * TODO C++23: Use std::mdspan
 */
template<typename T>
class AudioBus
{
public:
	static_assert(std::is_same_v<std::remove_const_t<T>, SampleFrame>);

	AudioBus() = default;
	AudioBus(const AudioBus&) = default;

	AudioBus(T* const* bus, track_ch_t channelPairs, f_cnt_t frames)
		: m_bus{bus}
		, m_channelPairs{channelPairs}
		, m_frames{frames}
	{
	}

	template<typename U = T> requires (std::is_const_v<U>)
	AudioBus(const AudioBus<std::remove_const_t<U>>& other)
		: m_bus{other.bus()}
		, m_channelPairs{other.channelPairs()}
		, m_frames{other.frames()}
	{
	}

	auto trackChannelPair(track_ch_t pairIndex) const -> std::span<T>
	{
		return {m_bus[pairIndex], m_frames};
	}

	auto operator[](track_ch_t channelPairIndex) const -> T* { return m_bus[channelPairIndex]; }

	auto bus() const -> T* const* { return m_bus; }
	auto channelPairs() const -> track_ch_t { return m_channelPairs; }
	auto frames() const -> f_cnt_t { return m_frames; }

private:
	T* const* m_bus = nullptr; //!< [channel pair index][frame index]
	track_ch_t m_channelPairs = 0;
	f_cnt_t m_frames = 0;
};


namespace detail {

template<AudioPortsConfig config, class R, class F>
inline void processHelper(R& router, AudioBus<SampleFrame> coreInOut,
	AudioBuffer<config>& processorBuffers, F&& processFunc)
{
	if constexpr (config.inplace)
	{
		// Write core to processor input buffer
		const auto processorInOut = processorBuffers.inputOutputBuffer();
		router.send(coreInOut, processorInOut);

		// Process
		if constexpr (!config.buffered) { processFunc(processorInOut); }
		else { processFunc(); }

		// Write processor output buffer to core
		router.receive(processorInOut, coreInOut);
	}
	else
	{
		// Write core to processor input buffer
		const auto processorIn = processorBuffers.inputBuffer();
		const auto processorOut = processorBuffers.outputBuffer();
		router.send(coreInOut, processorIn);

		// Process
		if constexpr (!config.buffered) { processFunc(processorIn, processorOut); }
		else { processFunc(); }

		// Write processor output buffer to core
		router.receive(processorOut, coreInOut);
	}
}

} // namespace detail


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
		std::vector<QString> m_channelNames; //!< optional
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
	 * Audio port router
	 *
	 * `process`
	 *     Routes audio to the processor's input buffers, then calls `processFunc` (the processor's process
	 *     method), then routes audio from the processor's output buffers back to LMMS track channels.
	 *
	 *     `inOut`            : track channels from LMMS core (currently just the main track channel pair)
	 *     `processorBuffers` : the processor's AudioBuffer
	 *     `processFunc`      : the processor's process method - a callable object with the signature
	 *                          `void(buffers...)` where `buffers` is the expected audio buffer(s) (if any)
	 *                          for the given `config`.
	 *
	 * `send`
	 *     Routes audio from LMMS track channels to processor inputs according to the pin connections.
	 *
	 *     Iterates through each output channel, mixing together all input audio routed to the output channel.
	 *     If no audio is routed to an output channel, the output channel's buffer is zeroed.
	 *
	 *     `in`     : track channels from LMMS core (currently just the main track channel pair)
	 *                `in.frames` provides the number of frames in each `in`/`out` audio buffer
	 *     `out`    : processor input buffers
	 *
	 * `receive`
	 *     Routes audio from processor outputs to LMMS track channels according to the pin connections.
	 *
	 *     Iterates through each output channel, mixing together all input audio routed to the output channel.
	 *     If no audio is routed to an output channel, `inOut` remains unchanged for audio bypass behavior.
	 *
	 *     `in`      : processor output buffers
	 *     `inOut`   : track channels from/to LMMS core (currently just the main track channel pair)
	 *                 `inOut.frames` provides the number of frames in each `in`/`inOut` audio buffer
	 */
	template<AudioPortsConfig config, AudioDataKind kind = config.kind, bool interleaved = config.interleaved>
	class Router
	{
		static_assert(always_false_v<Router<config, kind, interleaved>>,
			"A router for the requested configuration is not implemented yet");
	};

	//! Non-SampleFrame routing
	template<AudioPortsConfig config, AudioDataKind kind>
	class Router<config, kind, false>
	{
		using SampleT = GetAudioDataType<kind>;

	public:
		explicit Router(const AudioPortsModel& parent) : m_ap{&parent} {}

		template<class F>
		void process(AudioBus<SampleFrame> inOut, AudioBuffer<config>& processorBuffers, F&& processFunc)
		{
			detail::processHelper<config>(*this, inOut, processorBuffers, std::forward<F>(processFunc));
		}

		void send(AudioBus<const SampleFrame> in, SplitAudioData<SampleT, config.inputs> out) const;
		void receive(SplitAudioData<const SampleT, config.outputs> in, AudioBus<SampleFrame> inOut) const;

	private:
		const AudioPortsModel* m_ap;
	};

	//! SampleFrame routing
	template<AudioPortsConfig config>
	class Router<config, AudioDataKind::SampleFrame, true>
	{
	public:
		explicit Router(const AudioPortsModel& parent) : m_ap{&parent} {}

		/**
		 * Routes core audio to audio processor inputs, calls `processFunc`, then routes audio from
		 * processor outputs back to the core.
		 */
		template<class F>
		void process(AudioBus<SampleFrame> inOut, AudioBuffer<config>& processorBuffers, F&& processFunc)
		{
			if (const auto dr = m_ap->m_directRouting)
			{
				// The "direct routing" optimization can be applied
				processDirectRouting(inOut.trackChannelPair(*dr), processorBuffers, std::forward<F>(processFunc));
			}
			else
			{
				// Route normally without "direct routing" optimization
				detail::processHelper<config>(*this, inOut, processorBuffers, std::forward<F>(processFunc));
			}
		}

		void send(AudioBus<const SampleFrame> in, std::span<SampleFrame> out) const;
		void receive(std::span<const SampleFrame> in, AudioBus<SampleFrame> inOut) const;

	private:
		/**
		 * Applies the "direct routing" optimization for more efficient processing.
		 * Does not use `send` or `receive` at all.
		 */
		template<class F>
		void processDirectRouting(std::span<SampleFrame> inOut,
			AudioBuffer<config>& processorBuffers, F&& processFunc);

		const AudioPortsModel* m_ap;
	};


	template<AudioPortsConfig config>
	auto getRouter() const -> Router<config>
	{
		return Router<config>{*this};
	}


	/**
	 * SerializingObject implementation
	 */
	void saveSettings(QDomDocument& doc, QDomElement& elem) override;
	void loadSettings(const QDomElement& elem) override;
	auto nodeName() const -> QString override { return "pins"; }

	virtual auto instantiateView() const -> std::unique_ptr<gui::PinConnector>;

	auto getChannelCountText() const -> QString;

	static constexpr track_ch_t MaxTrackChannels = 256; // TODO: Move somewhere else

#ifdef LMMS_TESTING
	friend class ::AudioPortsModelTest;
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

private:
	void setChannelCountsImpl(proc_ch_t inCount, proc_ch_t outCount);
	void updateAllRoutedChannels();
	void updateDirectRouting();

	Matrix m_in{this, false}; //!< LMMS --> audio processor
	Matrix m_out{this, true}; //!< audio processor --> LMMS

	// TODO: When full routing is added, get LMMS channel counts from bus or audio router class
	track_ch_t m_totalTrackChannels = DEFAULT_CHANNELS;

	//! This value is <= to the total number of track channels (currently always 2)
	track_ch_t m_trackChannelsUpperBound = DEFAULT_CHANNELS; // TODO: Need to recalculate when pins are set/unset

	/**
	 * Caches whether any output channels are routed to a given track channel (meaning the
	 * track channel is not "bypassed"), which eliminates need for O(N) checking in `receive`.
	 *
	 * This means m_routedChannels[i] == true iif m_out.enabled(i, x) == true for any audio processor channel x.
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

	/**
	 * This needs to be known because the default connections (and view?) for instruments with sidechain
	 * inputs is different from effects, even though they may both have the same channel counts.
	 */
	const bool m_isInstrument = false;
};


// Non-SampleFrame Router out-of-class definitions

template<AudioPortsConfig config, AudioDataKind kind>
inline void AudioPortsModel::Router<config, kind, false>::send(
	AudioBus<const SampleFrame> in, SplitAudioData<SampleT, config.inputs> out) const
{
	if constexpr (config.inputs == 0) { return; }

	assert(m_ap->m_in.channelCount() != DynamicChannelCount);
	if (m_ap->m_in.channelCount() == 0) { return; }

	// Ignore all unused track channels for better performance
	const auto inSizeConstrained = m_ap->m_trackChannelsUpperBound / 2;
	assert(inSizeConstrained <= in.channelPairs());

	// Zero the output buffer - TODO: std::memcpy?
	{
		auto source = out.sourceBuffer();
		assert(source.data() != nullptr);
		std::fill_n(source.data(), source.size(), SampleT{});
		//std::memset(source.data(), 0, source.size_bytes());
	}

	for (std::uint32_t outChannel = 0; outChannel < out.channels(); ++outChannel)
	{
		SampleT* outPtr = out.buffer(outChannel);

		for (std::uint8_t inChannelPairIdx = 0; inChannelPairIdx < inSizeConstrained; ++inChannelPairIdx)
		{
			const SampleFrame* inPtr = in[inChannelPairIdx]; // L/R track channel pair

			const std::uint8_t inChannel = inChannelPairIdx * 2;
			const std::uint8_t enabledPins =
				(static_cast<std::uint8_t>(m_ap->m_in.enabled(inChannel, outChannel)) << 1u)
				| static_cast<std::uint8_t>(m_ap->m_in.enabled(inChannel + 1, outChannel));

			switch (enabledPins)
			{
				case 0b00: break;
				case 0b01: // R channel only
				{
					for (f_cnt_t frame = 0; frame < in.frames(); ++frame)
					{
						outPtr[frame] += convertSample<SampleT>(inPtr[frame].right());
					}
					break;
				}
				case 0b10: // L channel only
				{
					for (f_cnt_t frame = 0; frame < in.frames(); ++frame)
					{
						outPtr[frame] += convertSample<SampleT>(inPtr[frame].left());
					}
					break;
				}
				case 0b11: // Both channels
				{
					for (f_cnt_t frame = 0; frame < in.frames(); ++frame)
					{
						outPtr[frame] += convertSample<SampleT>(inPtr[frame].left() + inPtr[frame].right());
					}
					break;
				}
				default:
					unreachable();
					break;
			}
		}
	}
}

template<AudioPortsConfig config, AudioDataKind kind>
inline void AudioPortsModel::Router<config, kind, false>::receive(
	SplitAudioData<const SampleT, config.outputs> in, AudioBus<SampleFrame> inOut) const
{
	if constexpr (config.outputs == 0) { return; }

	assert(m_ap->m_out.channelCount() != DynamicChannelCount);
	if (m_ap->m_out.channelCount() == 0) { return; }

	// Ignore all unused track channels for better performance
	const auto inOutSizeConstrained = m_ap->m_trackChannelsUpperBound / 2;
	assert(inOutSizeConstrained <= inOut.channelPairs());

	/*
	 * Routes processor audio to track channel pair and normalizes the result. For track channels
	 * without any processor audio routed to it, the track channel is unmodified for "bypass"
	 * behavior.
	 */
	const auto routeNx2 = [&](SampleFrame* outPtr, track_ch_t outChannel, auto routedChannels) {
		constexpr std::uint8_t rc = routedChannels();

		if constexpr (rc == 0b00)
		{
			// Both track channels bypassed - nothing to do
			return;
		}

		// We know at this point that we are writing to at least one of the output channels
		// rather than bypassing, so it is safe to set the output buffer of those channels
		// to zero prior to accumulation

		if constexpr (rc == 0b11)
		{
			// TODO: std::memcpy?
			std::fill_n(outPtr, inOut.frames(), SampleFrame{});
		}
		else
		{
			for (f_cnt_t frame = 0; frame < inOut.frames(); ++frame)
			{
				if constexpr ((rc & 0b10) != 0)
				{
					outPtr[frame].leftRef() = 0.f;
				}
				if constexpr ((rc & 0b01) != 0)
				{
					outPtr[frame].rightRef() = 0.f;
				}
			}
		}

		for (proc_ch_t inChannel = 0; inChannel < in.channels(); ++inChannel)
		{
			const SampleT* inPtr = in.buffer(inChannel);

			if constexpr (rc == 0b11)
			{
				// This input channel could be routed to either left, right, both, or neither output channels
				if (m_ap->m_out.enabled(outChannel, inChannel))
				{
					if (m_ap->m_out.enabled(outChannel + 1, inChannel))
					{
						for (f_cnt_t frame = 0; frame < inOut.frames(); ++frame)
						{
							outPtr[frame].leftRef() += inPtr[frame];
							outPtr[frame].rightRef() += inPtr[frame];
						}
					}
					else
					{
						for (f_cnt_t frame = 0; frame < inOut.frames(); ++frame)
						{
							outPtr[frame].leftRef() += inPtr[frame];
						}
					}
				}
				else if (m_ap->m_out.enabled(outChannel + 1, inChannel))
				{
					for (f_cnt_t frame = 0; frame < inOut.frames(); ++frame)
					{
						outPtr[frame].rightRef() += inPtr[frame];
					}
				}
			}
			else if constexpr (rc == 0b10)
			{
				// This input channel may or may not be routed to the left output channel
				if (!m_ap->m_out.enabled(outChannel, inChannel)) { continue; }

				for (f_cnt_t frame = 0; frame < inOut.frames(); ++frame)
				{
					outPtr[frame].leftRef() += inPtr[frame];
				}
			}
			else if constexpr (rc == 0b01)
			{
				// This input channel may or may not be routed to the right output channel
				if (!m_ap->m_out.enabled(outChannel + 1, inChannel)) { continue; }

				for (f_cnt_t frame = 0; frame < inOut.frames(); ++frame)
				{
					outPtr[frame].rightRef() += inPtr[frame];
				}
			}
		}
	};


	for (std::uint8_t outChannelPairIdx = 0; outChannelPairIdx < inOutSizeConstrained; ++outChannelPairIdx)
	{
		SampleFrame* outPtr = inOut[outChannelPairIdx]; // L/R track channel pair
		const auto outChannel = static_cast<track_ch_t>(outChannelPairIdx * 2);

		const std::uint8_t routedChannels =
				(static_cast<std::uint8_t>(m_ap->m_routedChannels[outChannel]) << 1u)
				| static_cast<std::uint8_t>(m_ap->m_routedChannels[outChannel + 1]);

		switch (routedChannels)
		{
			case 0b00:
				// Both track channels are bypassed, so nothing is allowed to be written to output
				break;
			case 0b01:
				routeNx2(outPtr, outChannel, std::integral_constant<std::uint8_t, 0b01>{});
				break;
			case 0b10:
				routeNx2(outPtr, outChannel, std::integral_constant<std::uint8_t, 0b10>{});
				break;
			case 0b11:
				routeNx2(outPtr, outChannel, std::integral_constant<std::uint8_t, 0b11>{});
				break;
			default:
				unreachable();
				break;
		}
	}
}

// SampleFrame Router out-of-class definitions

template<AudioPortsConfig config>
inline void AudioPortsModel::Router<config, AudioDataKind::SampleFrame, true>::send(
	AudioBus<const SampleFrame> in, std::span<SampleFrame> out) const
{
	if constexpr (config.inputs == 0) { return; }

	assert(m_ap->m_in.channelCount() != DynamicChannelCount);
	if (m_ap->m_in.channelCount() == 0) { return; }
	assert(m_ap->m_in.channelCount() == 2); // SampleFrame routing only allows exactly 0 or 2 channels

	// Ignore all unused track channels for better performance
	const auto inSizeConstrained = m_ap->m_trackChannelsUpperBound / 2;
	assert(inSizeConstrained <= in.channelPairs());
	assert(out.data() != nullptr);

	// Zero the output buffer - TODO: std::memcpy?
	std::fill(out.begin(), out.end(), SampleFrame{});

	/*
	 * This is essentially a function template with specializations for each
	 * of the 16 total routing combinations of an input `SampleFrame*` to an
	 * output `SampleFrame*`. The purpose is to eliminate all branching within
	 * the inner for-loop in hopes of better performance.
	 */
	auto route2x2 = [samples = in.frames() * 2, outPtr = out.data()->data()](const sample_t* inPtr, auto enabledPins) {
		constexpr auto epL =  static_cast<std::uint8_t>(enabledPins() >> 2); // for L out channel
		constexpr auto epR = static_cast<std::uint8_t>(enabledPins() & 0b0011); // for R out channel

		if constexpr (enabledPins() == 0) { return; }

		for (f_cnt_t sampleIdx = 0; sampleIdx < samples; sampleIdx += 2)
		{
			// Route to left output channel
			if constexpr ((epL & 0b01) != 0)
			{
				outPtr[sampleIdx] += inPtr[sampleIdx + 1];
			}
			if constexpr ((epL & 0b10) != 0)
			{
				outPtr[sampleIdx] += inPtr[sampleIdx];
			}

			// Route to right output channel
			if constexpr ((epR & 0b01) != 0)
			{
				outPtr[sampleIdx + 1] += inPtr[sampleIdx + 1];
			}
			if constexpr ((epR & 0b10) != 0)
			{
				outPtr[sampleIdx + 1] += inPtr[sampleIdx];
			}
		}
	};


	for (std::uint8_t inChannelPairIdx = 0; inChannelPairIdx < inSizeConstrained; ++inChannelPairIdx)
	{
		const sample_t* inPtr = in[inChannelPairIdx]->data(); // L/R track channel pair

		const std::uint8_t inChannel = inChannelPairIdx * 2;
		const std::uint8_t enabledPins =
			(static_cast<std::uint8_t>(m_ap->m_in.enabled(inChannel, 0)) << 3u)
			| (static_cast<std::uint8_t>(m_ap->m_in.enabled(inChannel + 1, 0)) << 2u)
			| (static_cast<std::uint8_t>(m_ap->m_in.enabled(inChannel, 1)) << 1u)
			| static_cast<std::uint8_t>(m_ap->m_in.enabled(inChannel + 1, 1));

		switch (enabledPins)
		{
			case 0: break;
			case 1: route2x2(inPtr, std::integral_constant<std::uint8_t, 1>{}); break;
			case 2: route2x2(inPtr, std::integral_constant<std::uint8_t, 2>{}); break;
			case 3: route2x2(inPtr, std::integral_constant<std::uint8_t, 3>{}); break;
			case 4: route2x2(inPtr, std::integral_constant<std::uint8_t, 4>{}); break;
			case 5: route2x2(inPtr, std::integral_constant<std::uint8_t, 5>{}); break;
			case 6: route2x2(inPtr, std::integral_constant<std::uint8_t, 6>{}); break;
			case 7: route2x2(inPtr, std::integral_constant<std::uint8_t, 7>{}); break;
			case 8: route2x2(inPtr, std::integral_constant<std::uint8_t, 8>{}); break;
			case 9: route2x2(inPtr, std::integral_constant<std::uint8_t, 9>{}); break;
			case 10: route2x2(inPtr, std::integral_constant<std::uint8_t, 10>{}); break;
			case 11: route2x2(inPtr, std::integral_constant<std::uint8_t, 11>{}); break;
			case 12: route2x2(inPtr, std::integral_constant<std::uint8_t, 12>{}); break;
			case 13: route2x2(inPtr, std::integral_constant<std::uint8_t, 13>{}); break;
			case 14: route2x2(inPtr, std::integral_constant<std::uint8_t, 14>{}); break;
			case 15: route2x2(inPtr, std::integral_constant<std::uint8_t, 15>{}); break;
			default:
				unreachable();
				break;
		}
	}
}

template<AudioPortsConfig config>
inline void AudioPortsModel::Router<config, AudioDataKind::SampleFrame, true>::receive(
	std::span<const SampleFrame> in, AudioBus<SampleFrame> inOut) const
{
	if constexpr (config.outputs == 0) { return; }

	assert(m_ap->m_out.channelCount() != DynamicChannelCount);
	if (m_ap->m_out.channelCount() == 0) { return; }
	assert(m_ap->m_out.channelCount() == 2); // SampleFrame routing only allows exactly 0 or 2 channels

	// Ignore all unused track channels for better performance
	const auto inOutSizeConstrained = m_ap->m_trackChannelsUpperBound / 2;
	assert(inOutSizeConstrained <= inOut.channelPairs());
	assert(in.data() != nullptr);

	/*
	 * This is essentially a function template with specializations for each
	 * of the 16 total routing combinations of an input `SampleFrame*` to an
	 * output `SampleFrame*`. The purpose is to eliminate all branching within
	 * the inner for-loop in hopes of better performance.
	 */
	auto route2x2 = [samples = inOut.frames() * 2, inPtr = in.data()->data()](sample_t* outPtr, auto enabledPins) {
		constexpr auto epL =  static_cast<std::uint8_t>(enabledPins() >> 2); // for L out channel
		constexpr auto epR = static_cast<std::uint8_t>(enabledPins() & 0b0011); // for R out channel

		if constexpr (enabledPins() == 0) { return; }

		// We know at this point that we are writing to at least one of the output channels
		// rather than bypassing, so it is safe to overwrite the contents of the output buffer

		for (f_cnt_t sampleIdx = 0; sampleIdx < samples; sampleIdx += 2)
		{
			// Route to left output channel
			if constexpr (epL == 0b11)
			{
				outPtr[sampleIdx] = inPtr[sampleIdx] + inPtr[sampleIdx + 1];
			}
			else if constexpr (epL == 0b01)
			{
				outPtr[sampleIdx] = inPtr[sampleIdx + 1];
			}
			else if constexpr (epL == 0b10)
			{
				outPtr[sampleIdx] = inPtr[sampleIdx];
			}

			// Route to right output channel
			if constexpr (epR == 0b11)
			{
				outPtr[sampleIdx + 1] = inPtr[sampleIdx] + inPtr[sampleIdx + 1];
			}
			else if constexpr (epR == 0b01)
			{
				outPtr[sampleIdx + 1] = inPtr[sampleIdx + 1];
			}
			else if constexpr (epR == 0b10)
			{
				outPtr[sampleIdx + 1] = inPtr[sampleIdx];
			}
		}
	};


	for (std::uint8_t outChannelPairIdx = 0; outChannelPairIdx < inOutSizeConstrained; ++outChannelPairIdx)
	{
		sample_t* outPtr = inOut[outChannelPairIdx]->data(); // L/R track channel pair
		assert(outPtr != nullptr);

		const auto outChannel = static_cast<track_ch_t>(outChannelPairIdx * 2);
		const std::uint8_t enabledPins =
			(static_cast<std::uint8_t>(m_ap->m_out.enabled(outChannel, 0)) << 3u)
			| (static_cast<std::uint8_t>(m_ap->m_out.enabled(outChannel, 1)) << 2u)
			| (static_cast<std::uint8_t>(m_ap->m_out.enabled(outChannel + 1, 0)) << 1u)
			| static_cast<std::uint8_t>(m_ap->m_out.enabled(outChannel + 1, 1));

		switch (enabledPins)
		{
			case 0: break;
			case 1: route2x2(outPtr, std::integral_constant<std::uint8_t, 1>{}); break;
			case 2: route2x2(outPtr, std::integral_constant<std::uint8_t, 2>{}); break;
			case 3: route2x2(outPtr, std::integral_constant<std::uint8_t, 3>{}); break;
			case 4: route2x2(outPtr, std::integral_constant<std::uint8_t, 4>{}); break;
			case 5: route2x2(outPtr, std::integral_constant<std::uint8_t, 5>{}); break;
			case 6: route2x2(outPtr, std::integral_constant<std::uint8_t, 6>{}); break;
			case 7: route2x2(outPtr, std::integral_constant<std::uint8_t, 7>{}); break;
			case 8: route2x2(outPtr, std::integral_constant<std::uint8_t, 8>{}); break;
			case 9: route2x2(outPtr, std::integral_constant<std::uint8_t, 9>{}); break;
			case 10: route2x2(outPtr, std::integral_constant<std::uint8_t, 10>{}); break;
			case 11: route2x2(outPtr, std::integral_constant<std::uint8_t, 11>{}); break;
			case 12: route2x2(outPtr, std::integral_constant<std::uint8_t, 12>{}); break;
			case 13: route2x2(outPtr, std::integral_constant<std::uint8_t, 13>{}); break;
			case 14: route2x2(outPtr, std::integral_constant<std::uint8_t, 14>{}); break;
			case 15: route2x2(outPtr, std::integral_constant<std::uint8_t, 15>{}); break;
			default:
				unreachable();
				break;
		}
	}
}

template<AudioPortsConfig config>
template<class F>
inline void AudioPortsModel::Router<config, AudioDataKind::SampleFrame, true>::processDirectRouting(
	std::span<SampleFrame> coreBuffer, AudioBuffer<config>& processorBuffers, F&& processFunc)
{
	if constexpr (config.inplace)
	{
		if constexpr (config.buffered)
		{
			// Can avoid calling routing methods, but must write to and read from processor's buffers

			// Write core to processor input buffer (if it has one)
			const auto processorInOut = processorBuffers.inputOutputBuffer();
			if constexpr (config.inputs != 0)
			{
				if (m_ap->in().channelCount() != 0)
				{
					assert(processorInOut.data() != nullptr);
					std::memcpy(processorInOut.data(), coreBuffer.data(), coreBuffer.size_bytes());
				}
			}

			// Process
			processFunc();

			// Write processor output buffer (if it has one) to core
			if constexpr (config.outputs != 0)
			{
				if (m_ap->out().channelCount() != 0)
				{
					assert(processorInOut.data() != nullptr);
					std::memcpy(coreBuffer.data(), processorInOut.data(), coreBuffer.size_bytes());
				}
			}
		}
		else
		{
			// Can avoid using processor's input/output buffers AND avoid calling routing methods
			processFunc(coreBuffer);
		}
	}
	else
	{
		if constexpr (config.buffered)
		{
			// Can avoid calling routing methods, but must write to and read from processor's buffers

			// Write core to processor input buffer (if it has one)
			const auto processorIn = processorBuffers.inputBuffer();
			if constexpr (config.inputs != 0)
			{
				if (m_ap->in().channelCount() != 0)
				{
					assert(processorIn.data() != nullptr);
					std::memcpy(processorIn.data(), coreBuffer.data(), coreBuffer.size_bytes());
				}
			}

			// Process
			processFunc();

			// Write processor output buffer (if it has one) to core
			const auto processorOut = processorBuffers.outputBuffer();
			if constexpr (config.outputs != 0)
			{
				if (m_ap->out().channelCount() != 0)
				{
					assert(processorOut.data() != nullptr);
					std::memcpy(coreBuffer.data(), processorOut.data(), coreBuffer.size_bytes());
				}
			}
		}
		else
		{
			// Can avoid calling routing methods, but a buffer copy may be needed

			const auto processorIn = processorBuffers.inputBuffer();
			const auto processorOut = processorBuffers.outputBuffer();

			// Check if processor is dynamically using in-place processing
			if (processorIn.data() != processorOut.data() || processorIn.size() != processorOut.size())
			{
				// Not using in-place processing - the processor implementation may be written under the
				// assumption that the input and output buffers are two different buffers, so we can't break
				// that assumption here. If the processor has both inputs and outputs, a buffer copy is needed.

				if (!processorIn.empty())
				{
					assert(m_ap->in().channelCount() == 2);
					if (!processorOut.empty())
					{
						// Processor has inputs and outputs - need to copy buffer
						assert(m_ap->out().channelCount() == 2);
						assert(processorOut.data() != nullptr);

						// Process
						processFunc(coreBuffer, processorOut);

						// Write processor output buffer to core
						std::memcpy(coreBuffer.data(), processorOut.data(), coreBuffer.size_bytes());
					}
					else
					{
						// Input-only processor - no buffer copy needed
						processFunc(coreBuffer, processorOut);
					}
				}
				else
				{
					// Output-only processor - no buffer copy needed
					processFunc(processorIn, coreBuffer);
				}
			}
			else
			{
				// Using in-place processing - the input and output buffers
				// are allowed to be the same buffer, so no buffer copy is needed.
				processFunc(coreBuffer, coreBuffer);
			}
		}
	}
}


} // namespace lmms

#endif // LMMS_AUDIO_PORTS_MODEL_H
