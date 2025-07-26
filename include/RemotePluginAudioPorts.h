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

#include "PluginAudioPorts.h"
#include "RemotePlugin.h"
#include "lmms_export.h"

namespace lmms {

/*
 * TODO: A better design would just have `RemotePluginAudioPorts<settings>` passed to
 *       RemotePlugin from the plugin implementation rather than
 *       RemotePluginAudioPortsController, and RemotePlugin would be a class
 *       template with an `AudioPortsSettings` template parameter.
 *       There would also be RemotePluginAudioPortsBuffer, a custom buffer implementation
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
template<AudioPortsSettings settings>
class RemotePluginAudioPorts
	: public CustomAudioPorts<settings>
	, public RemotePluginAudioPortsController
{
	using SampleT = GetAudioDataType<settings.kind>;

public:
	RemotePluginAudioPorts(bool isInstrument, Model* parent)
		: CustomAudioPorts<settings>{isInstrument, parent}
		, RemotePluginAudioPortsController{*static_cast<AudioPortsModel*>(this)}
	{
	}

	static_assert(settings.kind == AudioDataKind::F32, "RemotePlugin only supports float");
	static_assert(!settings.interleaved, "RemotePlugin only supports non-interleaved");
	static_assert(!settings.inplace, "RemotePlugin does not support in-place processing");
	static_assert(settings.buffered, "RemotePlugin must be buffered");

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

	auto active() const -> bool override { return remoteActive(); }

	//! Only returns the buffer interface if it's available
	auto buffers() -> AudioPorts<settings>::Buffer* override
	{
		return remoteActive() ? this : nullptr;
	}

protected:
	/*
	 * `AudioPorts::Buffer` implementation
	 */

	auto input() -> PlanarBufferView<SampleT, settings.inputs> override
	{
		if (!remoteActive()) { return {}; }

		return {m_insOuts, this->in().channelCount(), m_frames};
	}

	auto output() -> PlanarBufferView<SampleT, settings.outputs> override
	{
		if (!remoteActive()) { return {}; }

		return {m_insOuts + this->in().channelCount(), this->out().channelCount(), m_frames};
	}

	auto frames() const -> fpp_t override
	{
		return remoteActive() ? m_frames : 0;
	}

	void updateBuffers(proc_ch_t channelsIn, proc_ch_t channelsOut, f_cnt_t frames) override
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
		const auto channels = channelsIn + channelsOut;
		if constexpr (!settings.staticChannelCount())
		{
			m_accessBuffer.resize(channels);
		}
		else
		{
			// If channel counts are known at compile time, they should never change
			assert(channelsIn == settings.inputs);
			assert(channelsOut == settings.outputs);
		}

		m_frames = frames;

		for (proc_ch_t channel = 0; channel < channels; ++channel)
		{
			m_accessBuffer[channel] = ptr;
			ptr += frames;
		}

		m_insOuts = m_accessBuffer.data();
	}

private:
	auto remoteActive() const -> bool { return m_buffers != nullptr && m_remoteActive; }

	// Views into RemotePlugin's shared memory buffer
	detail::AccessBufferType<settings> m_accessBuffer;
	SampleT** m_insOuts = nullptr;

protected:
	bool m_remoteActive = false;
};


//! An audio port that can choose between RemotePlugin or a local buffer at runtime
template<AudioPortsSettings settings, template<AudioPortsSettings> class LocalBufferT>
class ConfigurableAudioPorts
	: public RemotePluginAudioPorts<settings>
{
	using SampleT = GetAudioDataType<settings.kind>;

public:
	ConfigurableAudioPorts(bool isInstrument, Model* parent, bool beginAsRemote = true)
		: RemotePluginAudioPorts<settings>{isInstrument, parent}
	{
		setBufferType(beginAsRemote);
	}

	//! Call this to switch to a different buffer type, then call `activate`
	void setBufferType(bool remote = true)
	{
		// Deactivate both buffers until activation
		m_localActive = false;
		RemotePluginAudioPorts<settings>::m_remoteActive = false;

		m_isRemote = remote;
	}

	auto isRemote() const -> bool { return m_isRemote; }

	//! Activates the audio port after switching buffer types
	void activate(f_cnt_t frames) override
	{
		if (isRemote()) { RemotePluginAudioPorts<settings>::activate(frames); return; }

		updateBuffers(this->in().channelCount(), this->out().channelCount(), frames);

		m_localActive = true;
	}

	/*
	 * `AudioPorts` implementation
	 */

	auto active() const -> bool override
	{
		return isRemote()
			? RemotePluginAudioPorts<settings>::active()
			: localActive();
	}

	auto buffers() -> AudioPorts<settings>::Buffer* override
	{
		if (isRemote()) { return RemotePluginAudioPorts<settings>::buffers(); }
		return localActive() ? &m_localBuffer.value() : nullptr;
	}

protected:
	/*
	 * `AudioPorts::Buffer` implementation
	 */

	auto input() -> PlanarBufferView<SampleT, settings.inputs> override
	{
		if (isRemote()) { return RemotePluginAudioPorts<settings>::input(); }
		return localActive()
			? m_localBuffer->input()
			: PlanarBufferView<SampleT, settings.inputs>{};
	}

	auto output() -> PlanarBufferView<SampleT, settings.outputs> override
	{
		if (isRemote()) { return RemotePluginAudioPorts<settings>::output(); }
		return localActive()
			? m_localBuffer->output()
			: PlanarBufferView<SampleT, settings.outputs>{};
	}

	auto frames() const -> fpp_t override
	{
		if (isRemote()) { return RemotePluginAudioPorts<settings>::frames(); }
		return localActive() ? m_localBuffer->frames() : 0;
	}

	void updateBuffers(proc_ch_t channelsIn, proc_ch_t channelsOut, f_cnt_t frames) override
	{
		if (isRemote())
		{
			RemotePluginAudioPorts<settings>::updateBuffers(channelsIn, channelsOut, frames);
		}
		else
		{
			if (!m_localBuffer) { m_localBuffer.emplace(); }
			m_localBuffer->updateBuffers(channelsIn, channelsOut, frames);
		}
	}

private:
	auto localActive() const -> bool { return m_localBuffer.has_value() && m_localActive; }

	std::optional<LocalBufferT<settings>> m_localBuffer;
	bool m_localActive = false;
	bool m_isRemote = true;
};


template<AudioPortsSettings settings>
using DefaultConfigurableAudioPorts = ConfigurableAudioPorts<settings, PluginAudioPortsBuffer>;


} // namespace lmms

#endif // LMMS_REMOTE_PLUGIN_AUDIO_PORTS_H
