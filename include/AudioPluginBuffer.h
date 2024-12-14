/*
 * AudioPluginBuffer.h - Customizable working buffer for plugins
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

#ifndef LMMS_AUDIO_PLUGIN_BUFFER_H
#define LMMS_AUDIO_PLUGIN_BUFFER_H

#include <QObject>
#include <type_traits>
#include <vector>

#include "AudioData.h"
#include "AudioEngine.h"
#include "Engine.h"
#include "SampleFrame.h"
#include "lmms_basics.h"

namespace lmms
{

namespace detail
{

/**
 * Metafunction to select the appropriate non-owning audio buffer view
 * given the layout, sample type, and channel count
 */
template<AudioDataLayout layout, typename SampleT, int numChannels>
struct AudioDataTypeSelector
{
	static_assert(always_false_v<AudioDataTypeSelector<layout, SampleT, numChannels>>,
		"Unsupported audio data type");
};

template<typename SampleT, int channelCount>
struct AudioDataTypeSelector<AudioDataLayout::Split, SampleT, channelCount>
{
	using type = SplitAudioData<SampleT, channelCount>;
};

template<int channelCount>
struct AudioDataTypeSelector<AudioDataLayout::Interleaved, SampleFrame, channelCount>
{
	static_assert(channelCount == 0 || channelCount == 2,
		"Plugins using SampleFrame buffers must have exactly 0 or 2 inputs or outputs");
	using type = CoreAudioDataMut;
};

template<int channelCount>
struct AudioDataTypeSelector<AudioDataLayout::Interleaved, const SampleFrame, channelCount>
{
	static_assert(channelCount == 0 || channelCount == 2,
		"Plugins using SampleFrame buffers must have exactly 0 or 2 inputs or outputs");
	using type = CoreAudioData;
};


} // namespace detail


//! Provides a view into a plugin's input and output audio buffers
template<AudioDataLayout layout, typename SampleT, int numChannelsIn, int numChannelsOut>
class AudioPluginBufferInterface
{
public:
	virtual ~AudioPluginBufferInterface() = default;
	virtual auto inputBuffer() -> typename detail::AudioDataTypeSelector<layout, SampleT, numChannelsIn>::type = 0;
	virtual auto outputBuffer() -> typename detail::AudioDataTypeSelector<layout, SampleT, numChannelsOut>::type = 0;
	virtual void updateBuffers(int channelsIn, int channelsOut) = 0;
};


//! This lets plugin implementations provide their own buffers
template<AudioDataLayout layout, typename SampleT, int numChannelsIn, int numChannelsOut>
class AudioPluginBufferInterfaceProvider
{
protected:
	virtual auto bufferInterface()
		-> AudioPluginBufferInterface<layout, SampleT, numChannelsIn, numChannelsOut>* = 0;
};


//! Default implementation of `AudioPluginBufferInterface`
template<AudioDataLayout layout, typename SampleT, int numChannelsIn, int numChannelsOut, bool inplace>
class AudioPluginBufferDefaultImpl;

