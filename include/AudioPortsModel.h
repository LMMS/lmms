/*
 * AudioPortsModel.h - Specifies how to route audio channels
 *                     in and out of a plugin.
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
#include <type_traits>
#include <vector>

#include "AudioBuffer.h"
#include "AudioData.h"
#include "AudioPortsConfig.h"
#include "AutomatableModel.h"
#include "SampleFrame.h"
#include "SerializingObject.h"
#include "lmms_basics.h"
#include "lmms_export.h"

class QWidget;

#ifdef LMMS_TESTING
class AudioPortsModelTest;
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
 *   bus[channel pair index][frame index]
 *
 * where
 *   0 <= channel pair index < channelPairs
 *   0 <= frame index < frames
 *
 * TODO C++23: Use std::mdspan
 */
template<typename T>
struct AudioBus
{
	static_assert(std::is_same_v<std::remove_const_t<T>, SampleFrame>);

	AudioBus() = default;
	AudioBus(const AudioBus&) = default;

	AudioBus(T* const* bus, ch_cnt_t channelPairs, f_cnt_t frames)
		: bus{bus}
		, channelPairs{channelPairs}
		, frames{frames}
	{
	}

	template<typename U = T, std::enable_if_t<std::is_const_v<U>, bool> = true>
	AudioBus(const AudioBus<std::remove_const_t<U>>& other)
		: bus{other.bus}
		, channelPairs{other.channelPairs}
		, frames{other.frames}
	{
	}

	auto trackChannelPair(ch_cnt_t pairIndex) const -> std::span<T>
	{
		return {bus[pairIndex], frames};
	}

	T* const* bus = nullptr; //!< [channel pair index][frame index]
	ch_cnt_t channelPairs = 0;
	f_cnt_t frames = 0;
};

using CoreAudioBus = AudioBus<const SampleFrame>;
using CoreAudioBusMut = AudioBus<SampleFrame>;

namespace detail {

template<AudioPortsConfig config, class R, class F>
inline void processHelper(R& router, CoreAudioBusMut coreInOut, AudioBuffer<config>& deviceBuffers, F&& processFunc)
{
	if constexpr (config.inplace)
	{
		// Write core to device input buffer
		const auto deviceInOut = deviceBuffers.inputOutputBuffer();
		router.routeToPlugin(coreInOut, deviceInOut);

		// Process
		if constexpr (!config.buffered) { processFunc(deviceInOut); }
		else { processFunc(); }

		// Write device output buffer to core
		router.routeFromPlugin(deviceInOut, coreInOut);
	}
	else
	{
		// Write core to device input buffer
		const auto deviceIn = deviceBuffers.inputBuffer();
		const auto deviceOut = deviceBuffers.outputBuffer();
		router.routeToPlugin(coreInOut, deviceIn);

		// Process
		if constexpr (!config.buffered) { processFunc(deviceIn, deviceOut); }
		else { processFunc(); }

		// Write device output buffer to core
		router.routeFromPlugin(deviceOut, coreInOut);
	}
}

} // namespace detail


