/*
 * AudioPorts.h
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

#ifndef LMMS_AUDIO_PORTS_H
#define LMMS_AUDIO_PORTS_H

#include "AudioBufferView.h"
#include "AudioEngine.h"
#include "AudioPortsSettings.h"
#include "AudioPortsModel.h"
#include "Engine.h"
#include "LmmsPolyfill.h"
#include "SampleFrame.h"

namespace lmms
{

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
	const track_ch_t m_channelPairs = 0;
	const f_cnt_t m_frames = 0;
};


namespace detail {

/**
 * Metafunction to select the appropriate non-owning audio buffer view
 * given the layout, sample type, and channel count
 */
template<AudioDataKind kind, bool interleaved, proc_ch_t channels, bool isConst>
struct GetAudioBufferViewTypeHelper
{
	static_assert(always_false_v<GetAudioBufferViewTypeHelper<kind, interleaved, channels, isConst>>,
		"Unsupported audio data type");
};

//! Non-interleaved specialization
template<AudioDataKind kind, proc_ch_t channels, bool isConst>
struct GetAudioBufferViewTypeHelper<kind, false, channels, isConst>
{
	using type = PlanarBufferView<
		std::conditional_t<isConst,
			const GetAudioDataType<kind>,
			GetAudioDataType<kind>
		>, channels>;
};

//! SampleFrame specialization
template<proc_ch_t channels, bool isConst>
struct GetAudioBufferViewTypeHelper<AudioDataKind::SampleFrame, true, channels, isConst>
{
	static_assert(channels == 0 || channels == 2,
		"SampleFrame buffers must have exactly 0 or 2 inputs and outputs");
	using type = std::conditional_t<isConst, std::span<const SampleFrame>, std::span<SampleFrame>>;
};

} // namespace detail


//! Metafunction to select the appropriate non-owning audio buffer view
template<AudioPortsSettings settings, bool isOutput, bool isConst>
using GetAudioBufferViewType = typename detail::GetAudioBufferViewTypeHelper<
	settings.kind, settings.interleaved, (isOutput ? settings.outputs : settings.inputs), isConst>::type;


// Forward declaration
template<AudioPortsSettings settings>
class AudioPorts;

namespace detail {

struct AudioPortsTag {};

//! Interface for accessing input/output audio buffers
template<AudioPortsSettings settings, bool inplace = settings.inplace>
class AudioPortsBuffer;

//! Dynamically in-place specialization
template<AudioPortsSettings settings>
class AudioPortsBuffer<settings, false>
{
public:
	virtual ~AudioPortsBuffer() = default;

	virtual auto input() -> GetAudioBufferViewType<settings, false, false> = 0;
	virtual auto output() -> GetAudioBufferViewType<settings, true, false> = 0;
	virtual auto frames() const -> fpp_t = 0;
	virtual void updateBuffers(proc_ch_t channelsIn, proc_ch_t channelsOut, f_cnt_t frames) = 0;
};

//! Statically in-place specialization
template<AudioPortsSettings settings>
class AudioPortsBuffer<settings, true>
{
public:
	virtual ~AudioPortsBuffer() = default;

	virtual auto inputOutput() -> GetAudioBufferViewType<settings, false, false> = 0;
	virtual auto frames() const -> fpp_t = 0;
	virtual void updateBuffers(proc_ch_t channelsIn, proc_ch_t channelsOut, f_cnt_t frames) = 0;
};


//! Performs pin connector routing. See `AudioPorts::Router`
template<AudioPortsSettings settings, AudioDataKind kind = settings.kind, bool interleaved = settings.interleaved>
class AudioPortsRouter
{
	static_assert(always_false_v<AudioPortsRouter<settings, kind, interleaved>>,
		"A router for the requested settings is not implemented yet");
};

template<AudioPortsSettings settings, class F>
inline void processHelper(AudioPortsRouter<settings>& router, AudioBus<SampleFrame> coreInOut,
	AudioPortsBuffer<settings>& processorBuffers, F&& processFunc)
{
	if constexpr (settings.inplace)
	{
		// Write core to processor input buffer
		const auto processorInOut = processorBuffers.inputOutput();
		router.send(coreInOut, processorInOut);

		// Process
		if constexpr (!settings.buffered) { processFunc(processorInOut); }
		else { processFunc(); }

		// Write processor output buffer to core
		router.receive(processorInOut, coreInOut);
	}
	else
	{
		// Write core to processor input buffer
		const auto processorIn = processorBuffers.input();
		const auto processorOut = processorBuffers.output();
		router.send(coreInOut, processorIn);

		// Process
		if constexpr (!settings.buffered) { processFunc(processorIn, processorOut); }
		else { processFunc(); }

		// Write processor output buffer to core
		router.receive(processorOut, coreInOut);
	}
}

//! Non-SampleFrame specialization
template<AudioPortsSettings settings, AudioDataKind kind>
class AudioPortsRouter<settings, kind, false>
{
	using SampleT = GetAudioDataType<kind>;

public:
	explicit AudioPortsRouter(const AudioPorts<settings>& parent) : m_ap{&parent} {}

