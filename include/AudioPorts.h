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
#include "AudioBus.h"
#include "AudioEngine.h"
#include "AudioPortsSettings.h"
#include "AudioPortsModel.h"
#include "Engine.h"
#include "LmmsPolyfill.h"

namespace lmms
{

//! Tells the Core what to do with a processor after processing
enum class ProcessStatus
{
	/**
	 * Unconditionally continue processing.
	 *
	 * NOTE: Instruments currently only support this option, so they must return this value.
	 */
	Continue,

	/**
	 * When the "Keep effects running even without input" setting is off (i.e. "auto-quit" is on):
	 *    The samples of each processor output channel currently in use (that is, output channels routed to
	 *    a track channel) are compared to a silence threshold, and if all the samples are silent for enough
	 *    periods (see `Effect::handleAutoQuit`), the plugin will be put to sleep (`ProcessStatus::Sleep`).
	 *
	 * Otherwise, this is equivalent to `ProcessStatus::Continue`.
	 */
	ContinueIfNotQuiet,

	//! Do not continue processing
	Sleep
};


namespace detail {

//! Metafunction to select the appropriate non-owning audio buffer view
template<AudioPortsSettings settings, bool isOutput, bool isConst>
class GetAudioBufferViewTypeHelper
{
	static constexpr auto s_channels = settings.inplace
		? std::max(settings.inputs, settings.outputs)
		: (isOutput ? settings.outputs : settings.inputs);

public:
	using SampleT = std::conditional_t<isConst,
		const GetAudioDataType<settings.kind>,
		GetAudioDataType<settings.kind>
	>;

	using type = std::conditional_t<settings.interleaved,
		InterleavedBufferView<SampleT, s_channels>,
		PlanarBufferView<SampleT, s_channels>
	>;
};

} // namespace detail


//! Metafunction to select the appropriate non-owning audio buffer view
template<AudioPortsSettings settings, bool isOutput, bool isConst>
using GetAudioBufferViewType = typename detail::GetAudioBufferViewTypeHelper<settings, isOutput, isConst>::type;


// Forward declaration
template<AudioPortsSettings settings>
class AudioPorts;

namespace detail {

/**
 * Base interface for input/output audio buffers.
 * Only the methods found here are safe to call when the buffers are not initialized.
 */
class AudioPortsBufferBase
{
public:
	virtual ~AudioPortsBufferBase() = default;

	//! Whether the buffers can be used
	virtual auto initialized() const -> bool = 0;

	//! May not return a meaningful result unless initialized
	virtual auto frames() const -> fpp_t = 0;

	/**
	 * Initialize or update the buffers.
	 * Never call while `AudioPorts` is active because the processor could be using the buffers.
	 *
	 * TODO: Rework plugin threading conventions + introduce plugin state (inactive/active/processing)
	 *       so that this can be safely called after buffers are already initialized.
	 */
	virtual void updateBuffers(proc_ch_t channelsIn, proc_ch_t channelsOut, f_cnt_t frames) = 0;
};

/**
 * Interface for accessing input/output audio buffers.
 * Do not call the methods here unless the buffers are initialized.
 */
template<AudioPortsSettings settings, bool inplace = settings.inplace>
class AudioPortsBuffer;

//! Dynamically in-place specialization
template<AudioPortsSettings settings>
class AudioPortsBuffer<settings, false> : public AudioPortsBufferBase
{
public:
	virtual auto input() -> GetAudioBufferViewType<settings, false, false> = 0;
	virtual auto output() -> GetAudioBufferViewType<settings, true, false> = 0;
};

//! Statically in-place specialization
template<AudioPortsSettings settings>
class AudioPortsBuffer<settings, true> : public AudioPortsBufferBase
{
public:
	virtual auto inputOutput() -> GetAudioBufferViewType<settings, false, false> = 0;
};


//! Performs pin connector routing. See `AudioPorts::Router`.
template<AudioPortsSettings settings>
class AudioPortsRouter
{
	using SampleT = GetAudioDataType<settings.kind>;