//! Configuration for audio channel routing in/out of plugin
class LMMS_EXPORT AudioPortsModel
	: public Model
	, public SerializingObject
{
	Q_OBJECT

public:
	//! [track channel][plugin channel]
	using PinMap = std::vector<std::vector<BoolModel*>>; // TODO: Experiment with different options to see which has the best performance

	//! A plugin's input or output connections and other info
	class Matrix
	{
	public:
		explicit Matrix(bool isOutput)
			: m_isOutput{isOutput}
		{
		}

		auto pins() const -> const PinMap& { return m_pins; }
		auto pins(ch_cnt_t trackChannel) const -> const std::vector<BoolModel*>& { return m_pins[trackChannel]; }

		auto channelCount() const -> int { return m_channelCount; }

		auto channelName(int channel) const -> QString;

		auto enabled(ch_cnt_t trackChannel, pi_ch_t pluginChannel) const -> bool
		{
			return m_pins[trackChannel][pluginChannel]->value();
		}

		auto isOutput() const -> bool { return m_isOutput; }

		friend class AudioPortsModel;

	private:
		void setTrackChannelCount(AudioPortsModel* parent, int count, const QString& nameFormat);
		void setPluginChannelCount(AudioPortsModel* parent, int count, const QString& nameFormat);

		void setDefaultConnections();

		void saveSettings(QDomDocument& doc, QDomElement& elem) const;
		void loadSettings(const QDomElement& elem);

		PinMap m_pins;
		int m_channelCount = 0;
		const bool m_isOutput = false;
		std::vector<QString> m_channelNames; //!< optional
	};

	AudioPortsModel(bool isInstrument, Model* parent);
	AudioPortsModel(int pluginChannelCountIn, int pluginChannelCountOut, bool isInstrument, Model* parent);

	/**
	 * Getters
	 */
	auto in() const -> const Matrix& { return m_in; }
	auto out() const -> const Matrix& { return m_out; }
	auto trackChannelCount() const -> std::size_t { return m_totalTrackChannels; }

	//! This class is initialized once the number of in/out channels are known TODO: Remove?
	auto initialized() const -> bool { return m_in.m_channelCount != 0 || m_out.m_channelCount != 0; }

	auto isInstrument() const -> bool { return m_isInstrument; }

	/**
	 * Setters
	 */
	void setPluginChannelCounts(int inCount, int outCount);
	void setPluginChannelCountIn(int inCount);
	void setPluginChannelCountOut(int outCount);


	/*
	 * Audio port router
	 *
	 * `process`
	 *     Routes audio to the audio device's input buffers, then calls `processFunc` (the audio device's process
	 *     method), then routes audio from the device's output buffers back to LMMS track channels.
	 *
	 *     `inOut`         : track channels from LMMS core (currently just the main track channel pair)
	 *     `deviceBuffers` : the audio device's AudioBuffer
	 *     `processFunc`   : the audio device's process method - a callable object with the signature
	 *                       `void(buffers...)` where `buffers` is the expected audio buffer(s) (if any)
	 *                       for the given `config`.
	 *
	 * `routeToPlugin`
	 *     Routes audio from LMMS track channels to device inputs according to the audio port configuration.
	 *
	 *     Iterates through each output channel, mixing together all input audio routed to the output channel.
	 *     If no audio is routed to an output channel, the output channel's buffer is zeroed.
	 *
	 *     `in`     : track channels from LMMS core (currently just the main track channel pair)
	 *                `in.frames` provides the number of frames in each `in`/`out` audio buffer
	 *     `out`    : device audio input buffers
	 *
	 * `routeFromPlugin`
	 *     Routes audio from device outputs to LMMS track channels according to the audio port configuration.
	 *
	 *     Iterates through each output channel, mixing together all input audio routed to the output channel.
	 *     If no audio is routed to an output channel, `inOut` remains unchanged for audio bypass behavior.
	 *
	 *     `in`      : device audio output buffers
	 *     `inOut`   : track channels from/to LMMS core
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
		void process(CoreAudioBusMut inOut, AudioBuffer<config>& deviceBuffers, F&& processFunc)
		{
			detail::processHelper<config>(*this, inOut, deviceBuffers, std::forward<F>(processFunc));
		}

		void routeToPlugin(CoreAudioBus in, SplitAudioData<SampleT, config.inputs> out) const;
		void routeFromPlugin(SplitAudioData<const SampleT, config.outputs> in, CoreAudioBusMut inOut) const;

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
		 * Routes core audio to device inputs, calls `processFunc`, then routes audio from plugin
		 * outputs back to the core.
		 */
		template<class F>
		void process(CoreAudioBusMut inOut, AudioBuffer<config>& deviceBuffers, F&& processFunc)
		{
			if (const auto dr = m_ap->m_directRouting)
			{
				// The "direct routing" optimization can be applied
				processDirectRouting(inOut.trackChannelPair(*dr), deviceBuffers, std::forward<F>(processFunc));
			}
			else
			{
				// Route normally without "direct routing" optimization
				detail::processHelper<config>(*this, inOut, deviceBuffers, std::forward<F>(processFunc));
			}
		}

		void routeToPlugin(CoreAudioBus in, std::span<SampleFrame> out) const;
		void routeFromPlugin(std::span<const SampleFrame> in, CoreAudioBusMut inOut) const;

	private:
		/**
		 * Applies the "direct routing" optimization for more efficient processing.
		 * Does not use `routeToPlugin` or `routeFromPlugin` at all.
		 */
		template<class F>
		void processDirectRouting(std::span<SampleFrame> inOut, AudioBuffer<config>& deviceBuffers, F&& processFunc);

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

	static constexpr std::size_t MaxTrackChannels = 256; // TODO: Move somewhere else

#ifdef LMMS_TESTING
	friend class ::AudioPortsModelTest;
#endif

signals:
	//! Called when the plugin channel counts change or the track channel counts change
	//void propertiesChanged(); [from Model base class]

public slots:
	void setTrackChannelCount(int count);
	void updateRoutedChannels(unsigned int trackChannel);

protected:
	/**
	 * To be implemented by the plugin's audio port.
	 * Called when channel counts or sample rate changes.
	 *
	 * NOTE: Virtual method, do not call in constructor.
	 */
	virtual void bufferPropertiesChanged(int inChannels, int outChannels, f_cnt_t frames) {}

private:
	void setPluginChannelCountsImpl(int inCount, int outCount);
	void updateAllRoutedChannels();
	void updateDirectRouting();

	Matrix m_in{false}; //!< LMMS --> Plugin
	Matrix m_out{true}; //!< Plugin --> LMMS

	// TODO: When full routing is added, get LMMS channel counts from bus or audio router class
	std::size_t m_totalTrackChannels = DEFAULT_CHANNELS;

	//! This value is <= to the total number of track channels (currently always 2)
	unsigned int m_trackChannelsUpperBound = DEFAULT_CHANNELS; // TODO: Need to recalculate when pins are set/unset

	/**
	 * Caches whether any plugin output channels are routed to a given track channel (meaning the
	 * track channel is not "bypassed"), which eliminates need for O(N) checking in `routeFromPlugin`.
	 *
	 * This means m_routedChannels[i] == true iif m_out.enabled(i, x) == true for any plugin channel x.
	 */
	std::vector<bool> m_routedChannels; // TODO: Need to calculate when pins are set/unset

	/**
	 * Any SampleFrame-based device (2-channel interleaved) connected to the track channels in the default
	 * pin configuration (L --> L, R --> R) can be connected directly without without the need for the audio port buffers
	 * or the audio port router, which allows a significant performance optimization.
	 *
	 * In theory, the performance when this optimization is enabled (which should be the case for most plugins most
	 * of the time) should be about the same as if the plugin did not use the pin connector at all.
	 *
	 * This variable caches whether that optimization is currently possible.
	 * When std::nullopt, the optimization is disabled, otherwise the value equals the index of the track channel pair
	 * currently routed to/from the device.
	 */
	std::optional<ch_cnt_t> m_directRouting;

	/**
	 * This needs to be known because the default connections (and view?) for instruments with sidechain
	 * inputs is different from effects, even though they may both have the same channel counts.
	 */
	const bool m_isInstrument = false;
};


