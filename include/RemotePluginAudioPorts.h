/*
 * RemotePluginAudioPorts.h - AudioPorts implementation for RemotePlugin
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

#ifndef LMMS_REMOTE_PLUGIN_AUDIO_PORTS_H
#define LMMS_REMOTE_PLUGIN_AUDIO_PORTS_H

#include <optional>

#include "AudioPorts.h"
#include "RemotePlugin.h"
#include "lmms_export.h"

namespace lmms {

/*
 * TODO: A better design would just have `RemotePluginAudioPorts<config>` passed to
 *       RemotePlugin from the plugin implementation rather than
 *       RemotePluginAudioPortsController, and RemotePlugin would be a class
 *       template with an `AudioPortsConfig` template parameter.
 *       There would also be RemotePluginAudioBuffer, a custom buffer implementation
 *       for remote plugins and remote plugin clients which RemotePlugin would
 *       derive from. However, this design requires C++20's class type NTTP on the
 *       remote plugin's client side but we're compiling that with C++17 due to a
 *       weird regression.
 */

class LMMS_EXPORT RemotePluginAudioPortsController
{
public:
	RemotePluginAudioPortsController(AudioPortsModel& model)
		: m_model{&model}
	{
	}

	//! Connects RemotePlugin's buffers to audio port; Call after buffers are created
	void connectBuffers(RemotePlugin* buffers)
	{
		m_buffers = buffers;
	}

	//! Disconnects RemotePlugin's buffers from audio port; Call before buffers are destroyed
	void disconnectBuffers()
	{
		m_buffers = nullptr;
	}

	//! Call after the RemotePlugin is fully initialized
	virtual void activate(f_cnt_t frames) = 0;

	auto audioPortsModel() -> AudioPortsModel&
	{
		return *m_model;
	}

protected:
	RemotePlugin* m_buffers = nullptr;
	AudioPortsModel* m_model = nullptr;

	fpp_t m_frames = 0;
};


//! AudioPorts implementation for RemotePlugin
template<AudioPortsConfig config>
class RemotePluginAudioPorts
	: public CustomAudioPorts<config>
	, public RemotePluginAudioPortsController
{
	using SampleT = GetAudioDataType<config.kind>;

public:
	RemotePluginAudioPorts(bool isInstrument, Model* parent)
		: CustomAudioPorts<config>{isInstrument, parent}
		, RemotePluginAudioPortsController{*static_cast<AudioPortsModel*>(this)}
	{
	}

	static_assert(config.kind == AudioDataKind::F32, "RemotePlugin only supports float");
	static_assert(!config.interleaved, "RemotePlugin only supports non-interleaved");
	static_assert(!config.inplace, "RemotePlugin does not support in-place processing");
	static_assert(config.buffered, "RemotePlugin must be buffered");

	auto controller() -> RemotePluginAudioPortsController&
	{
		return *static_cast<RemotePluginAudioPortsController*>(this);
	}

	void activate(f_cnt_t frames) override
	{
		assert(m_buffers != nullptr);
		updateBuffers(this->in().channelCount(), this->out().channelCount(), frames);

		m_remoteActive = true;
	}

	/*
	 * `AudioPorts` implementation
	 */

	//! Only returns the buffer interface if audio port is active
	auto buffers() -> AudioBuffer<config>* override
	{
		return remoteActive() ? this : nullptr;
	}

	/*
	 * `AudioBuffer` implementation
	 */

	auto inputBuffer() -> SplitAudioData<SampleT, config.inputs> override
	{
		if (!remoteActive()) { return SplitAudioData<SampleT, config.inputs>{}; }

		return SplitAudioData<SampleT, config.inputs> {
			m_insOuts,
			static_cast<pi_ch_t>(this->in().channelCount()),
			m_frames
		};
	}

	auto outputBuffer() -> SplitAudioData<SampleT, config.outputs> override
	{
		if (!remoteActive()) { return SplitAudioData<SampleT, config.outputs>{}; }

		return SplitAudioData<SampleT, config.outputs> {
			m_insOuts + this->in().channelCount(),
			static_cast<pi_ch_t>(this->out().channelCount()),
			m_frames
		};
	}

	auto frames() const -> fpp_t override
	{
		return remoteActive() ? m_frames : 0;
	}

	void updateBuffers(int channelsIn, int channelsOut, f_cnt_t frames) override
	{
		// Update the shared memory audio buffer in RemotePlugin
		if (!m_buffers) { return; }

		SampleT* ptr = m_buffers->updateAudioBuffer(channelsIn, channelsOut, frames);
		if (!ptr)
		{
			// Error occurred
			m_insOuts = nullptr;
			return;
		}

		// Update our views into the RemotePlugin buffer
		const auto channels = static_cast<std::size_t>(channelsIn + channelsOut);
		if constexpr (!config.staticChannelCount())
		{
			m_accessBuffer.resize(channels);
		}
		else
		{
			// If channel counts are known at compile time, they should never change
			assert(channelsIn == config.inputs);
			assert(channelsOut == config.outputs);
		}

		m_frames = frames;

		for (std::size_t channel = 0; channel < channels; ++channel)
		{
			m_accessBuffer[channel] = ptr;
			ptr += frames;
		}

		m_insOuts = m_accessBuffer.data();
	}

	auto active() const -> bool override { return remoteActive(); }

private:
	auto remoteActive() const -> bool { return m_buffers != nullptr && m_remoteActive; }

	// Views into RemotePlugin's shared memory buffer
	detail::AccessBufferType<config> m_accessBuffer;
	SampleT** m_insOuts = nullptr;

protected:
	bool m_remoteActive = false;
};


