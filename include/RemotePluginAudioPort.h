/*
 * RemotePluginAudioPort.h - PluginAudioPort implementation for RemotePlugin
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

#ifndef LMMS_REMOTE_PLUGIN_AUDIO_PORT_H
#define LMMS_REMOTE_PLUGIN_AUDIO_PORT_H

#include "PluginAudioPort.h"
#include "lmms_basics.h"
#include "lmms_export.h"

namespace lmms
{

class RemotePlugin;

/*
 * TODO: A better design would just have `RemotePluginAudioPort<config>` passed to
 *       `RemotePlugin` from the plugin implementation rather than
 *       `RemotePluginAudioPortController`, and `RemotePlugin` would be a class
 *       template with an `AudioPluginConfig` template parameter.
 *       There would also be `RemotePluginAudioBuffer`, a custom buffer implementation
 *       for remote plugins and remote plugin clients which `RemotePlugin` would
 *       derive from. However, this design requires C++20's class type NTTP on the
 *       remote plugin's client side but we're compiling that with C++17 due to a
 *       weird regression.
 */

class LMMS_EXPORT RemotePluginAudioPortController
{
public:
	RemotePluginAudioPortController(PluginPinConnector& pinConnector);

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

	auto pc() -> PluginPinConnector&
	{
		return *m_pinConnector;
	}

protected:
	void remotePluginUpdateBuffers(int channelsIn, int channelsOut, fpp_t frames);
	auto remotePluginInputBuffer() const -> float*;
	auto remotePluginOutputBuffer() const -> float*;

	RemotePlugin* m_buffers = nullptr;
	PluginPinConnector* m_pinConnector = nullptr;

	fpp_t m_frames = 0;
};


//! `PluginAudioPort` implementation for `RemotePlugin`
template<AudioPluginConfig config>
class RemotePluginAudioPort
	: public CustomPluginAudioPort<config>
	, public RemotePluginAudioPortController
{
	using SampleT = GetAudioDataType<config.kind>;

public:
	RemotePluginAudioPort(bool isInstrument, Model* parent)
		: CustomPluginAudioPort<config>{isInstrument, parent}
		, RemotePluginAudioPortController{*static_cast<PluginPinConnector*>(this)}
	{
	}

	static_assert(config.kind == AudioDataKind::F32, "RemotePlugin only supports float");
	static_assert(config.interleaved == false, "RemotePlugin only supports non-interleaved");
	static_assert(!config.inplace, "RemotePlugin does not support inplace processing");

	auto controller() -> RemotePluginAudioPortController&
	{
		return *static_cast<RemotePluginAudioPortController*>(this);
	}

	void activate(f_cnt_t frames) override
	{
		assert(m_buffers != nullptr);
		updateBuffers(this->in().channelCount(), this->out().channelCount(), frames);
		m_remoteActive = true;
	}

	/*
	 * `PluginAudioPort` implementation
	 */

	//! Only returns the buffer interface if audio port is active
	auto buffers() -> AudioPluginBufferInterface<config>* override
	{
		return remoteActive() ? this : nullptr;
	}

	/*
	 * `AudioPluginBufferInterface` implementation
	 */

	auto inputBuffer() -> SplitAudioData<SampleT, config.inputs> override
	{
		if (!remoteActive()) { return SplitAudioData<SampleT, config.inputs>{}; }

		return SplitAudioData<SampleT, config.inputs> {
			m_audioBufferIn.data(),
			static_cast<pi_ch_t>(this->in().channelCount()),
			m_frames
		};
	}

	auto outputBuffer() -> SplitAudioData<SampleT, config.outputs> override
	{
		if (!remoteActive()) { return SplitAudioData<SampleT, config.outputs>{}; }

		return SplitAudioData<SampleT, config.outputs> {
			m_audioBufferOut.data(),
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
		if (!m_buffers) { return; }
		remotePluginUpdateBuffers(channelsIn, channelsOut, frames);

		m_frames = frames;

		// Update the views into the RemotePlugin buffer
		float* ptr = remotePluginInputBuffer();
		m_audioBufferIn.resize(channelsIn);
		for (pi_ch_t idx = 0; idx < channelsIn; ++idx)
		{
			m_audioBufferIn[idx] = ptr;
			ptr += frames;
		}

		ptr = remotePluginOutputBuffer();
		m_audioBufferOut.resize(channelsOut);
		for (pi_ch_t idx = 0; idx < channelsOut; ++idx)
		{
			m_audioBufferOut[idx] = ptr;
			ptr += frames;
		}
	}

	auto active() const -> bool override { return remoteActive(); }

private:
	auto remoteActive() const -> bool { return m_buffers != nullptr && m_remoteActive; }

	// Views into RemotePlugin's shared memory buffer
	std::vector<float*> m_audioBufferIn;
	std::vector<float*> m_audioBufferOut;

protected:
	bool m_remoteActive = false;
};


//! An audio port that can choose between RemotePlugin or a local buffer at runtime
template<AudioPluginConfig config, class LocalBufferT = DefaultAudioPluginBuffer<config>>
class ConfigurableAudioPort
	: public RemotePluginAudioPort<config>
{
	using SampleT = GetAudioDataType<config.kind>;

public:
	ConfigurableAudioPort(bool isInstrument, Model* parent, bool beginAsRemote = true)
		: RemotePluginAudioPort<config>{isInstrument, parent}
	{
		useRemote(beginAsRemote);
	}

	void useRemote(bool remote = true)
	{
		if (remote) { m_localActive = false; }
		else { RemotePluginAudioPort<config>::m_remoteActive = false; }

		m_isRemote = remote;
	}

	auto isRemote() const -> bool { return m_isRemote; }

	void activate(f_cnt_t frames) override
	{
		if (isRemote()) { RemotePluginAudioPort<config>::activate(frames); return; }

		updateBuffers(this->in().channelCount(), this->out().channelCount(), frames);
		m_localActive = true;
	}

	auto buffers() -> AudioPluginBufferInterface<config>* override
	{
		if (isRemote()) { return RemotePluginAudioPort<config>::buffers(); }
		return localActive() ? &m_localBuffer.value() : nullptr;
	}

	auto inputBuffer() -> SplitAudioData<SampleT, config.inputs> override
	{
		if (isRemote()) { return RemotePluginAudioPort<config>::inputBuffer(); }
		return localActive()
			? m_localBuffer->inputBuffer()
			: SplitAudioData<SampleT, config.inputs>{};
	}

	auto outputBuffer() -> SplitAudioData<SampleT, config.outputs> override
	{
		if (isRemote()) { return RemotePluginAudioPort<config>::outputBuffer(); }
		return localActive()
			? m_localBuffer->outputBuffer()
			: SplitAudioData<SampleT, config.outputs>{};
	}

	auto frames() const -> fpp_t override
	{
		if (isRemote()) { return RemotePluginAudioPort<config>::frames(); }
		return localActive() ? m_localBuffer->frames() : 0;
	}

	void updateBuffers(int channelsIn, int channelsOut, f_cnt_t frames) override
	{
		if (isRemote())
		{
			RemotePluginAudioPort<config>::updateBuffers(channelsIn, channelsOut, frames);
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
			? RemotePluginAudioPort<config>::active()
			: localActive();
	}

private:
	auto localActive() const -> bool { return m_localBuffer.has_value() && m_localActive; }

	std::optional<LocalBufferT> m_localBuffer;
	bool m_localActive = false;
	bool m_isRemote = true;
};


} // namespace lmms

#endif // LMMS_REMOTE_PLUGIN_AUDIO_PORT_H