	template<class F>
	void process(AudioBus<SampleFrame> inOut, AudioPortsBuffer<settings>& processorBuffers, F&& processFunc)
	{
		processHelper<settings>(*this, inOut, processorBuffers, std::forward<F>(processFunc));
	}

	void send(AudioBus<const SampleFrame> in, PlanarBufferView<SampleT, settings.inputs> out) const;
	void receive(PlanarBufferView<const SampleT, settings.outputs> in, AudioBus<SampleFrame> inOut) const;

private:
	const AudioPorts<settings>* const m_ap;
};

//! SampleFrame specialization
template<AudioPortsSettings settings>
class AudioPortsRouter<settings, AudioDataKind::SampleFrame, true>
{
public:
	explicit AudioPortsRouter(const AudioPorts<settings>& parent) : m_ap{&parent} {}

	/**
	 * Routes core audio to audio processor inputs, calls `processFunc`, then routes audio from
	 * processor outputs back to the core.
	 */
	template<class F>
	void process(AudioBus<SampleFrame> inOut, AudioPortsBuffer<settings>& processorBuffers, F&& processFunc)
	{
		if (const auto dr = m_ap->m_directRouting)
		{
			// The "direct routing" optimization can be applied
			processDirectRouting(inOut.trackChannelPair(*dr), processorBuffers, std::forward<F>(processFunc));
		}
		else
		{
			// Route normally without "direct routing" optimization
			processHelper<settings>(*this, inOut, processorBuffers, std::forward<F>(processFunc));
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
		AudioPortsBuffer<settings>& processorBuffers, F&& processFunc);

	const AudioPorts<settings>* const m_ap;
};

} // namespace detail


/**
 * Interface for an audio port implementation.
 * Contains the audio port model and provides access to the audio buffers.
 *
 * Used by `AudioPlugin` to handle all the customizable aspects of the plugin's
 * audio ports.
 */
template<AudioPortsSettings settings>
class AudioPorts
	: public AudioPortsModel
	, public detail::AudioPortsTag
{
	static_assert(validate<settings>());

public:
	AudioPorts(bool isInstrument, Model* parent = nullptr)
		: AudioPortsModel{settings.inputs, settings.outputs, isInstrument, parent}
	{
	}

	/**
	 * Interface for accessing input/output audio buffers.
	 * Implement this for custom audio port buffers.
	 */
	using Buffer = detail::AudioPortsBuffer<settings>;

	/**
	 * Audio port router
	 *
	 * `process`
	 *     Routes audio to the processor's input buffers, then calls `processFunc` (the processor's process
	 *     method), then routes audio from the processor's output buffers back to LMMS track channels.
	 *
	 *     `inOut`            : track channels from LMMS core (currently just the main track channel pair)
	 *     `processorBuffers` : the processor's AudioPorts::Buffer
	 *     `processFunc`      : the processor's process method - a callable object with the signature
	 *                          `void(buffers...)` where `buffers` is the expected audio buffer(s) (if any)
	 *                          for the given `settings`.
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
	using Router = detail::AudioPortsRouter<settings>;

	template<AudioPortsSettings, AudioDataKind, bool>
	friend class detail::AudioPortsRouter;

	/**
	 * Must be called after constructing an audio port.
	 *
	 * NOTE: This cannot be called in the constructor due
	 *       to the use of a virtual method.
	 */
	void init()
	{
		if (auto buffers = this->buffers())
		{
			buffers->updateBuffers(in().channelCount(), out().channelCount(),
				Engine::audioEngine()->framesPerPeriod());
		}
	}

	/**
	 * Returns true if the audio port can be used.
	 * Active implies `buffers()` is non-null.
	 * Custom audio ports with an unusable state (i.e. a "plugin not loaded" state) must override this.
	 */
	virtual auto active() const -> bool { return true; }

	//! Never nullptr when `active()` is true
	virtual auto buffers() -> Buffer* = 0;

	//! Never nullptr when `active()` is true
	auto constBuffers() const -> const Buffer*
	{
		// const cast to avoid duplicate code - should be safe since buffers() doesn't modify
		return const_cast<AudioPorts*>(this)->buffers();
	}

	auto model() const -> const AudioPortsModel&
	{
		return *static_cast<const AudioPortsModel*>(this);
	}

	auto model() -> AudioPortsModel&
	{
		return *static_cast<AudioPortsModel*>(this);
	}

	auto getRouter() const -> Router { return Router{*this}; }

	static constexpr auto audioPortsSettings() -> AudioPortsSettings { return settings; }
};


/**
 * Custom audio port.
 * AudioPorts::Buffer must be implemented in child class.
 */
template<AudioPortsSettings settings>
class CustomAudioPorts
	: public AudioPorts<settings>
	, public AudioPorts<settings>::Buffer
{
public:
	using AudioPorts<settings>::AudioPorts;

private:
	void bufferPropertiesChanged(proc_ch_t inChannels, proc_ch_t outChannels, f_cnt_t frames) final
	{
		// Connects the audio port model to the buffers
		this->updateBuffers(inChannels, outChannels, frames);
	}
};


namespace detail {

//! Converts between sample types
template<typename Out, typename In>
inline auto convertSample(const In sample) -> Out
{
	if constexpr (std::is_floating_point_v<In> && std::is_floating_point_v<Out>)
	{
		return static_cast<Out>(sample);
	}
	else
	{
		static_assert(always_false_v<In, Out>, "only implemented for floating point samples");
	}
}

// Non-SampleFrame AudioPortsRouter out-of-class definitions

template<AudioPortsSettings settings, AudioDataKind kind>
inline void AudioPortsRouter<settings, kind, false>::send(
	AudioBus<const SampleFrame> in, PlanarBufferView<SampleT, settings.inputs> out) const
{
	if constexpr (settings.inputs == 0) { return; }

	assert(m_ap->in().channelCount() != DynamicChannelCount);
	if (m_ap->in().channelCount() == 0) { return; }

	// Ignore all unused track channels for better performance
	const auto inSizeConstrained = m_ap->m_trackChannelsUpperBound / 2;
	assert(inSizeConstrained <= in.channelPairs());

	for (proc_ch_t outChannel = 0; outChannel < out.channels(); ++outChannel)
	{
		SampleT* outPtr = out.bufferPtr(outChannel);

		// Zero the output buffer
		std::fill_n(outPtr, out.frames(), SampleT{});

		for (std::uint8_t inChannelPairIdx = 0; inChannelPairIdx < inSizeConstrained; ++inChannelPairIdx)
		{
			const SampleFrame* inPtr = in[inChannelPairIdx]; // L/R track channel pair

			const std::uint8_t inChannel = inChannelPairIdx * 2;
			const std::uint8_t enabledPins =
				(static_cast<std::uint8_t>(m_ap->in().enabled(inChannel, outChannel)) << 1u)
				| static_cast<std::uint8_t>(m_ap->in().enabled(inChannel + 1, outChannel));

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

template<AudioPortsSettings settings, AudioDataKind kind>
inline void AudioPortsRouter<settings, kind, false>::receive(
	PlanarBufferView<const SampleT, settings.outputs> in, AudioBus<SampleFrame> inOut) const
{
	if constexpr (settings.outputs == 0) { return; }

	assert(m_ap->out().channelCount() != DynamicChannelCount);
	if (m_ap->out().channelCount() == 0) { return; }

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
			const SampleT* inPtr = in.bufferPtr(inChannel);

			if constexpr (rc == 0b11)
			{
				// This input channel could be routed to either left, right, both, or neither output channels
				if (m_ap->out().enabled(outChannel, inChannel))
				{
					if (m_ap->out().enabled(outChannel + 1, inChannel))
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
				else if (m_ap->out().enabled(outChannel + 1, inChannel))
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
				if (!m_ap->out().enabled(outChannel, inChannel)) { continue; }

				for (f_cnt_t frame = 0; frame < inOut.frames(); ++frame)
				{
					outPtr[frame].leftRef() += inPtr[frame];
				}
			}
			else if constexpr (rc == 0b01)
			{
				// This input channel may or may not be routed to the right output channel
				if (!m_ap->out().enabled(outChannel + 1, inChannel)) { continue; }

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


// SampleFrame AudioPortsRouter out-of-class definitions

template<AudioPortsSettings settings>
inline void AudioPortsRouter<settings, AudioDataKind::SampleFrame, true>::send(
	AudioBus<const SampleFrame> in, std::span<SampleFrame> out) const
{
	if constexpr (settings.inputs == 0) { return; }

	assert(m_ap->in().channelCount() != DynamicChannelCount);
	if (m_ap->in().channelCount() == 0) { return; }
	assert(m_ap->in().channelCount() == 2); // SampleFrame routing only allows exactly 0 or 2 channels

	// Ignore all unused track channels for better performance
	const auto inSizeConstrained = m_ap->m_trackChannelsUpperBound / 2;
	assert(inSizeConstrained <= in.channelPairs());
	assert(out.data() != nullptr);

	// Zero the output buffer
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
			(static_cast<std::uint8_t>(m_ap->in().enabled(inChannel, 0)) << 3u)
			| (static_cast<std::uint8_t>(m_ap->in().enabled(inChannel + 1, 0)) << 2u)
			| (static_cast<std::uint8_t>(m_ap->in().enabled(inChannel, 1)) << 1u)
			| static_cast<std::uint8_t>(m_ap->in().enabled(inChannel + 1, 1));

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

template<AudioPortsSettings settings>
inline void AudioPortsRouter<settings, AudioDataKind::SampleFrame, true>::receive(
	std::span<const SampleFrame> in, AudioBus<SampleFrame> inOut) const
{
	if constexpr (settings.outputs == 0) { return; }

	assert(m_ap->out().channelCount() != DynamicChannelCount);
	if (m_ap->out().channelCount() == 0) { return; }
	assert(m_ap->out().channelCount() == 2); // SampleFrame routing only allows exactly 0 or 2 channels

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
			(static_cast<std::uint8_t>(m_ap->out().enabled(outChannel, 0)) << 3u)
			| (static_cast<std::uint8_t>(m_ap->out().enabled(outChannel, 1)) << 2u)
			| (static_cast<std::uint8_t>(m_ap->out().enabled(outChannel + 1, 0)) << 1u)
			| static_cast<std::uint8_t>(m_ap->out().enabled(outChannel + 1, 1));

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

template<AudioPortsSettings settings>
template<class F>
inline void AudioPortsRouter<settings, AudioDataKind::SampleFrame, true>::processDirectRouting(
	std::span<SampleFrame> coreBuffer, AudioPortsBuffer<settings>& processorBuffers, F&& processFunc)
{
	if constexpr (settings.inplace)
	{
		if constexpr (settings.buffered)
		{
			// Can avoid calling routing methods, but must write to and read from processor's buffers

			// Write core to processor input buffer (if it has one)
			const auto processorInOut = processorBuffers.inputOutput();
			if constexpr (settings.inputs != 0)
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
			if constexpr (settings.outputs != 0)
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
		if constexpr (settings.buffered)
		{
			// Can avoid calling routing methods, but must write to and read from processor's buffers

			// Write core to processor input buffer (if it has one)
			const auto processorIn = processorBuffers.input();
			if constexpr (settings.inputs != 0)
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
			const auto processorOut = processorBuffers.output();
			if constexpr (settings.outputs != 0)
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

			const auto processorIn = processorBuffers.input();
			const auto processorOut = processorBuffers.output();

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

} // namespace detail

} // namespace lmms

#endif // LMMS_AUDIO_PORTS_H
