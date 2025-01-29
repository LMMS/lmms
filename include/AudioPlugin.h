/*
 * AudioPlugin.h - Interface for audio plugins which provides
 *                 pin connector support and compile-time customizations
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

#ifndef LMMS_AUDIO_PLUGIN_H
#define LMMS_AUDIO_PLUGIN_H

#include <type_traits>
#include "AudioData.h"
#include "AudioPluginConfig.h"
#include "Effect.h"
#include "Instrument.h"
#include "InstrumentTrack.h"
#include "PluginAudioPort.h"

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

class NotePlayHandle;

namespace detail
{

//! Provides the correct `processImpl` interface for instruments or effects to implement
template<class ParentT, typename BufferT, typename ConstBufferT, bool inplace, bool provideBuffers>
class AudioProcessingMethod;

//! Instrument specialization
template<typename BufferT, typename ConstBufferT>
class AudioProcessingMethod<Instrument, BufferT, ConstBufferT, false, true>
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
class AudioProcessingMethod<Instrument, BufferT, ConstBufferT, true, true>
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
class AudioProcessingMethod<Instrument, BufferT, ConstBufferT, inplace, false>
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
class AudioProcessingMethod<Effect, BufferT, ConstBufferT, false, true>
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
class AudioProcessingMethod<Effect, BufferT, ConstBufferT, true, true>
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
class AudioProcessingMethod<Effect, BufferT, ConstBufferT, inplace, false>
{
protected:
	/**
	 * The main audio processing method for Effects. Runs when plugin is not asleep.
	 * The implementation knows how to provide the working buffers.
	 * The implementation is expected to perform wet/dry mixing for the first 2 channels.
	 */
	virtual auto processImpl() -> ProcessStatus = 0;
};

//! Connects the core audio channels to the instrument or effect using the pin connector
template<class ParentT, AudioPluginConfig config, class AudioPortT>
class AudioPlugin
{
	static_assert(always_false_v<ParentT>, "ParentT must be either Instrument or Effect");
};

//! Instrument specialization
template<AudioPluginConfig config, class AudioPortT>
class AudioPlugin<Instrument, config, AudioPortT>
	: public Instrument
	, public AudioProcessingMethod<Instrument,
		typename AudioDataViewSelector<config.kind, config.interleaved, config.inputs, false>::type,
		typename AudioDataViewSelector<config.kind, config.interleaved, config.inputs, true>::type,
		config.inplace, AudioPortT::provideProcessBuffers()>
{
public:
	template<typename... AudioPortArgsT>
	AudioPlugin(const Plugin::Descriptor* desc, InstrumentTrack* parent = nullptr,
		const Plugin::Descriptor::SubPluginFeatures::Key* key = nullptr,
		Instrument::Flags flags = Instrument::Flag::NoFlags,
		AudioPortArgsT&&... audioPortArgs)
		: Instrument{desc, parent, key, flags}
		, m_audioPort{true, this, std::forward<AudioPortArgsT>(audioPortArgs)...}
	{
		m_audioPort.init();
	}

protected:
	auto audioPort() -> AudioPortT& { return m_audioPort; }

	auto pinConnector() const -> const PluginPinConnector* final
	{
		return m_audioPort.active()
			? &m_audioPort.pinConnector()
			: nullptr;
	}

	void playImpl(CoreAudioDataMut inOut) final
	{
		auto buffers = m_audioPort.buffers();
		if (!buffers)
		{
			// Plugin is not running
			return;
		}

		SampleFrame* temp = inOut.data();
		const auto bus = CoreAudioBusMut{&temp, 1, inOut.size()};
		auto router = m_audioPort.getRouter();

		if constexpr (config.inplace)
		{
			// Write core to plugin input buffer
			const auto pluginInOut = buffers->inputOutputBuffer();
			router.routeToPlugin(bus, pluginInOut);

			// Process
			if constexpr (AudioPortT::provideProcessBuffers()) { this->processImpl(pluginInOut); }
			else { this->processImpl(); }

			// Write plugin output buffer to core
			router.routeFromPlugin(pluginInOut, bus);
		}
		else
		{
			// Write core to plugin input buffer
			const auto pluginIn = buffers->inputBuffer();
			const auto pluginOut = buffers->outputBuffer();
			router.routeToPlugin(bus, pluginIn);

			// Process
			if constexpr (AudioPortT::provideProcessBuffers()) { this->processImpl(pluginIn, pluginOut); }
			else { this->processImpl(); }

			// Write plugin output buffer to core
			router.routeFromPlugin(pluginOut, bus);
		}
	}

	void playNoteImpl(NotePlayHandle* notesToPlay, CoreAudioDataMut inOut) final
	{
		/**
		 * NOTE: Only MIDI-based instruments are currently supported by AudioPlugin.
		 * NotePlayHandle-based instruments use buffers from their play handles, and more work
		 * would be needed to integrate that system with the AudioPluginBufferInterface system
		 * used by AudioPlugin. AudioPluginBufferInterface is also not thread-safe.
		 *
		 * The `Instrument::playNote()` method is still called for MIDI-based instruments when
		 * notes are played, so this method is a no-op.
		 */
	}

private:
	AudioPortT m_audioPort;
};

