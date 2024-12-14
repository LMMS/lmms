/*
 * AudioPluginInterface.h - Interface for all audio plugins
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

#ifndef LMMS_AUDIO_PLUGIN_INTERFACE_H
#define LMMS_AUDIO_PLUGIN_INTERFACE_H

#include "AudioData.h"
#include "AudioPluginBuffer.h"
#include "Effect.h"
#include "Instrument.h"
#include "InstrumentTrack.h"
#include "PluginPinConnector.h"
#include "SampleFrame.h"

namespace lmms
{

enum class ProcessStatus
{
	//! Unconditionally continue processing
	Continue,

	//! Calculate the RMS out sum and call `checkGate` to determine whether to stop processing
	ContinueIfNotQuiet,

	//! Do not continue processing
	Sleep
};

//! Compile time customizations for `AudioPluginInterface` to meet the needs of a plugin implementation
struct PluginConfig
{
	//! The audio data layout used by the plugin
	AudioDataLayout layout;

	//! The number of plugin input channels, or `DynamicChannelCount` if unknown at compile time
	int inputs = DynamicChannelCount;

	//! The number of plugin output channels, or `DynamicChannelCount` if unknown at compile time
	int outputs = DynamicChannelCount;

	//! In-place processing - true (always in-place) or false (dynamic, customizable in audio buffer impl)
	bool inplace = false;

	//! If true, plugin implementation will provide an `AudioPluginBufferInterfaceProvider`
	bool customBuffer = false;
};

class NotePlayHandle;

namespace detail
{

//! Provides the correct `processImpl` interface for instruments or effects to implement
template<class ChildT, typename BufferT, typename ConstBufferT, bool inplace, bool customBuffer>
class AudioProcessingMethod;

//! Instrument specialization
template<typename BufferT, typename ConstBufferT>
class AudioProcessingMethod<Instrument, BufferT, ConstBufferT, false, false>
{
protected:
	//! The main audio processing method for NotePlayHandle-based Instruments
	//! NOTE: NotePlayHandle-based instruments are currently unsupported
	//virtual void processImpl(NotePlayHandle* nph, ConstBufferT in, BufferT out) {}

	//! The main audio processing method for MIDI-based Instruments
	virtual void processImpl(ConstBufferT in, BufferT out) = 0;
};

//! Instrument specialization (in-place)
template<typename BufferT, typename ConstBufferT>
class AudioProcessingMethod<Instrument, BufferT, ConstBufferT, true, false>
{
protected:
	//! The main audio processing method for NotePlayHandle-based Instruments
	//! NOTE: NotePlayHandle-based instruments are currently unsupported
	//virtual void processImpl(NotePlayHandle* nph, BufferT inOut) {}

	//! The main audio processing method for MIDI-based Instruments
	virtual void processImpl(BufferT inOut) = 0;
};

//! Instrument specialization (custom working buffers)
template<typename BufferT, typename ConstBufferT, bool inplace>
class AudioProcessingMethod<Instrument, BufferT, ConstBufferT, inplace, true>
{
protected:
	/**
	 * The main audio processing method for NotePlayHandle-based Instruments.
	 * The implementation knows how to provide the working buffers.
	 * NOTE: NotePlayHandle-based instruments are currently unsupported
	 */
	//virtual void processImpl(NotePlayHandle* nph) {}

	/**
	 * The main audio processing method for MIDI-based Instruments.
	 * The implementation knows how to provide the working buffers.
	 */
	virtual void processImpl() = 0;
};

//! Effect specialization
template<typename BufferT, typename ConstBufferT>
class AudioProcessingMethod<Effect, BufferT, ConstBufferT, false, false>
{
protected:
	/**
	 * The main audio processing method for Effects. Runs when plugin is not asleep.
	 * The implementation is expected to perform wet/dry mixing for the first 2 channels.
	 */
	virtual auto processImpl(ConstBufferT in, BufferT out) -> ProcessStatus = 0;
};

//! Effect specialization (in-place)
template<typename BufferT, typename ConstBufferT>
class AudioProcessingMethod<Effect, BufferT, ConstBufferT, true, false>
{
protected:
	/**
	 * The main audio processing method for inplace Effects. Runs when plugin is not asleep.
	 * The implementation is expected to perform wet/dry mixing for the first 2 channels.
	 */
	virtual auto processImpl(BufferT inOut) -> ProcessStatus = 0;
};

//! Effect specialization (custom working buffers)
template<typename BufferT, typename ConstBufferT, bool inplace>
class AudioProcessingMethod<Effect, BufferT, ConstBufferT, inplace, true>
{
protected:
	/**
	 * The main audio processing method for Effects. Runs when plugin is not asleep.
	 * The implementation knows how to provide the working buffers.
	 * The implementation is expected to perform wet/dry mixing for the first 2 channels.
	 */
	virtual auto processImpl() -> ProcessStatus = 0;
};

