/*
 * AudioPluginBuffer.h - Customizable working buffer for plugins
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

#ifndef LMMS_AUDIO_PLUGIN_BUFFER_H
#define LMMS_AUDIO_PLUGIN_BUFFER_H

#include <type_traits>
#include <vector>

#include "AudioData.h"
#include "AudioPluginConfig.h"
#include "SampleFrame.h"
#include "lmms_basics.h"

namespace lmms
{


namespace detail {

/**
 * Metafunction to select the appropriate non-owning audio buffer view
 * given the layout, sample type, and channel count
 */
template<AudioDataKind kind, bool interleaved, int channels, bool isConst>
struct AudioDataViewSelector
{
	static_assert(always_false_v<AudioDataViewSelector<kind, interleaved, channels, isConst>>,
		"Unsupported audio data type");
};

//! Non-interleaved specialization
template<AudioDataKind kind, int channels, bool isConst>
struct AudioDataViewSelector<kind, false, channels, isConst>
{
	using type = SplitAudioData<
		std::conditional_t<isConst, const GetAudioDataType<kind>, GetAudioDataType<kind>>,
		channels>;
};

//! SampleFrame specialization
template<int channels, bool isConst>
struct AudioDataViewSelector<AudioDataKind::SampleFrame, true, channels, isConst>
{
	static_assert(channels == 0 || channels == 2,
		"Plugins using SampleFrame buffers must have exactly 0 or 2 inputs or outputs");
	using type = std::conditional_t<isConst, CoreAudioData, CoreAudioDataMut>;
};


//! Provides a view into a plugin's input and output audio buffers
template<AudioPluginConfig config, bool inplace = config.inplace>
class AudioPluginBufferInterface;

//! Non-inplace specialization
template<AudioPluginConfig config>
class AudioPluginBufferInterface<config, false>
{
public:
	virtual ~AudioPluginBufferInterface() = default;

	virtual auto inputBuffer()
		-> typename detail::AudioDataViewSelector<config.kind, config.interleaved, config.inputs, false>::type = 0;

	virtual auto outputBuffer()
		-> typename detail::AudioDataViewSelector<config.kind, config.interleaved, config.outputs, false>::type = 0;

	virtual auto frames() const -> fpp_t = 0;

	virtual void updateBuffers(int channelsIn, int channelsOut, f_cnt_t frames) = 0;
};

//! Inplace specialization
template<AudioPluginConfig config>
class AudioPluginBufferInterface<config, true>
{
public:
	virtual ~AudioPluginBufferInterface() = default;

	virtual auto inputOutputBuffer()
		-> typename detail::AudioDataViewSelector<config.kind, config.interleaved, config.inputs, false>::type = 0;

	virtual auto frames() const -> fpp_t = 0;

	virtual void updateBuffers(int channelsIn, int channelsOut, f_cnt_t frames) = 0;
};


//! Default implementation of `AudioPluginBufferInterface`
template<AudioPluginConfig config,
	AudioDataKind kind = config.kind, bool interleaved = config.interleaved, bool inplace = config.inplace>
class AudioPluginBufferDefaultImpl;

