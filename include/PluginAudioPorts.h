/*
 * PluginAudioPorts.h - Default `AudioPorts` implementation for
 *                      audio plugins
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

#ifndef LMMS_PLUGIN_AUDIO_PORTS_H
#define LMMS_PLUGIN_AUDIO_PORTS_H

#include "AudioPorts.h"

namespace lmms
{

namespace detail {

//! Optimization - Choose std::array or std::vector based on whether size is known at compile time
template<AudioPortsConfig config>
using AccessBufferType = std::conditional_t<
	config.staticChannelCount(),
	std::array<GetAudioDataType<config.kind>*, config.inputs + config.outputs>,
	std::vector<GetAudioDataType<config.kind>*>>;


//! Default implementation of `AudioPorts::Buffer` for audio plugins
template<AudioPortsConfig config,
	AudioDataKind kind = config.kind, bool interleaved = config.interleaved, bool inplace = config.inplace>
class PluginAudioPortsBuffer
{
	static_assert(always_false_v<PluginAudioPortsBuffer<config>>, "Unsupported audio port configuration");
};

//! Specialization for dynamically in-place, non-interleaved buffers
template<AudioPortsConfig config, AudioDataKind kind>
class PluginAudioPortsBuffer<config, kind, false, false>
	: public AudioPorts<config>::Buffer
{
	using SampleT = GetAudioDataType<kind>;

public:
	PluginAudioPortsBuffer() = default;
	~PluginAudioPortsBuffer() override = default;

	auto inputBuffer() -> SplitAudioData<SampleT, config.inputs> final
	{
		return {m_accessBuffer.data(), m_channelsIn, m_frames};
	}

	auto outputBuffer() -> SplitAudioData<SampleT, config.outputs> final
	{
		return {m_accessBuffer.data() + m_channelsIn, m_channelsOut, m_frames};
	}

	auto frames() const -> fpp_t final
	{
		return m_frames;
	}

	void updateBuffers(proc_ch_t channelsIn, proc_ch_t channelsOut, f_cnt_t frames) final
	{
		if (channelsIn == DynamicChannelCount || channelsOut == DynamicChannelCount) { return; }

		const auto channels = static_cast<std::size_t>(channelsIn + channelsOut);

		m_sourceBuffer.resize(channels * frames);
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

		SampleT* ptr = m_sourceBuffer.data();
		for (std::size_t channel = 0; channel < channels; ++channel)
		{
			m_accessBuffer[channel] = ptr;
			ptr += frames;
		}

		m_channelsIn = channelsIn;
		m_channelsOut = channelsOut;
	}

private:
	//! All input buffers followed by all output buffers
	std::vector<SampleT> m_sourceBuffer;

	//! Provides [channel][frame] view into `m_sourceBuffer`
	AccessBufferType<config> m_accessBuffer;

	proc_ch_t m_channelsIn = config.inputs;
	proc_ch_t m_channelsOut = config.outputs;
	f_cnt_t m_frames = 0;
};


//! Specialization for statically in-place, non-interleaved buffers
template<AudioPortsConfig config, AudioDataKind kind>
class PluginAudioPortsBuffer<config, kind, false, true>
	: public AudioPorts<config>::Buffer
{
	static_assert(config.inputs == config.outputs || config.inputs == 0 || config.outputs == 0,
		"in-place buffers must have same number of input channels and output channels, "
		"or one of the channel counts must be fixed at zero");

	using SampleT = GetAudioDataType<kind>;

public:
	PluginAudioPortsBuffer() = default;
	~PluginAudioPortsBuffer() override = default;

	auto inputOutputBuffer() -> SplitAudioData<SampleT, config.outputs> final
	{
		return {m_accessBuffer.data(), m_channels, m_frames};
	}

	auto frames() const -> fpp_t final
	{
		return m_frames;
	}

	void updateBuffers(proc_ch_t channelsIn, proc_ch_t channelsOut, f_cnt_t frames) final
	{
		assert(channelsIn == channelsOut || channelsIn == 0 || channelsOut == 0);
		if (channelsIn == DynamicChannelCount || channelsOut == DynamicChannelCount) { return; }

		const auto channels = std::max(channelsIn, channelsOut);

		m_sourceBuffer.resize(channels * frames);
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

		SampleT* ptr = m_sourceBuffer.data();
		for (proc_ch_t channel = 0; channel < channels; ++channel)
		{
			m_accessBuffer[channel] = ptr;
			ptr += frames;
		}

		m_channels = channelsOut;
	}

private:
	//! Input/output buffers
	std::vector<SampleT> m_sourceBuffer;

	//! Provides [channel][frame] view into `m_sourceBuffer`
	AccessBufferType<config> m_accessBuffer;

	proc_ch_t m_channels = config.outputs;
	f_cnt_t m_frames = 0;
};


//! Specialization for 2-channel SampleFrame buffers
template<AudioPortsConfig config>
class PluginAudioPortsBuffer<config, AudioDataKind::SampleFrame, true, true>
	: public AudioPorts<config>::Buffer
{
public:
	PluginAudioPortsBuffer() = default;
	~PluginAudioPortsBuffer() override = default;

	auto inputOutputBuffer() -> std::span<SampleFrame> final
	{
		return m_buffer;
	}

	auto frames() const -> fpp_t final
	{
		return m_buffer.size();
	}

	void updateBuffers(proc_ch_t channelsIn, proc_ch_t channelsOut, f_cnt_t frames) final
	{
		(void)channelsIn;
		(void)channelsOut;
		m_buffer.resize(frames);
	}

private:
	std::vector<SampleFrame> m_buffer;
};


/**
 * The default audio port for audio plugins that do not provide their own.
 * Contains an audio port model and audio buffers.
 *
 * This audio port still has *some* ability for customization by using a custom `BufferT`,
 * but for full control, you'll need to provide your own audio port implementation.
 */
