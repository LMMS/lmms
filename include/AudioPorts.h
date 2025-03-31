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

#include "AudioEngine.h"
#include "AudioBuffer.h"
#include "AudioPortsConfig.h"
#include "AudioPortsModel.h"
#include "Engine.h"

namespace lmms
{


namespace detail {
struct AudioPortsTag {};
} // namespace detail


/**
 * Interface for an audio port implementation.
 * Contains the audio port model and provides access to the audio buffers.
 */
template<AudioPortsConfig config>
class AudioPorts
	: public AudioPortsModel
	, public detail::AudioPortsTag
{
public:
	AudioPorts(bool isInstrument, Model* parent)
		: AudioPortsModel{config.inputs, config.outputs, isInstrument, parent}
	{
	}

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

	auto model() const -> const AudioPortsModel&
	{
		return *static_cast<const AudioPortsModel*>(this);
	}

	auto model() -> AudioPortsModel&
	{
		return *static_cast<AudioPortsModel*>(this);
	}

	//! Returns the audio port router
	auto getRouter() const -> AudioPortsModel::Router<config>
	{
		return static_cast<const AudioPortsModel*>(this)->getRouter<config>();
	}

	//! Returns nullptr if the port is unavailable (i.e. Vestige with no plugin loaded)
	virtual auto buffers() -> AudioBuffer<config>* = 0;

	/**
	 * Returns true if the audio port can be used.
	 * Custom audio ports with an unusable state (i.e. a "plugin not loaded" state) should override this.
	 */
	virtual auto active() const -> bool { return true; }

	static constexpr auto configuration() -> AudioPortsConfig { return config; }
};


namespace detail {

/**
 * The default audio port for audio processors that do not provide their own.
 * Contains an audio port model and audio buffers.
 *
 * This audio port still has *some* ability for customization by using a custom `BufferT`,
 * but for full control, you'll need to provide your own audio port implementation.
 */
template<AudioPortsConfig config, template<AudioPortsConfig> class BufferT>
class DefaultAudioPorts
	: public AudioPorts<config>
	, public BufferT<config>
{
	static_assert(std::is_base_of_v<AudioBuffer<config>, BufferT<config>>,
		"BufferT must derive from AudioBuffer");

public:
	using AudioPorts<config>::AudioPorts;

	auto buffers() -> BufferT<config>* final { return static_cast<BufferT<config>*>(this); }

private:
	void bufferPropertiesChanged(proc_ch_t inChannels, proc_ch_t outChannels, f_cnt_t frames) final
	{
		// Connects the audio port model to the buffers
		this->updateBuffers(inChannels, outChannels, frames);
	}
};

} // namespace detail


//! Default audio port
template<AudioPortsConfig config>
using DefaultAudioPorts = detail::DefaultAudioPorts<config, DefaultAudioBuffer>;


//! Custom audio port - audio buffer interface to be implemented in child class
template<AudioPortsConfig config>
class CustomAudioPorts
	: public AudioPorts<config>
	, public AudioBuffer<config>
{
public:
	using AudioPorts<config>::AudioPorts;

private:
	void bufferPropertiesChanged(proc_ch_t inChannels, proc_ch_t outChannels, f_cnt_t frames) final
	{
		// Connects the audio port model to the buffers
		this->updateBuffers(inChannels, outChannels, frames);
	}
};


} // namespace lmms

#endif // LMMS_AUDIO_PORTS_H