//! Specialization for split (non-interleaved) buffers
template<typename SampleT, int numChannelsIn, int numChannelsOut, bool inplace>
class AudioPluginBufferDefaultImpl<AudioDataLayout::Split, SampleT, numChannelsIn, numChannelsOut, inplace>
	: public AudioPluginBufferInterface<AudioDataLayout::Split, SampleT, numChannelsIn, numChannelsOut>
	, public AudioPluginBufferInterfaceProvider<AudioDataLayout::Split, SampleT, numChannelsIn, numChannelsOut>
{
	static constexpr bool s_hasStaticChannelCount
		= numChannelsIn != DynamicChannelCount && numChannelsOut != DynamicChannelCount;

	// Optimization to avoid need for std::vector if size is known at compile time
	using AccessBufferType = std::conditional_t<
		s_hasStaticChannelCount,
		std::array<SplitSampleType<SampleT>*, static_cast<std::size_t>(numChannelsIn + numChannelsOut)>,
		std::vector<SplitSampleType<SampleT>*>>;

public:
	static_assert(numChannelsIn == numChannelsOut || !inplace,
		"compile-time inplace buffers must have same number of input channels and output channels");

	AudioPluginBufferDefaultImpl()
		: m_frames{Engine::audioEngine()->framesPerPeriod()}
	{
		updateBuffers(numChannelsIn, numChannelsOut);
	}

	~AudioPluginBufferDefaultImpl() override = default;

	auto inputBuffer() -> SplitAudioData<SampleT, numChannelsIn> final
	{
		return SplitAudioData<SampleT, numChannelsIn> {
			m_accessBuffer.data(), static_cast<pi_ch_t>(m_channelsIn), m_frames
		};
	}

	auto outputBuffer() -> SplitAudioData<SampleT, numChannelsOut> final
	{
		return SplitAudioData<SampleT, numChannelsOut> {
			m_accessBuffer.data() + static_cast<pi_ch_t>(m_channelsIn),
			static_cast<pi_ch_t>(m_channelsOut),
			m_frames
		};
	}

	void updateBuffers(int channelsIn, int channelsOut) final
	{
		if (channelsIn == DynamicChannelCount || channelsOut == DynamicChannelCount) { return; }

		m_frames = Engine::audioEngine()->framesPerPeriod();
		const auto channels = static_cast<std::size_t>(channelsIn + channelsOut);

		m_sourceBuffer.resize(channels * m_frames);
		if constexpr (!s_hasStaticChannelCount)
		{
			m_accessBuffer.resize(channels);
		}
		else
		{
			// If channel counts are known at compile time, they should never change
			assert(channelsIn == numChannelsIn);
			assert(channelsOut == numChannelsOut);
		}

		std::size_t idx = 0;
		f_cnt_t pos = 0;
		while (idx < channels)
		{
			m_accessBuffer[idx] = m_sourceBuffer.data() + pos;
			++idx;
			pos += m_frames;
		}

		m_channelsIn = channelsIn;
		m_channelsOut = channelsOut;
	}

protected:
	auto bufferInterface() -> AudioPluginBufferDefaultImpl* final
	{
		return this;
	}

private:
	//! All input buffers followed by all output buffers
	std::vector<SplitSampleType<SampleT>> m_sourceBuffer;

	//! Provides [channel][frame] view into `m_sourceBuffer`
	AccessBufferType m_accessBuffer;

	int m_channelsIn = numChannelsIn;
	int m_channelsOut = numChannelsOut;
	f_cnt_t m_frames = 0;
};

//! Specialization for 2-channel SampleFrame buffers
template<int numChannelsIn, int numChannelsOut, bool inplace>
class AudioPluginBufferDefaultImpl<AudioDataLayout::Interleaved, SampleFrame, numChannelsIn, numChannelsOut, inplace>
	: public AudioPluginBufferInterface<AudioDataLayout::Interleaved, SampleFrame, numChannelsIn, numChannelsOut>
	, public AudioPluginBufferInterfaceProvider<AudioDataLayout::Interleaved, SampleFrame, numChannelsIn, numChannelsOut>
{
public:
	static_assert(inplace, "SampleFrame buffers are always processed in-place");

	AudioPluginBufferDefaultImpl()
	{
		updateBuffers(numChannelsIn, numChannelsOut);
	}

	~AudioPluginBufferDefaultImpl() override = default;

	auto inputBuffer() -> CoreAudioDataMut final
	{
		return CoreAudioDataMut{m_buffer.data(), m_buffer.size()};
	}

	auto outputBuffer() -> CoreAudioDataMut final
	{
		return CoreAudioDataMut{m_buffer.data(), m_buffer.size()};
	}

	void updateBuffers(int channelsIn, int channelsOut) final
	{
		(void)channelsIn;
		(void)channelsOut;
		m_buffer.resize(Engine::audioEngine()->framesPerPeriod());
	}

protected:
	auto bufferInterface() -> AudioPluginBufferDefaultImpl* final
	{
		return this;
	}

private:
	std::vector<SampleFrame> m_buffer;
};


} // namespace lmms

#endif // LMMS_AUDIO_PLUGIN_BUFFER_H