//! An audio port that can choose between RemotePlugin or a local buffer at runtime
template<AudioPortsConfig config, template<AudioPortsConfig> class LocalBufferT>
class ConfigurableAudioPorts
	: public RemotePluginAudioPorts<config>
{
	using SampleT = GetAudioDataType<config.kind>;

public:
	ConfigurableAudioPorts(bool isInstrument, Model* parent, bool beginAsRemote = true)
		: RemotePluginAudioPorts<config>{isInstrument, parent}
	{
		setBufferType(beginAsRemote);
	}

	//! Call this to switch to a different buffer type, then call `activate`
	void setBufferType(bool remote = true)
	{
		// Deactivate both buffers until activation
		m_localActive = false;
		RemotePluginAudioPorts<config>::m_remoteActive = false;

		m_isRemote = remote;
	}

	auto isRemote() const -> bool { return m_isRemote; }

	//! Activates the audio port after switching buffer types
	void activate(f_cnt_t frames) override
	{
		if (isRemote()) { RemotePluginAudioPorts<config>::activate(frames); return; }

		updateBuffers(this->in().channelCount(), this->out().channelCount(), frames);

		m_localActive = true;
	}

	auto buffers() -> AudioBuffer<config>* override
	{
		if (isRemote()) { return RemotePluginAudioPorts<config>::buffers(); }
		return localActive() ? &m_localBuffer.value() : nullptr;
	}

	auto inputBuffer() -> SplitAudioData<SampleT, config.inputs> override
	{
		if (isRemote()) { return RemotePluginAudioPorts<config>::inputBuffer(); }
		return localActive()
			? m_localBuffer->inputBuffer()
			: SplitAudioData<SampleT, config.inputs>{};
	}

	auto outputBuffer() -> SplitAudioData<SampleT, config.outputs> override
	{
		if (isRemote()) { return RemotePluginAudioPorts<config>::outputBuffer(); }
		return localActive()
			? m_localBuffer->outputBuffer()
			: SplitAudioData<SampleT, config.outputs>{};
	}

	auto frames() const -> fpp_t override
	{
		if (isRemote()) { return RemotePluginAudioPorts<config>::frames(); }
		return localActive() ? m_localBuffer->frames() : 0;
	}

	void updateBuffers(int channelsIn, int channelsOut, f_cnt_t frames) override
	{
		if (isRemote())
		{
			RemotePluginAudioPorts<config>::updateBuffers(channelsIn, channelsOut, frames);
		}
		else
		{
			if (!m_localBuffer) { m_localBuffer.emplace(); }
			m_localBuffer->updateBuffers(channelsIn, channelsOut, frames);
		}
	}

	auto active() const -> bool override
	{
		return isRemote()
			? RemotePluginAudioPorts<config>::active()
			: localActive();
	}

private:
	auto localActive() const -> bool { return m_localBuffer.has_value() && m_localActive; }

	std::optional<LocalBufferT<config>> m_localBuffer;
	bool m_localActive = false;
	bool m_isRemote = true;
};


template<AudioPortsConfig config>
using DefaultConfigurableAudioPorts = ConfigurableAudioPorts<config, DefaultAudioBuffer>;


} // namespace lmms

#endif // LMMS_REMOTE_PLUGIN_AUDIO_PORTS_H