	static_assert(!settings.interleaved || settings.sampleFrameCompatible(),
		"AudioPorts::Router currently only supports interleaved buffers if they are SampleFrame-compatible");

public:
	explicit AudioPortsRouter(const AudioPorts<settings>& parent)
		: m_ap{&parent}
	{
	}

	/**
	 * Routes core audio to the processor's input buffers, calls `processFunc` (the processor's process
	 * method), then routes audio from the processor's output buffers back to the track channels.
	 *
	 * `coreInOut`        : track channels from LMMS core (currently just the main track channel pair)
	 * `processorBuffers` : the processor's `AudioPorts::Buffer` TODO: The Router shouldn't need to be given this by users
	 * `processFunc`      : the processor's process method - a callable object with the signature
	 *                      `ProcessStatus(buffers...)` where `buffers` is the expected audio buffer(s)
	 *                      (if any) for the given `settings`.
	 */
	template<class F>
	auto process(AudioBus& coreInOut, AudioPortsBuffer<settings>& processorBuffers, F&& processFunc)
		-> ProcessStatus
	{
		m_silentOutput = false;

		ProcessStatus status;
		if constexpr (settings.sampleFrameCompatible())
		{
			if (const auto dr = m_ap->directRouting())
			{
				// The "direct routing" optimization can be applied
				status = processWithDirectRouting(coreInOut.trackChannelPair(*dr),
					processorBuffers, std::forward<F>(processFunc));
			}
			else
			{
				status = processNormally(coreInOut, processorBuffers, std::forward<F>(processFunc));
			}
		}
		else
		{
			status = processNormally(coreInOut, processorBuffers, std::forward<F>(processFunc));
		}

		coreInOut.sanitize(*m_ap);

		// Update silence status for track channels the processor wrote to
		if (!m_ap->isInstrument()) // TODO: Remove condition once NotePlayHandle-based instruments are supported?
		{
			m_silentOutput = coreInOut.update(*m_ap);
		}

		return status;
	}

	/**
	 * Whether the processor output channels currently in use (processor channels that are routed to
	 * at least one track channel) are silent.
	 *
	 * Only calculated for effects. Otherwise the outputs are assumed to not be silent.
	 */
	auto silentOutput() const -> bool { return m_silentOutput; }

#ifdef LMMS_TESTING
	friend class ::AudioPortsTest;
#endif

private:
	/**
	 * `send`
	 *     Routes audio from track channels to processor inputs according to the pin connections.
	 *
	 *     For each processor input channel, mixes together audio from all track channels
	 *     that are routed to that processor input channel, then writes the result to that channel.
	 *     If no audio is routed to a processor input channel, that channel's buffer is zeroed.
	 *
	 *     `in`     : track channels from LMMS core (currently just the main track channel pair)
	 *                `in.frames` provides the number of frames in each `in`/`out` audio buffer
	 *     `out`    : processor input buffers
	 *
	 * `receive`
	 *     Routes audio from processor outputs to track channels according to the pin connections.
	 *
	 *     For each track channel, mixes together audio from all processor output channels
	 *     that are routed to that track channel, then writes the result to that channel.
	 *     If no audio is routed to a track channel, that channel remains unchanged for "passthrough" behavior.
	 *
	 *     `in`      : processor output buffers
	 *     `inOut`   : track channels from/to LMMS core (currently just the main track channel pair)
	 *                 `inOut.frames` provides the number of frames in each `in`/`inOut` audio buffer
	 */

	void send(const AudioBus& in, PlanarBufferView<SampleT, settings.inputs> out) const
		requires (!settings.interleaved);
	void receive(PlanarBufferView<const SampleT, settings.outputs> in, AudioBus& inOut) const
		requires (!settings.interleaved);

	void send(const AudioBus& in, InterleavedBufferView<float, settings.inputs> out) const
		requires (settings.interleaved);
	void receive(InterleavedBufferView<const float, settings.outputs> in, AudioBus& inOut) const
		requires (settings.interleaved);

	/**
	 * Processes audio normally (no "direct routing").
	 * Uses `send` and `receive` to route audio to and from the processor.
	 */
	template<class F>
	auto processNormally(AudioBus& coreInOut, AudioPortsBuffer<settings>& processorBuffers, F&& processFunc)
		-> ProcessStatus;