//! Connects the core audio buses to the instrument or effect using the pin connector
template<class ChildT, typename SampleT, PluginConfig config>
class AudioProcessorImpl
{
	static_assert(always_false_v<ChildT>, "ChildT must be either Instrument or Effect");
};

//! Instrument specialization
template<typename SampleT, PluginConfig config>
class AudioProcessorImpl<Instrument, SampleT, config>
	: public Instrument
	, public AudioProcessingMethod<Instrument,
		typename AudioDataTypeSelector<config.layout, SampleT, config.inputs>::type,
		typename AudioDataTypeSelector<config.layout, const SampleT, config.outputs>::type,
		config.inplace, config.customBuffer>
	, public std::conditional_t<config.customBuffer,
		AudioPluginBufferInterfaceProvider<config.layout, SampleT, config.inputs, config.outputs>,
		AudioPluginBufferDefaultImpl<config.layout, SampleT, config.inputs, config.outputs, config.inplace>>
{
public:
	AudioProcessorImpl(const Plugin::Descriptor* desc, InstrumentTrack* parent = nullptr, const Plugin::Descriptor::SubPluginFeatures::Key* key = nullptr, Instrument::Flags flags = Instrument::Flag::NoFlags)
		: Instrument{desc, parent, key, flags}
		, m_pinConnector{config.inputs, config.outputs, this}
	{
		connect(Engine::audioEngine(), &AudioEngine::sampleRateChanged, [this]() {
			auto iface = this->bufferInterface();
			if (!iface) { return; }
			iface->updateBuffers(
				m_pinConnector.in().channelCount(),
				m_pinConnector.out().channelCount()
			);
		});
	}

	auto pinConnector() const -> const PluginPinConnector* final { return &m_pinConnector; }

protected:
	void playImpl(CoreAudioDataMut inOut) final
	{
		SampleFrame* temp = inOut.data();
		const auto bus = CoreAudioBusMut{&temp, 1, Engine::audioEngine()->framesPerPeriod()};
		auto bufferInterface = this->bufferInterface();
		if (!bufferInterface)
		{
			// Plugin is not running
			return;
		}

		auto router = m_pinConnector.getRouter<config.layout, SampleT, config.inputs, config.outputs>();

		if constexpr (config.inplace)
		{
			// Write core to plugin input buffer
			const auto pluginInOut = bufferInterface->inputBuffer();
			router.routeToPlugin(bus, pluginInOut);

			// Process
			if constexpr (config.customBuffer) { this->processImpl(); }
			else { this->processImpl(pluginInOut); }

			// Write plugin output buffer to core
			router.routeFromPlugin(pluginInOut, bus);
		}
		else
		{
			// Write core to plugin input buffer
			const auto pluginIn = bufferInterface->inputBuffer();
			const auto pluginOut = bufferInterface->outputBuffer();
			router.routeToPlugin(bus, pluginIn);

			// Process
			if constexpr (config.customBuffer) { this->processImpl(); }
			else { this->processImpl(pluginIn, pluginOut); }

			// Write plugin output buffer to core
			router.routeFromPlugin(pluginOut, bus);
		}
	}

	void playNoteImpl(NotePlayHandle* notesToPlay, CoreAudioDataMut inOut) final
	{
		/**
		 * NOTE: Only MIDI-based instruments are currently supported by AudioPluginInterface.
		 * NotePlayHandle-based instruments use buffers from their play handles, and more work
		 * would be needed to integrate that system with the AudioPluginBufferInterface system
		 * used by AudioPluginInterface. AudioPluginBufferInterface is also not thread-safe.
		 *
		 * The `Instrument::playNote()` method is still called for MIDI-based instruments when
		 * notes are played, so this method is a no-op.
		 */
	}

	auto pinConnector() -> PluginPinConnector* { return &m_pinConnector; }

private:
	PluginPinConnector m_pinConnector;
};