//! Effect specialization
template<AudioPluginConfig config, class AudioPortT>
class AudioPlugin<Effect, config, AudioPortT>
	: public Effect
	, public AudioProcessingMethod<Effect,
		typename AudioDataViewSelector<config.kind, config.interleaved, config.inputs, false>::type,
		typename AudioDataViewSelector<config.kind, config.interleaved, config.inputs, true>::type,
		config.inplace, AudioPortT::provideProcessBuffers()>
{
public:
	template<typename... AudioPortArgsT>
	AudioPlugin(const Plugin::Descriptor* desc, Model* parent = nullptr,
		const Plugin::Descriptor::SubPluginFeatures::Key* key = nullptr,
		AudioPortArgsT&&... audioPortArgs)
		: Effect{desc, parent, key}
		, m_audioPort{false, this, std::forward<AudioPortArgsT>(audioPortArgs)...}
	{
		m_audioPort.init();
	}

protected:
	auto audioPort() -> AudioPortT& { return m_audioPort; }

	auto pinConnector() const -> const PluginPinConnector* final
	{
		return m_audioPort.active()
			? &m_audioPort.pinConnector()
			: nullptr;
	}

	auto processAudioBufferImpl(CoreAudioDataMut inOut) -> bool final
	{
		if (isSleeping())
		{
			this->processBypassedImpl();
			return false;
		}

		auto buffers = m_audioPort.buffers();
		assert(buffers != nullptr);

		SampleFrame* temp = inOut.data();
		const auto bus = CoreAudioBusMut{&temp, 1, inOut.size()};
		auto router = m_audioPort.getRouter();

		ProcessStatus status;

		if constexpr (config.inplace)
		{
			// Write core to plugin input buffer
			const auto pluginInOut = buffers->inputOutputBuffer();
			router.routeToPlugin(bus, pluginInOut);

			// Process
			if constexpr (AudioPortT::provideProcessBuffers()) { status = this->processImpl(pluginInOut); }
			else { status = this->processImpl(); }

			// Write plugin output buffer to core
			router.routeFromPlugin(pluginInOut, bus);
		}
		else
		{
			// Write core to plugin input buffer
			const auto pluginIn = buffers->inputBuffer();
			const auto pluginOut = buffers->outputBuffer();
			router.routeToPlugin(bus, pluginIn);

			// Process
			if constexpr (AudioPortT::provideProcessBuffers()) { status = this->processImpl(pluginIn, pluginOut); }
			else { status = this->processImpl(); }

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

private:
	AudioPortT m_audioPort;
};


} // namespace detail


/**
 * AudioPlugin is the bridge connecting an Instrument/Effect base class used by the Core
 * with its derived class used by a plugin implementation.
 *
 * Pin connector routing and other common tasks are handled here to allow plugin implementations
 * to focus solely on audio processing or generation without needing to worry about how their plugin
 * interfaces with LMMS Core.
 *
 * This design allows for some compile-time customization over aspects of the plugin implementation
 * such as the number of in/out channels and whether samples are interleaved, so plugin developers can
 * implement their plugin in whatever way works best for them. All the mapping of their plugin to/from
 * LMMS Core is handled here, at compile-time where possible for best performance.
 *
 * A `processImpl` interface method is provided which must be implemented by the plugin implementation.
 *
 * @param ParentT Either `Instrument` or `Effect`
 * @param config Compile time configuration to customize `AudioPlugin`
 * @param AudioPortT The plugin's audio port - must fully implement `PluginAudioPort`
 */
template<class ParentT, AudioPluginConfig config, class AudioPortT = DefaultPluginAudioPort<config>>
class AudioPlugin
	: public detail::AudioPlugin<ParentT, config, AudioPortT>
{
	static_assert(config.kind != AudioDataKind::SampleFrame
		|| ((config.inputs == 0 || config.inputs == 2) && (config.outputs == 0 || config.outputs == 2)),
		"Don't use SampleFrame if more than 2 processor channels are needed");

	static_assert(std::is_base_of_v<detail::PluginAudioPortTag, AudioPortT>,
		"AudioPortT must be `PluginAudioPort` or inherit from it");

	using Base = typename detail::AudioPlugin<ParentT, config, AudioPortT>;

public:
	//! The last parameter(s) are variadic template parameters passed to the audio port constructor
	using Base::Base;

	static constexpr auto pluginConfig() -> AudioPluginConfig { return config; }
};


// NOTE: NotePlayHandle-based instruments are not supported yet
using DefaultMidiInstrument = AudioPlugin<Instrument, AudioPluginConfig {
	.kind = AudioDataKind::SampleFrame,
	.interleaved = true,
	.inputs = 0,
	.outputs = 2,
	.inplace = true }>;

using DefaultEffect = AudioPlugin<Effect, AudioPluginConfig {
	.kind = AudioDataKind::SampleFrame,
	.interleaved = true,
	.inputs = 2,
	.outputs = 2,
	.inplace = true }>;


} // namespace lmms

#endif // LMMS_AUDIO_PLUGIN_H