	/**
	 * Processes audio using the "direct routing" optimization for more efficient processing.
	 * Does not use `send` or `receive` at all.
	 */
	template<class F>
	auto processWithDirectRouting(InterleavedBufferView<float, 2> coreBuffer,
		AudioPortsBuffer<settings>& processorBuffers, F&& processFunc) -> ProcessStatus;

	const AudioPorts<settings>* const m_ap;
	bool m_silentOutput = false;
};

} // namespace detail


/**
 * Interface for an audio ports implementation.
 * Contains the `AudioPortsModel` and provides access to the audio buffers.
 *
 * Used by `AudioPlugin` to handle all the customizable aspects of the plugin's audio ports.
 */
template<AudioPortsSettings settings>
class AudioPorts
	: public AudioPortsModel
{
	static_assert(Validate<settings>{}());

public:
	explicit AudioPorts(bool isInstrument, Model* parent = nullptr)
		: AudioPortsModel{settings.inputs, settings.outputs, isInstrument, parent}
	{
	}

	/**
	 * AudioPorts buffer
	 *
	 * Interface for accessing input/output audio buffers.
	 * Implement this for custom AudioPorts buffers.
	 */
	using Buffer = detail::AudioPortsBuffer<settings>;

	/**
	 * AudioPorts router
	 *
	 * Performs pin connector routing for an audio processor.
	 */
	using Router = detail::AudioPortsRouter<settings>;

	template<AudioPortsSettings>
	friend class detail::AudioPortsRouter;

	/**
	 * Must be called after construction.
	 *
	 * NOTE: This cannot be called in the constructor due
	 *       to the use of a virtual method.
	 */
	void init()
	{
		if (!AudioPortsModel::initialized()) { return; }
		if (auto buffers = this->buffers())
		{
			buffers->updateBuffers(in().channelCount(), out().channelCount(),
				Engine::audioEngine()->framesPerPeriod());
		}
	}

	/**
	 * @returns whether `AudioPorts` is ready for use in the audio processor's process method.
	 *
	 * This requires `AudioPortsModel` to be initialized and the audio buffers to be available
	 * and initialized.
	 */
	auto active() const -> bool
	{
		if (!AudioPortsModel::initialized()) { return false; }
		if (auto buffers = constBuffers())
		{
			return buffers->initialized();
		}
		return false;
	}

	/**
	 * @returns the audio buffers if available, otherwise nullptr.
	 *
	 * If the buffers are available but not initialized, the input/output methods must not be called.
	 */
	virtual auto buffers() -> Buffer* = 0;

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

	auto getRouter() const -> Router
	{
		return Router{*this};
	}

	static constexpr auto audioPortsSettings() -> AudioPortsSettings { return settings; }
};


/**
 * Interface to help create a custom `AudioPorts` implementation.
 * `AudioPorts::Buffer` must be implemented in child class.
 */
template<AudioPortsSettings settings>
class CustomAudioPorts
	: public AudioPorts<settings>
	, protected AudioPorts<settings>::Buffer
{
public:
	using AudioPorts<settings>::AudioPorts;

protected:
	void bufferPropertiesChanging(proc_ch_t inChannels, proc_ch_t outChannels, f_cnt_t frames) override
	{
		// Connects `AudioPortsModel` to the buffers
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

// AudioPorts::Router out-of-class definitions

template<AudioPortsSettings settings>
inline void AudioPortsRouter<settings>::send(
	const AudioBus& in, PlanarBufferView<SampleT, settings.inputs> out) const
	requires (!settings.interleaved)
{
	assert(m_ap->in().channelCount() != DynamicChannelCount);
	if (m_ap->in().channelCount() == 0) { return; }

	// Ignore all unused track channels for better performance
	const auto inSizeConstrained = m_ap->trackChannelsUpperBound() / 2;
	assert(inSizeConstrained <= in.channelPairs());

	for (proc_ch_t outChannel = 0; outChannel < out.channels(); ++outChannel)
	{
		SampleT* outPtr = out.bufferPtr(outChannel);

		// Zero the output buffer
		std::fill_n(outPtr, out.frames(), SampleT{});

		for (std::uint8_t inChannelPairIdx = 0; inChannelPairIdx < inSizeConstrained; ++inChannelPairIdx)
		{
			const float* inPtr = in[inChannelPairIdx]; // track channel pair - 2-channel interleaved

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
						outPtr[frame] += convertSample<SampleT>(inPtr[frame * 2 + 1]);
					}
					break;
				}
				case 0b10: // L channel only
				{
					for (f_cnt_t frame = 0; frame < in.frames(); ++frame)
					{
						outPtr[frame] += convertSample<SampleT>(inPtr[frame * 2]);
					}
					break;
				}
				case 0b11: // Both channels
				{
					for (f_cnt_t frame = 0; frame < in.frames(); ++frame)
					{
						outPtr[frame] += convertSample<SampleT>(inPtr[frame * 2] + inPtr[frame * 2 + 1]);
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

template<AudioPortsSettings settings>
inline void AudioPortsRouter<settings>::receive(
	PlanarBufferView<const SampleT, settings.outputs> in, AudioBus& inOut) const
	requires (!settings.interleaved)
{
	assert(m_ap->out().channelCount() != DynamicChannelCount);
	if (m_ap->out().channelCount() == 0) { return; }

	// Ignore all unused track channels for better performance
	const auto inOutSizeConstrained = m_ap->trackChannelsUpperBound() / 2;
	assert(inOutSizeConstrained <= inOut.channelPairs());

	/*
	 * Routes processor audio to track channel pair and normalizes the result. For track channels
	 * without any processor audio routed to it, the track channel is unmodified for "passthrough"
	 * behavior.
	 */
	const auto routeNx2 = [&](float* outPtr, track_ch_t outChannel, auto usedTrackChannels) {
		constexpr std::uint8_t utc = usedTrackChannels();

		if constexpr (utc == 0b00)
		{
			// Both track channels pass through - nothing to do
			return;
		}

		const auto samples = inOut.frames() * 2;

		// We know at this point that we are writing to at least one of the track channels
		// rather than letting them pass through, so it is safe to set the output buffer of those
		// channels to zero prior to accumulation
		// TODO: This would benefit from keeping track of silent track channels

		if constexpr (utc == 0b11)
		{
			std::fill_n(outPtr, samples, 0.f);
		}
		else
		{
			for (f_cnt_t sampleIdx = 0; sampleIdx < samples; sampleIdx += 2)
			{
				if constexpr ((utc & 0b10) != 0)
				{
					outPtr[sampleIdx] = 0.f;
				}
				if constexpr ((utc & 0b01) != 0)
				{
					outPtr[sampleIdx + 1] = 0.f;
				}
			}
		}

		for (proc_ch_t inChannel = 0; inChannel < in.channels(); ++inChannel)
		{
			const SampleT* inPtr = in.bufferPtr(inChannel);

			if constexpr (utc == 0b11)
			{
				// This input channel could be routed to either left, right, both, or neither output channels
				if (m_ap->out().enabled(outChannel, inChannel))
				{
					if (m_ap->out().enabled(outChannel + 1, inChannel))
					{
						for (f_cnt_t sampleIdx = 0; sampleIdx < samples; sampleIdx += 2)
						{
							outPtr[sampleIdx]     += inPtr[sampleIdx / 2];
							outPtr[sampleIdx + 1] += inPtr[sampleIdx / 2];
						}
					}
					else
					{
						for (f_cnt_t sampleIdx = 0; sampleIdx < samples; sampleIdx += 2)
						{
							outPtr[sampleIdx] += inPtr[sampleIdx / 2];
						}
					}
				}
				else if (m_ap->out().enabled(outChannel + 1, inChannel))
				{
					for (f_cnt_t sampleIdx = 0; sampleIdx < samples; sampleIdx += 2)
					{
						outPtr[sampleIdx + 1] += inPtr[sampleIdx / 2];
					}
				}
			}
			else if constexpr (utc == 0b10)
			{
				// This input channel may or may not be routed to the left output channel
				if (!m_ap->out().enabled(outChannel, inChannel)) { continue; }

				for (f_cnt_t sampleIdx = 0; sampleIdx < samples; sampleIdx += 2)
				{
					outPtr[sampleIdx] += inPtr[sampleIdx / 2];
				}
			}
			else if constexpr (utc == 0b01)
			{
				// This input channel may or may not be routed to the right output channel
				if (!m_ap->out().enabled(outChannel + 1, inChannel)) { continue; }

				for (f_cnt_t sampleIdx = 0; sampleIdx < samples; sampleIdx += 2)
				{
					outPtr[sampleIdx + 1] += inPtr[sampleIdx / 2];
				}
			}
		}
	};


	for (std::uint8_t outChannelPairIdx = 0; outChannelPairIdx < inOutSizeConstrained; ++outChannelPairIdx)
	{
		float* outPtr = inOut[outChannelPairIdx]; // track channel pair - 2-channel interleaved
		const auto outChannel = static_cast<track_ch_t>(outChannelPairIdx * 2);

		const std::uint8_t usedTrackChannels =
			(static_cast<std::uint8_t>(m_ap->out().usedTrackChannels()[outChannel]) << 1u)
			| static_cast<std::uint8_t>(m_ap->out().usedTrackChannels()[outChannel + 1]);

		switch (usedTrackChannels)
		{
			case 0b00:
				// Both track channels pass through, so nothing is allowed to be written to output
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

template<AudioPortsSettings settings>
inline void AudioPortsRouter<settings>::send(
	const AudioBus& in, InterleavedBufferView<float, settings.inputs> out) const
	requires (settings.interleaved)
{
	assert(m_ap->in().channelCount() != DynamicChannelCount);
	if (m_ap->in().channelCount() == 0) { return; }
	assert(m_ap->in().channelCount() == 2); // Interleaved routing only allows exactly 0 or 2 channels

	// Ignore all unused track channels for better performance
	const auto inSizeConstrained = m_ap->trackChannelsUpperBound() / 2;
	assert(inSizeConstrained <= in.channelPairs());
	assert(out.data() != nullptr);

	// Zero the output buffer
	std::fill_n(out.data(), out.frames() * 2, 0.f);

	/*
	 * This is essentially a function template with specializations for each
	 * of the 16 total routing combinations of an input 2-channel interleaved buffer to an
	 * output 2-channel interleaved buffer. The purpose is to eliminate all branching within
	 * the inner for-loop in hopes of better performance.
	 */
	auto route2x2 = [samples = in.frames() * 2, outPtr = out.data()](const float* inPtr, auto enabledPins) {
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
		const float* inPtr = in[inChannelPairIdx]; // track channel pair - 2-channel interleaved

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
inline void AudioPortsRouter<settings>::receive(
	InterleavedBufferView<const float, settings.outputs> in, AudioBus& inOut) const
	requires (settings.interleaved)
{
	assert(m_ap->out().channelCount() != DynamicChannelCount);
	if (m_ap->out().channelCount() == 0) { return; }
	assert(m_ap->out().channelCount() == 2); // Interleaved routing only allows exactly 0 or 2 channels

	// Ignore all unused track channels for better performance
	const auto inOutSizeConstrained = m_ap->trackChannelsUpperBound() / 2;
	assert(inOutSizeConstrained <= inOut.channelPairs());
	assert(in.data() != nullptr);

	/*
	 * This is essentially a function template with specializations for each
	 * of the 16 total routing combinations of an input 2-channel interleaved buffer to an
	 * output 2-channel interleaved buffer. The purpose is to eliminate all branching within
	 * the inner for-loop in hopes of better performance.
	 */
	auto route2x2 = [samples = inOut.frames() * 2, inPtr = in.data()](float* outPtr, auto enabledPins) {
		constexpr auto epL =  static_cast<std::uint8_t>(enabledPins() >> 2); // for L out channel
		constexpr auto epR = static_cast<std::uint8_t>(enabledPins() & 0b0011); // for R out channel

		if constexpr (enabledPins() == 0) { return; }

		// We know at this point that we are writing to at least one of the track channels rather
		// than letting them pass through, so it is safe to overwrite the contents of the output buffer

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
		float* outPtr = inOut[outChannelPairIdx]; // track channel pair - 2-channel interleaved
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
inline auto AudioPortsRouter<settings>::processNormally(AudioBus& coreInOut,
	AudioPortsBuffer<settings>& processorBuffers, F&& processFunc) -> ProcessStatus
{
	ProcessStatus status;
	if constexpr (settings.inplace)
	{
		const auto processorInOut = processorBuffers.inputOutput();

		// Write core to processor input buffer
		if constexpr (settings.inputs != 0)
		{
			send(coreInOut, processorInOut);
		}

		// Process
		status = processFunc(processorInOut);

		// Write processor output buffer to core
		if constexpr (settings.outputs != 0)
		{
			receive(processorInOut, coreInOut);
		}
	}
	else
	{
		const auto processorIn = processorBuffers.input();
		const auto processorOut = processorBuffers.output();

		// Write core to processor input buffer
		if constexpr (settings.inputs != 0)
		{
			send(coreInOut, processorIn);
		}

		// Process
		status = processFunc(processorIn, processorOut);

		// Write processor output buffer to core
		if constexpr (settings.outputs != 0)
		{
			receive(processorOut, coreInOut);
		}
	}
	return status;
}

template<AudioPortsSettings settings>
template<class F>
inline auto AudioPortsRouter<settings>::processWithDirectRouting(InterleavedBufferView<float, 2> coreBuffer,
	AudioPortsBuffer<settings>& processorBuffers, F&& processFunc) -> ProcessStatus
{
	ProcessStatus status;
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
					std::memcpy(processorInOut.data(), coreBuffer.data(), coreBuffer.dataSizeBytes());
				}
			}

			// Process
			status = processFunc(processorInOut);

			// Write processor output buffer (if it has one) to core
			if constexpr (settings.outputs != 0)
			{
				if (m_ap->out().channelCount() != 0)
				{
					assert(processorInOut.data() != nullptr);
					std::memcpy(coreBuffer.data(), processorInOut.data(), coreBuffer.dataSizeBytes());
				}
			}
		}
		else
		{
			// Can avoid using processor's input/output buffers AND avoid calling routing methods
			status = processFunc(coreBuffer);
		}
	}
	else
	{
		if constexpr (settings.buffered)
		{
			// Can avoid calling routing methods, but must write to and read from processor's buffers

			const auto processorIn = processorBuffers.input();
			const auto processorOut = processorBuffers.output();

			// Write core to processor input buffer (if it has one)
			if constexpr (settings.inputs != 0)
			{
				if (m_ap->in().channelCount() != 0)
				{
					assert(processorIn.data() != nullptr);
					std::memcpy(processorIn.data(), coreBuffer.data(), coreBuffer.dataSizeBytes());
				}
			}

			// Process
			status = processFunc(processorIn, processorOut);

			// Write processor output buffer (if it has one) to core
			if constexpr (settings.outputs != 0)
			{
				if (m_ap->out().channelCount() != 0)
				{
					assert(processorOut.data() != nullptr);
					std::memcpy(coreBuffer.data(), processorOut.data(), coreBuffer.dataSizeBytes());
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
				// Probably not using in-place processing - the processor implementation may be written under the
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
						status = processFunc(coreBuffer, processorOut);

						// Write processor output buffer to core
						std::memcpy(coreBuffer.data(), processorOut.data(), coreBuffer.dataSizeBytes());
					}
					else
					{
						// Input-only processor - no buffer copy needed
						status = processFunc(coreBuffer, processorOut);
					}
				}
				else
				{
					// Output-only processor - no buffer copy needed
					status = processFunc(processorIn, coreBuffer);
				}
			}
			else
			{
				// Using in-place processing - the input and output buffers
				// are allowed to be the same buffer, so no buffer copy is needed.
				status = processFunc(coreBuffer, coreBuffer);
			}
		}
	}

	return status;
}

} // namespace detail

} // namespace lmms

#endif // LMMS_AUDIO_PORTS_H
