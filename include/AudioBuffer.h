/*
 * AudioBuffer.h - Customizable audio buffer
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

#ifndef LMMS_AUDIO_BUFFER_H
#define LMMS_AUDIO_BUFFER_H

#include <type_traits>
#include <vector>

#include "AudioData.h"
#include "AudioPortsConfig.h"
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
struct AudioDataViewTypeHelper
{
	static_assert(always_false_v<AudioDataViewTypeHelper<kind, interleaved, channels, isConst>>,
		"Unsupported audio data type");
};

//! Non-interleaved specialization
template<AudioDataKind kind, int channels, bool isConst>
struct AudioDataViewTypeHelper<kind, false, channels, isConst>
{
	using type = SplitAudioData<
		std::conditional_t<isConst, const GetAudioDataType<kind>, GetAudioDataType<kind>>,
		channels>;
};

//! SampleFrame specialization
template<int channels, bool isConst>
struct AudioDataViewTypeHelper<AudioDataKind::SampleFrame, true, channels, isConst>
{
	static_assert(channels == 0 || channels == 2,
		"Plugins using SampleFrame buffers must have exactly 0 or 2 inputs or outputs");
	using type = std::conditional_t<isConst, std::span<const SampleFrame>, std::span<SampleFrame>>;
};

} // namespace detail


//! Metafunction to select the appropriate non-owning audio buffer view
template<AudioPortsConfig config, bool isOutput, bool isConst>
using AudioDataViewType = typename detail::AudioDataViewTypeHelper<
	config.kind, config.interleaved, (isOutput ? config.outputs : config.inputs), isConst>::type;


namespace detail {

//! Interface for accessing input/output audio buffers
template<AudioPortsConfig config, bool inplace = config.inplace>
class AudioBuffer;

//! Dynamically in-place specialization
template<AudioPortsConfig config>
class AudioBuffer<config, false>
{
public:
	virtual ~AudioBuffer() = default;

	virtual auto inputBuffer() -> AudioDataViewType<config, false, false> = 0;
	virtual auto outputBuffer() -> AudioDataViewType<config, true, false> = 0;
	virtual auto frames() const -> fpp_t = 0;
	virtual void updateBuffers(int channelsIn, int channelsOut, f_cnt_t frames) = 0;
};

//! Statically in-place specialization
template<AudioPortsConfig config>
class AudioBuffer<config, true>
{
public:
	virtual ~AudioBuffer() = default;

	virtual auto inputOutputBuffer() -> AudioDataViewType<config, false, false> = 0;
	virtual auto frames() const -> fpp_t = 0;
	virtual void updateBuffers(int channelsIn, int channelsOut, f_cnt_t frames) = 0;
};


//! Optimization - Choose std::array or std::vector based on whether size is known at compile time
template<AudioPortsConfig config>
using AccessBufferType = std::conditional_t<
	config.staticChannelCount(),
	std::array<GetAudioDataType<config.kind>*, static_cast<std::size_t>(config.inputs + config.outputs)>,
	std::vector<GetAudioDataType<config.kind>*>>;


//! Default implementation of `AudioBuffer`
template<AudioPortsConfig config,
	AudioDataKind kind = config.kind, bool interleaved = config.interleaved, bool inplace = config.inplace>
class DefaultAudioBuffer;

//! Specialization for dynamically in-place, non-interleaved buffers
template<AudioPortsConfig config, AudioDataKind kind>
class DefaultAudioBuffer<config, kind, false, false>
	: public AudioBuffer<config>
{
	using SampleT = GetAudioDataType<kind>;

public:
	DefaultAudioBuffer() = default;
	~DefaultAudioBuffer() override = default;

	auto inputBuffer() -> SplitAudioData<SampleT, config.inputs> final
	{
		return SplitAudioData<SampleT, config.inputs> {
			m_accessBuffer.data(), static_cast<proc_ch_t>(m_channelsIn), m_frames
		};
	}

	auto outputBuffer() -> SplitAudioData<SampleT, config.outputs> final
	{
		return SplitAudioData<SampleT, config.outputs> {
			m_accessBuffer.data() + static_cast<proc_ch_t>(m_channelsIn),
			static_cast<proc_ch_t>(m_channelsOut),
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

	int m_channelsIn = config.inputs;
	int m_channelsOut = config.outputs;
	f_cnt_t m_frames = 0;
};


//! Specialization for statically in-place, non-interleaved buffers
template<AudioPortsConfig config, AudioDataKind kind>
class DefaultAudioBuffer<config, kind, false, true>
	: public AudioBuffer<config>
{
	static_assert(config.inputs == config.outputs || config.inputs == 0 || config.outputs == 0,
		"in-place buffers must have same number of input channels and output channels, "
		"or one of the channel counts must be fixed at zero");

	using SampleT = GetAudioDataType<kind>;

public:
	DefaultAudioBuffer() = default;
	~DefaultAudioBuffer() override = default;

	auto inputOutputBuffer() -> SplitAudioData<SampleT, config.outputs> final
	{
		return SplitAudioData<SampleT, config.outputs> {
			m_accessBuffer.data(), static_cast<proc_ch_t>(m_channels), m_frames
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

		m_channels = channelsOut;
	}

private:
	//! Input/output buffers
	std::vector<SampleT> m_sourceBuffer;

	//! Provides [channel][frame] view into `m_sourceBuffer`
	AccessBufferType<config> m_accessBuffer;

	int m_channels = config.outputs;
	f_cnt_t m_frames = 0;
};


//! Specialization for 2-channel SampleFrame buffers
template<AudioPortsConfig config>
class DefaultAudioBuffer<config, AudioDataKind::SampleFrame, true, true>
	: public AudioBuffer<config>
{
public:
	DefaultAudioBuffer() = default;
	~DefaultAudioBuffer() override = default;

	auto inputOutputBuffer() -> std::span<SampleFrame> final
	{
		return std::span<SampleFrame>{m_buffer.data(), m_buffer.size()};
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


//! Interface for accessing input/output audio buffers
template<AudioPortsConfig config>
using AudioBuffer = detail::AudioBuffer<config>;


//! Default implementation of `AudioBuffer`
template<AudioPortsConfig config>
using DefaultAudioBuffer = detail::DefaultAudioBuffer<config>;


} // namespace lmms

#endif // LMMS_AUDIO_BUFFER_H