template<AudioPortsConfig config, template<AudioPortsConfig> class BufferT>
class PluginAudioPorts
	: public AudioPorts<config>
	, public BufferT<config>
{
	static_assert(std::is_base_of_v<typename AudioPorts<config>::Buffer, BufferT<config>>,
		"BufferT must derive from AudioPorts::Buffer");

public:
	using AudioPorts<config>::AudioPorts;

	using Buffer = BufferT<config>;

	auto buffers() -> BufferT<config>* override { return static_cast<BufferT<config>*>(this); }

	auto channelName(proc_ch_t channel, bool isOutput) const -> QString override
	{
		if (isOutput)
		{
			switch (this->out().channelCount())
			{
				case 1:
					return this->tr("Plugin Out");
				case 2:
					assert(channel < 2);
					return channel == 0 ? this->tr("Plugin Out L") : this->tr("Plugin Out R");
				default:
					return this->tr("Plugin Out %1").arg(channel + 1);
			}
		}
		else
		{
			switch (this->in().channelCount())
			{
				case 1:
					return this->tr("Plugin In");
				case 2:
					assert(channel < 2);
					return channel == 0 ? this->tr("Plugin In L") : this->tr("Plugin In R");
				default:
					return this->tr("Plugin In %1").arg(channel + 1);
			}
		}
	}

private:
	void bufferPropertiesChanged(proc_ch_t inChannels, proc_ch_t outChannels, f_cnt_t frames) final
	{
		// Connects the audio port model to the buffers
		this->updateBuffers(inChannels, outChannels, frames);
	}
};

} // namespace detail


//! Default implementation of `AudioPorts::Buffer` for audio plugins
template<AudioPortsConfig config>
using PluginAudioPortsBuffer = detail::PluginAudioPortsBuffer<config>;

//! Default implementation of `AudioPorts` for audio plugins
template<AudioPortsConfig config>
using PluginAudioPorts = detail::PluginAudioPorts<config, PluginAudioPortsBuffer>;

} // namespace lmms

#endif // LMMS_PLUGIN_AUDIO_PORTS_H
