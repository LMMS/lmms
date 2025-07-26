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

namespace lmms
{

/**
 * A non-owning span of InterleavedBufferView<const float, 2>.
 *
 * Access like this:
 *   myAudioBus[channel pair index][sample index]
 *
 * where
 *   0 <= channel pair index < channelPairs()
 *   0 <= sample index < frames() * 2
 *
 * TODO C++23: Use std::mdspan
 */
template<typename T>
class AudioBus
{
public:
	static_assert(std::is_same_v<std::remove_const_t<T>, float>);

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

	auto trackChannelPair(track_ch_t pairIndex) const -> InterleavedBufferView<T, 2>
	{
		return {m_bus[pairIndex], m_frames};
	}

	//! @return 2-channel interleaved buffer for the given track channel pair
	auto operator[](track_ch_t channelPairIndex) const -> T* { return m_bus[channelPairIndex]; }

	auto bus() const -> T* const* { return m_bus; }
	auto channelPairs() const -> track_ch_t { return m_channelPairs; }
	auto frames() const -> f_cnt_t { return m_frames; }

private:
	T* const* m_bus = nullptr; //!< [channel pair index][sample index]
	const track_ch_t m_channelPairs = 0;
	const f_cnt_t m_frames = 0;
};

enum class ProcessStatus
{
	//! Unconditionally continue processing
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
template<AudioPortsSettings settings>
class AudioPortsRouter
{
	using SampleT = GetAudioDataType<settings.kind>;

	static_assert(!settings.interleaved || settings.sampleFrameCompatible(),
		"Interleaved audio port routers are currently limited to SampleFrame-compatible buffers only");

public:
	explicit AudioPortsRouter(const AudioPorts<settings>& parent, bool autoQuitEnabled)
		: m_ap{&parent}
		, m_autoQuitEnabled{autoQuitEnabled}
	{
	}

	/**
	 * Routes core audio to audio processor inputs, calls `processFunc`, then routes audio from
	 * processor outputs back to the core.
	 */
	template<class F>
	auto process(AudioBus<float> inOut, AudioPortsBuffer<settings>& processorBuffers, F&& processFunc)
		-> ProcessStatus
	{
		m_silentOutput = false;
		if constexpr (settings.sampleFrameCompatible())
		{
			if (const auto dr = m_ap->m_directRouting)
			{
				// The "direct routing" optimization can be applied
				return processWithDirectRouting(inOut.trackChannelPair(*dr),
					processorBuffers, std::forward<F>(processFunc));
			}
			else
			{
				return processNormally(inOut, processorBuffers, std::forward<F>(processFunc));
			}
		}
		else
		{
			return processNormally(inOut, processorBuffers, std::forward<F>(processFunc));
		}
	}

	/**
	 * Whether the processor output channels currently in use (processor channels that are routed to
	 * at least one track channel) are silent.
	 *
	 * Only available when `process()` returns `ProcessStatus::ContinueIfNotQuiet` and auto-quit is enabled.
	 * Otherwise the outputs are assumed to not be silent.
	 */
	auto silentOutput() const -> bool { return m_silentOutput; }

#ifdef LMMS_TESTING
	friend class ::AudioPortsTest;
#endif

private:
	void send(AudioBus<const float> in, PlanarBufferView<SampleT, settings.inputs> out) const
		requires (!settings.interleaved);
	void receive(PlanarBufferView<const SampleT, settings.outputs> in, AudioBus<float> inOut) const
		requires (!settings.interleaved);

	void send(AudioBus<const float> in, InterleavedBufferView<float, settings.inputs> out) const
		requires (settings.interleaved);
	void receive(InterleavedBufferView<const float, settings.outputs> in, AudioBus<float> inOut) const
		requires (settings.interleaved);

	auto isOutputSilent(InterleavedBufferView<const float, 2> output) const -> bool;

	auto isOutputSilent(PlanarBufferView<const SampleT, settings.outputs> output) const -> bool
		requires (!settings.interleaved);

	/**
	 * Processes audio normally (no "direct routing").
	 * Uses `send` and `receive` to route audio to and from the processor.
	 */
	template<class F>
	auto processNormally(AudioBus<float> coreInOut, AudioPortsBuffer<settings>& processorBuffers, F&& processFunc)
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
	const bool m_autoQuitEnabled;

	/*
	 * In the past, the RMS was calculated then compared with a threshold of 10^(-10).
	 * Now we use a different algorithm to determine whether a buffer is non-quiet, so
	 * a new threshold is needed for the best compatibility. The following is how it's derived.
	 *
	 * Old method:
	 * RMS = average (L^2 + R^2) across stereo buffer.
	 * RMS threshold = 10^(-10)
	 *
	 * So for a single channel, it would be:
	 * RMS/2 = average M^2 across single channel buffer.
	 * RMS/2 threshold = 5^(-11)
	 *
	 * The new algorithm for determining whether a buffer is non-silent compares M with the threshold,
	 * not M^2, so the square root of M^2's threshold should give us the most compatible threshold for
	 * the new algorithm:
	 *
	 * (RMS/2)^0.5 = (5^(-11))^0.5 = 0.0001431 (approx.)
	 *
	 * In practice though, the exact value shouldn't really matter so long as it's sufficiently small.
	 */
	static constexpr auto s_silenceThreshold = 0.0001431f;
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
	 * `rms`
	 *     The RMS is calculated from the in-use processor output channels (processor channels that are
	 *     routed to at least one track channel).
	 *     Only available when `process()` returns `ProcessStatus::ContinueIfNotQuiet` and auto-quit is enabled.
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

	template<AudioPortsSettings>
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

	auto getRouter(bool autoQuitEnabled = false) const -> Router
	{
		return Router{*this, autoQuitEnabled};
	}