//! Effect specialization
template<typename SampleT, PluginConfig config>
class AudioProcessorImpl<Effect, SampleT, config>
	: public Effect
	, public AudioProcessingMethod<Effect,
		typename AudioDataTypeSelector<config.layout, SampleT, config.inputs>::type,
		typename AudioDataTypeSelector<config.layout, const SampleT, config.outputs>::type,
		config.inplace, config.customBuffer>
	, public std::conditional_t<config.customBuffer,
		AudioPluginBufferInterfaceProvider<config.layout, SampleT, config.inputs, config.outputs>,
		AudioPluginBufferDefaultImpl<config.layout, SampleT, config.inputs, config.outputs, config.inplace>>
{
public:
	AudioProcessorImpl(const Plugin::Descriptor* desc, Model* parent = nullptr, const Plugin::Descriptor::SubPluginFeatures::Key* key = nullptr)
		: Effect{desc, parent, key}
		, m_pinConnector{config.inputs, config.outputs, this}
	{
		connect(Engine::audioEngine(), &AudioEngine::sampleRateChanged, [this]() {
			auto iface = this->bufferInterface();
			if (!iface) { return; }
			iface->updateBuffers(
				m_pinConnector.in().channelCount(),
				m_pinConnector.out().channelCount()
			);
		});
	}

	auto pinConnector() const -> const PluginPinConnector* final { return &m_pinConnector; }

protected:
	auto processAudioBufferImpl(CoreAudioDataMut inOut) -> bool final
	{
		if (isSleeping())
		{
			this->processBypassedImpl();
			return false;
		}

		SampleFrame* temp = inOut.data();
		const auto bus = CoreAudioBusMut{&temp, 1, inOut.size()};
		auto bufferInterface = this->bufferInterface();
		auto router = m_pinConnector.getRouter<config.layout, SampleT, config.inputs, config.outputs>();

		ProcessStatus status;

		if constexpr (config.inplace)
		{
			// Write core to plugin input buffer
			const auto pluginInOut = bufferInterface->inputBuffer();
			router.routeToPlugin(bus, pluginInOut);

			// Process
			if constexpr (config.customBuffer) { status = this->processImpl(); }
			else { status = this->processImpl(pluginInOut); }

			// Write plugin output buffer to core
			router.routeFromPlugin(pluginInOut, bus);
		}
		else
		{
			// Write core to plugin input buffer
			const auto pluginIn = bufferInterface->inputBuffer();
			const auto pluginOut = bufferInterface->outputBuffer();
			router.routeToPlugin(bus, pluginIn);

			// Process
			if constexpr (config.customBuffer) { status = this->processImpl(); }
			else { status = this->processImpl(pluginIn, pluginOut); }

			// Write plugin output buffer to core
			router.routeFromPlugin(pluginOut, bus);
		}

		switch (status)
		{
			case ProcessStatus::Continue:
				break;
			case ProcessStatus::ContinueIfNotQuiet:
			{
				double outSum = 0.0;
				for (const SampleFrame& frame : inOut)
				{
					outSum += frame.sumOfSquaredAmplitudes();
				}

				checkGate(outSum / inOut.size());
				break;
			}
			case ProcessStatus::Sleep:
				return false;
			default:
				break;
		}

		return isRunning();
	}

	/**
	 * Optional method that runs when an effect is asleep (not enabled,
	 * not running, not in the Okay state, or in the Don't Run state)
	 */
	virtual void processBypassedImpl()
	{
	}

	auto pinConnector() -> PluginPinConnector* { return &m_pinConnector; }

private:
	PluginPinConnector m_pinConnector;
};


} // namespace detail


/**
 * AudioPluginInterface is the bridge connecting an Instrument/Effect base class used by the Core
 * with its derived class used by a plugin implementation.
 *
 * Pin connector routing and other common tasks are handled here to allow plugin implementations
 * to focus solely on audio processing or generation without needing to worry about how their plugin
 * interfaces with LMMS Core.
 *
 * This design allows for some compile-time customization over aspects of the plugin implementation
 * such as the number of in/out channels and the audio data layout, so plugin developers can
 * implement their plugin in whatever way works best for them. All the mapping from their plugin to/from
 * LMMS Core is handled here, at compile-time where possible for best performance.
 *
 * A `processImpl` interface method is provided which must be implemented by the plugin implementation.
 *
 * @param ChildT Either `Instrument` or `Effect`
 * @param SampleT The plugin's sample type - i.e. float, double, int32_t, `SampleFrame`, etc.
 * @param config Compile time configuration to customize `AudioPluginInterface`
 */
template<class ChildT, typename SampleT, PluginConfig config>
class AudioPluginInterface
	: public detail::AudioProcessorImpl<ChildT, SampleT, config>
{
	static_assert(!std::is_const_v<SampleT>);
	static_assert(!std::is_same_v<SampleT, SampleFrame>
		|| ((config.inputs == 0 || config.inputs == 2) && (config.outputs == 0 || config.outputs == 2)),
		"Don't use SampleFrame if more than 2 processor channels are needed");

	using Base = detail::AudioProcessorImpl<ChildT, SampleT, config>;

public:
	using Base::AudioProcessorImpl;
};

// NOTE: NotePlayHandle-based instruments are not supported yet
using DefaultMidiInstrumentPluginInterface = AudioPluginInterface<Instrument, SampleFrame,
	PluginConfig{ .layout = AudioDataLayout::Interleaved, .inputs = 0, .outputs = 2, .inplace = true }>;

using DefaultEffectPluginInterface = AudioPluginInterface<Effect, SampleFrame,
	PluginConfig{ .layout = AudioDataLayout::Interleaved, .inputs = 2, .outputs = 2, .inplace = true }>;


} // namespace lmms

#endif // LMMS_AUDIO_PLUGIN_INTERFACE_H