//! Specialization for non-inplace, non-interleaved buffers
template<AudioPluginConfig config, AudioDataKind kind>
class AudioPluginBufferDefaultImpl<config, kind, false, false>
	: public AudioPluginBufferInterface<config>
{
	static constexpr bool s_hasStaticChannelCount
		= config.inputs != DynamicChannelCount && config.outputs != DynamicChannelCount;

	using SampleT = GetAudioDataType<kind>;

	// Optimization to avoid need for std::vector if size is known at compile time
	using AccessBufferType = std::conditional_t<
		s_hasStaticChannelCount,
		std::array<SampleT*, static_cast<std::size_t>(config.inputs + config.outputs)>,
		std::vector<SampleT*>>;

public:
	AudioPluginBufferDefaultImpl() = default;
	~AudioPluginBufferDefaultImpl() override = default;

	auto inputBuffer() -> SplitAudioData<SampleT, config.inputs> final
	{
		return SplitAudioData<SampleT, config.inputs> {
			m_accessBuffer.data(), static_cast<pi_ch_t>(m_channelsIn), m_frames
		};
	}

	auto outputBuffer() -> SplitAudioData<SampleT, config.outputs> final
	{
		return SplitAudioData<SampleT, config.outputs> {
			m_accessBuffer.data() + static_cast<pi_ch_t>(m_channelsIn),
			static_cast<pi_ch_t>(m_channelsOut),
			m_frames
		};
	}

	auto frames() const -> fpp_t final
	{
		return m_frames;
	}

	void updateBuffers(int channelsIn, int channelsOut, f_cnt_t frames) final
	{
		if (channelsIn == DynamicChannelCount || channelsOut == DynamicChannelCount) { return; }

		const auto channels = static_cast<std::size_t>(channelsIn + channelsOut);

		m_sourceBuffer.resize(channels * frames);
		if constexpr (!s_hasStaticChannelCount)
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
	AccessBufferType m_accessBuffer;

	int m_channelsIn = config.inputs;
	int m_channelsOut = config.outputs;
	f_cnt_t m_frames = 0;
};


//! Specialization for inplace, non-interleaved buffers
template<AudioPluginConfig config, AudioDataKind kind>
class AudioPluginBufferDefaultImpl<config, kind, false, true>
	: public AudioPluginBufferInterface<config>
{
	static constexpr bool s_hasStaticChannelCount
		= config.inputs != DynamicChannelCount && config.outputs != DynamicChannelCount;

	static_assert(config.inputs == config.outputs || config.inputs == 0 || config.outputs == 0,
		"compile-time inplace buffers must have same number of input channels and output channels, "
		"or one of the channel counts must be fixed at zero");

	using SampleT = GetAudioDataType<kind>;

	// Optimization to avoid need for std::vector if size is known at compile time
	using AccessBufferType = std::conditional_t<
		s_hasStaticChannelCount,
		std::array<SampleT*, static_cast<std::size_t>(config.outputs)>,
		std::vector<SampleT*>>;

public:
	AudioPluginBufferDefaultImpl() = default;
	~AudioPluginBufferDefaultImpl() override = default;

	auto inputOutputBuffer() -> SplitAudioData<SampleT, config.outputs> final
	{
		return SplitAudioData<SampleT, config.outputs> {
			m_accessBuffer.data(), static_cast<pi_ch_t>(m_channels), m_frames
		};
	}

	auto frames() const -> fpp_t final
	{
		return m_frames;
	}

	void updateBuffers(int channelsIn, int channelsOut, f_cnt_t frames) final
	{
		assert(channelsIn == channelsOut || channelsIn == 0 || channelsOut == 0);
		if (channelsIn == DynamicChannelCount || channelsOut == DynamicChannelCount) { return; }

		const auto channels = std::max<std::size_t>(channelsIn, channelsOut);

		m_sourceBuffer.resize(channels * frames);
		if constexpr (!s_hasStaticChannelCount)
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

		m_channels = channelsOut;
	}

private:
	//! All input buffers followed by all output buffers
	std::vector<SampleT> m_sourceBuffer;

	//! Provides [channel][frame] view into `m_sourceBuffer`
	AccessBufferType m_accessBuffer;

	int m_channels = config.outputs;
	f_cnt_t m_frames = 0;
};


//! Specialization for 2-channel SampleFrame buffers
template<AudioPluginConfig config>
class AudioPluginBufferDefaultImpl<config, AudioDataKind::SampleFrame, true, true>
	: public AudioPluginBufferInterface<config>
{
public:
	AudioPluginBufferDefaultImpl() = default;
	~AudioPluginBufferDefaultImpl() override = default;

	auto inputOutputBuffer() -> CoreAudioDataMut final
	{
		return CoreAudioDataMut{m_buffer.data(), m_buffer.size()};
	}

	auto frames() const -> fpp_t final
	{
		return m_buffer.size();
	}

	void updateBuffers(int channelsIn, int channelsOut, f_cnt_t frames) final
	{
		(void)channelsIn;
		(void)channelsOut;
		m_buffer.resize(frames);
	}

private:
	std::vector<SampleFrame> m_buffer;
};

} // namespace detail


//! Provides a view into a plugin's input and output audio buffers
template<AudioPluginConfig config>
using AudioPluginBufferInterface = detail::AudioPluginBufferInterface<config>;


//! Default implementation of `AudioPluginBufferInterface`
template<AudioPluginConfig config>
using DefaultAudioPluginBuffer = detail::AudioPluginBufferDefaultImpl<config>;


} // namespace lmms

#endif // LMMS_AUDIO_PLUGIN_BUFFER_H