	static constexpr auto audioPortsSettings() -> AudioPortsSettings { return settings; }
};


/**
 * Custom audio port.
 * AudioPorts::Buffer must be implemented in child class.
 */
template<AudioPortsSettings settings>
class CustomAudioPorts
	: public AudioPorts<settings>
	, protected AudioPorts<settings>::Buffer
{
public:
	using AudioPorts<settings>::AudioPorts;

protected:
	void bufferPropertiesChanged(proc_ch_t inChannels, proc_ch_t outChannels, f_cnt_t frames) override
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

// AudioPorts::Router out-of-class definitions

template<AudioPortsSettings settings>
inline void AudioPortsRouter<settings>::send(
	AudioBus<const float> in, PlanarBufferView<SampleT, settings.inputs> out) const
	requires (!settings.interleaved)
{
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
	PlanarBufferView<const SampleT, settings.outputs> in, AudioBus<float> inOut) const
	requires (!settings.interleaved)
{
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
	const auto routeNx2 = [&](float* outPtr, track_ch_t outChannel, auto usedTrackChannels) {
		constexpr std::uint8_t utc = usedTrackChannels();

		if constexpr (utc == 0b00)
		{
			// Both track channels bypassed - nothing to do
			return;
		}

		const auto samples = inOut.frames() * 2;

		// We know at this point that we are writing to at least one of the output channels
		// rather than bypassing, so it is safe to set the output buffer of those channels
		// to zero prior to accumulation

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
				(static_cast<std::uint8_t>(m_ap->m_usedTrackChannels[outChannel]) << 1u)
				| static_cast<std::uint8_t>(m_ap->m_usedTrackChannels[outChannel + 1]);

		switch (usedTrackChannels)
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

template<AudioPortsSettings settings>
inline void AudioPortsRouter<settings>::send(
	AudioBus<const float> in, InterleavedBufferView<float, settings.inputs> out) const
	requires (settings.interleaved)
{
	assert(m_ap->in().channelCount() != DynamicChannelCount);
	if (m_ap->in().channelCount() == 0) { return; }
	assert(m_ap->in().channelCount() == 2); // Interleaved routing only allows exactly 0 or 2 channels

	// Ignore all unused track channels for better performance
	const auto inSizeConstrained = m_ap->m_trackChannelsUpperBound / 2;
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
	InterleavedBufferView<const float, settings.outputs> in, AudioBus<float> inOut) const
	requires (settings.interleaved)
{
	assert(m_ap->out().channelCount() != DynamicChannelCount);
	if (m_ap->out().channelCount() == 0) { return; }
	assert(m_ap->out().channelCount() == 2); // Interleaved routing only allows exactly 0 or 2 channels

	// Ignore all unused track channels for better performance
	const auto inOutSizeConstrained = m_ap->m_trackChannelsUpperBound / 2;
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
inline auto AudioPortsRouter<settings>::isOutputSilent(InterleavedBufferView<const float, 2> output) const -> bool
{
	assert(m_ap->m_usedProcessorChannels.size() == 2);

	// TODO C++26: Use std::simd::all_of
	switch ((m_ap->m_usedProcessorChannels[0] << 1) | std::uint8_t{m_ap->m_usedProcessorChannels[1]})
	{
		case 0b11:
			return std::ranges::all_of(output.dataView(), [](const float sample) {
				return std::abs(sample) < s_silenceThreshold;
			});
		case 0b10:
			return std::ranges::all_of(output.framesView(), [](const float* frame) {
				return std::abs(frame[0]) < s_silenceThreshold;
			});
		case 0b01:
			return std::ranges::all_of(output.framesView(), [](const float* frame) {
				return std::abs(frame[1]) < s_silenceThreshold;
			});
		case 0b00:
			// No outputs are used
			return true;
		default:
			unreachable();
			return true;
	}
}

template<AudioPortsSettings settings>
inline auto AudioPortsRouter<settings>::isOutputSilent(
	PlanarBufferView<const SampleT, settings.outputs> output) const -> bool
	requires (!settings.interleaved)
{
	assert(output.channels() == m_ap->m_usedProcessorChannels.size());

	proc_ch_t pc = 0;
	for (bool used : m_ap->m_usedProcessorChannels)
	{
		if (used)
		{
			const bool silent = std::ranges::all_of(output.buffer(pc), [](const SampleT sample) {
				return std::abs(sample) < s_silenceThreshold;
			});
			if (!silent) { return false; }
		}
		++pc;
	}
	return true;
}

template<AudioPortsSettings settings>
template<class F>
inline auto AudioPortsRouter<settings>::processNormally(AudioBus<float> coreInOut,
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
			if (status == ProcessStatus::ContinueIfNotQuiet && m_autoQuitEnabled)
			{
				m_silentOutput = isOutputSilent(processorInOut);
			}
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
			if (status == ProcessStatus::ContinueIfNotQuiet && m_autoQuitEnabled)
			{
				m_silentOutput = isOutputSilent(processorOut);
			}
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

	// Check if output buffers are silent (if needed)
	if constexpr (settings.outputs != 0)
	{
		if (status == ProcessStatus::ContinueIfNotQuiet
			&& m_autoQuitEnabled
			&& m_ap->out().channelCount() != 0)
		{
			// Silent output is determined using the processor output channels that are currently in use.
			// When "direct routing" is active (as it is here), the data on the track channels
			// is the same as the data on the in-use processor output channels, making this easy
			// to calculate.

			// TODO C++26: Use std::simd::all_of
			m_silentOutput = std::ranges::all_of(coreBuffer.dataView(), [](float sample) {
				return std::abs(sample) < s_silenceThreshold;
			});
		}
	}

	return status;
}

} // namespace detail

} // namespace lmms

#endif // LMMS_AUDIO_PORTS_H