// Non-SampleFrame Router out-of-class definitions

template<AudioPortsConfig config, AudioDataKind kind>
inline void AudioPortsModel::Router<config, kind, false>::routeToPlugin(
	CoreAudioBus in, SplitAudioData<SampleT, config.inputs> out) const
{
	if constexpr (config.inputs == 0) { return; }

	assert(m_ap->m_in.channelCount() != DynamicChannelCount);
	if (m_ap->m_in.channelCount() == 0) { return; }

	// Ignore all unused track channels for better performance
	const auto inSizeConstrained = m_ap->m_trackChannelsUpperBound / 2;
	assert(inSizeConstrained <= in.channelPairs);

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
			const SampleFrame* inPtr = in.bus[inChannelPairIdx]; // L/R track channel pair

			const std::uint8_t inChannel = inChannelPairIdx * 2;
			const std::uint8_t enabledPins =
				(static_cast<std::uint8_t>(m_ap->m_in.enabled(inChannel, outChannel)) << 1u)
				| static_cast<std::uint8_t>(m_ap->m_in.enabled(inChannel + 1, outChannel));

			switch (enabledPins)
			{
				case 0b00: break;
				case 0b01: // R channel only
				{
					for (f_cnt_t frame = 0; frame < in.frames; ++frame)
					{
						outPtr[frame] += convertSample<SampleT>(inPtr[frame].right());
					}
					break;
				}
				case 0b10: // L channel only
				{
					for (f_cnt_t frame = 0; frame < in.frames; ++frame)
					{
						outPtr[frame] += convertSample<SampleT>(inPtr[frame].left());
					}
					break;
				}
				case 0b11: // Both channels
				{
					for (f_cnt_t frame = 0; frame < in.frames; ++frame)
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
inline void AudioPortsModel::Router<config, kind, false>::routeFromPlugin(
	SplitAudioData<const SampleT, config.outputs> in, CoreAudioBusMut inOut) const
{
	if constexpr (config.outputs == 0) { return; }

	assert(m_ap->m_out.channelCount() != DynamicChannelCount);
	if (m_ap->m_out.channelCount() == 0) { return; }

	// Ignore all unused track channels for better performance
	const auto inOutSizeConstrained = m_ap->m_trackChannelsUpperBound / 2;
	assert(inOutSizeConstrained <= inOut.channelPairs);

	/*
	 * Routes plugin audio to track channel pair and normalizes the result. For track channels
	 * without any plugin audio routed to it, the track channel is unmodified for "bypass"
	 * behavior.
	 */
	const auto routeNx2 = [&](SampleFrame* outPtr, ch_cnt_t outChannel, auto routedChannels) {
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
			std::fill_n(outPtr, inOut.frames, SampleFrame{});
		}
		else
		{
			for (f_cnt_t frame = 0; frame < inOut.frames; ++frame)
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

		for (pi_ch_t inChannel = 0; inChannel < in.channels(); ++inChannel)
		{
			const SampleT* inPtr = in.buffer(inChannel);

			if constexpr (rc == 0b11)
			{
				// This input channel could be routed to either left, right, both, or neither output channels
				if (m_ap->m_out.enabled(outChannel, inChannel))
				{
					if (m_ap->m_out.enabled(outChannel + 1, inChannel))
					{
						for (f_cnt_t frame = 0; frame < inOut.frames; ++frame)
						{
							outPtr[frame].leftRef() += inPtr[frame];
							outPtr[frame].rightRef() += inPtr[frame];
						}
					}
					else
					{
						for (f_cnt_t frame = 0; frame < inOut.frames; ++frame)
						{
							outPtr[frame].leftRef() += inPtr[frame];
						}
					}
				}
				else if (m_ap->m_out.enabled(outChannel + 1, inChannel))
				{
					for (f_cnt_t frame = 0; frame < inOut.frames; ++frame)
					{
						outPtr[frame].rightRef() += inPtr[frame];
					}
				}
			}
			else if constexpr (rc == 0b10)
			{
				// This input channel may or may not be routed to the left output channel
				if (!m_ap->m_out.enabled(outChannel, inChannel)) { continue; }

				for (f_cnt_t frame = 0; frame < inOut.frames; ++frame)
				{
					outPtr[frame].leftRef() += inPtr[frame];
				}
			}
			else if constexpr (rc == 0b01)
			{
				// This input channel may or may not be routed to the right output channel
				if (!m_ap->m_out.enabled(outChannel + 1, inChannel)) { continue; }

				for (f_cnt_t frame = 0; frame < inOut.frames; ++frame)
				{
					outPtr[frame].rightRef() += inPtr[frame];
				}
			}
		}
	};


	for (ch_cnt_t outChannelPairIdx = 0; outChannelPairIdx < inOutSizeConstrained; ++outChannelPairIdx)
	{
		SampleFrame* outPtr = inOut.bus[outChannelPairIdx]; // L/R track channel pair
		const auto outChannel = static_cast<ch_cnt_t>(outChannelPairIdx * 2);

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
inline void AudioPortsModel::Router<config, AudioDataKind::SampleFrame, true>::routeToPlugin(
	CoreAudioBus in, std::span<SampleFrame> out) const
{
	if constexpr (config.inputs == 0) { return; }

	assert(m_ap->m_in.channelCount() != DynamicChannelCount);
	if (m_ap->m_in.channelCount() == 0) { return; }
	assert(m_ap->m_in.channelCount() == 2); // SampleFrame routing only allows exactly 0 or 2 channels

	// Ignore all unused track channels for better performance
	const auto inSizeConstrained = m_ap->m_trackChannelsUpperBound / 2;
	assert(inSizeConstrained <= in.channelPairs);
	assert(out.data() != nullptr);

	// Zero the output buffer - TODO: std::memcpy?
	std::fill(out.begin(), out.end(), SampleFrame{});

	/*
	 * This is essentially a function template with specializations for each
	 * of the 16 total routing combinations of an input `SampleFrame*` to an
	 * output `SampleFrame*`. The purpose is to eliminate all branching within
	 * the inner for-loop in hopes of better performance.
	 */
	auto route2x2 = [samples = in.frames * 2, outPtr = out.data()->data()](const sample_t* inPtr, auto enabledPins) {
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
		const sample_t* inPtr = in.bus[inChannelPairIdx]->data(); // L/R track channel pair

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
inline void AudioPortsModel::Router<config, AudioDataKind::SampleFrame, true>::routeFromPlugin(
	std::span<const SampleFrame> in, CoreAudioBusMut inOut) const
{
	if constexpr (config.outputs == 0) { return; }

	assert(m_ap->m_out.channelCount() != DynamicChannelCount);
	if (m_ap->m_out.channelCount() == 0) { return; }
	assert(m_ap->m_out.channelCount() == 2); // SampleFrame routing only allows exactly 0 or 2 channels

	// Ignore all unused track channels for better performance
	const auto inOutSizeConstrained = m_ap->m_trackChannelsUpperBound / 2;
	assert(inOutSizeConstrained <= inOut.channelPairs);
	assert(in.data() != nullptr);

	/*
	 * This is essentially a function template with specializations for each
	 * of the 16 total routing combinations of an input `SampleFrame*` to an
	 * output `SampleFrame*`. The purpose is to eliminate all branching within
	 * the inner for-loop in hopes of better performance.
	 */
	auto route2x2 = [samples = inOut.frames * 2, inPtr = in.data()->data()](sample_t* outPtr, auto enabledPins) {
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


	for (ch_cnt_t outChannelPairIdx = 0; outChannelPairIdx < inOutSizeConstrained; ++outChannelPairIdx)
	{
		sample_t* outPtr = inOut.bus[outChannelPairIdx]->data(); // L/R track channel pair
		assert(outPtr != nullptr);

		const ch_cnt_t outChannel = outChannelPairIdx * 2;
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
	std::span<SampleFrame> coreBuffer, AudioBuffer<config>& deviceBuffers, F&& processFunc)
{
	if constexpr (config.inplace)
	{
		if constexpr (config.buffered)
		{
			// Can avoid calling routing methods, but must write to and read from device's buffers

			// Write core to device input buffer
			const auto deviceInOut = deviceBuffers.inputOutputBuffer();
			if constexpr (config.inputs != 0)
			{
				if (m_ap->in().channelCount() != 0)
				{
					assert(deviceInOut.data() != nullptr);
					std::memcpy(deviceInOut.data(), coreBuffer.data(), coreBuffer.size_bytes());
				}
			}

			// Process
			processFunc();

			// Write device output buffer to core
			if constexpr (config.outputs != 0)
			{
				if (m_ap->out().channelCount() != 0)
				{
					assert(deviceInOut.data() != nullptr);
					std::memcpy(coreBuffer.data(), deviceInOut.data(), coreBuffer.size_bytes());
				}
			}
		}
		else
		{
			// Can avoid using device's input/output buffers AND avoid calling routing methods
			processFunc(coreBuffer);
		}
	}
	else
	{
		if constexpr (config.buffered)
		{
			// Can avoid calling routing methods, but must write to and read from device's buffers

			// Write core to device input buffer
			const auto deviceIn = deviceBuffers.inputBuffer();
			if constexpr (config.inputs != 0)
			{
				if (m_ap->in().channelCount() != 0)
				{
					assert(deviceIn.data() != nullptr);
					std::memcpy(deviceIn.data(), coreBuffer.data(), coreBuffer.size_bytes());
				}
			}

			// Process
			processFunc();

			// Write device output buffer to core
			const auto deviceOut = deviceBuffers.outputBuffer();
			if constexpr (config.outputs != 0)
			{
				if (m_ap->out().channelCount() != 0)
				{
					assert(deviceOut.data() != nullptr);
					std::memcpy(coreBuffer.data(), deviceOut.data(), coreBuffer.size_bytes());
				}
			}
		}
		else
		{
			// Can avoid calling routing methods, but a buffer copy may be needed

			const auto deviceIn = deviceBuffers.inputBuffer();
			const auto deviceOut = deviceBuffers.outputBuffer();

			// Check if device is dynamically using in-place processing
			if (deviceIn.data() != deviceOut.data() || deviceIn.size() != deviceOut.size())
			{
				// Not using in-place processing - the device implementation may be written under the assumption
				// that the input and output buffers are two different buffers, so we can't break that
				// assumption here. If the device has both inputs and outputs, a buffer copy is needed.

				if (!deviceIn.empty())
				{
					assert(m_ap->in().channelCount() == 2);
					if (!deviceOut.empty())
					{
						// Device has inputs and outputs - need to copy buffer
						assert(m_ap->out().channelCount() == 2);
						assert(deviceOut.data() != nullptr);

						// Process
						processFunc(coreBuffer, deviceOut);

						// Write device output buffer to core
						std::memcpy(coreBuffer.data(), deviceOut.data(), coreBuffer.size_bytes());
					}
					else
					{
						// Input-only device - no buffer copy needed
						processFunc(coreBuffer, deviceOut);
					}
				}
				else
				{
					// Output-only device - no buffer copy needed
					processFunc(deviceIn, coreBuffer);
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
