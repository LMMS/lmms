/*
 * AudioData.h - Audio data types
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

#ifndef LMMS_AUDIO_DATA_H
#define LMMS_AUDIO_DATA_H

#include <cassert>
#include <type_traits>

#include "lmms_basics.h"

namespace lmms
{


//! Types of audio data supported in LMMS
enum class AudioDataKind : std::uint8_t
{
	SampleFrame,
	F32,
	// F64,
	// I16,
	// etc.
};

namespace detail {

//! Specialize this struct to enable the use of an audio data kind
template<AudioDataKind kind> struct AudioDataType;

template<> struct AudioDataType<AudioDataKind::F32> { using type = float; };

} // namespace detail


//! Metafunction to convert `AudioDataKind` to its type
template<AudioDataKind kind>
using GetAudioDataType = typename detail::AudioDataType<kind>::type;


//! Use when the number of channels is not known at compile time
inline constexpr int DynamicChannelCount = -1;


/**
 * Non-owning view for multi-channel non-interleaved audio data
 *
 * TODO C++23: Use std::mdspan
 */
template<typename SampleT, int channelCount = DynamicChannelCount>
class SplitAudioData
{
public:
	SplitAudioData() = default;
	SplitAudioData(const SplitAudioData&) = default;

	/**
	 * `data` is a 2D array of channels to buffers:
	 *  data[channels][frames]
	 * Each buffer contains `frames` frames.
	 */
	SplitAudioData(SampleT* const* data, pi_ch_t channels, f_cnt_t frames)
		: m_data{data}
		, m_channels{channels}
		, m_frames{frames}
	{
		assert(channelCount == DynamicChannelCount || channels == channelCount);
	}

	//! Contruct const from mutable
	template<typename T = SampleT, std::enable_if_t<std::is_const_v<T>, bool> = true>
	SplitAudioData(const SplitAudioData<std::remove_const_t<T>, channelCount>& other)
		: m_data{other.data()}
		, m_channels{other.channels()}
		, m_frames{other.frames()}
	{
	}

	/**
	 * Returns pointer to the buffer of a given channel.
	 * The size of the buffer is `frames()`.
	 */
	auto buffer(pi_ch_t channel) const -> SampleT*
	{
		assert(channel < m_channels);
		assert(m_data != nullptr);
		return m_data[channel];
	}

	template<pi_ch_t channel>
	auto buffer() const -> SampleT*
	{
		static_assert(channel != DynamicChannelCount);
		static_assert(channel < channelCount);
		assert(m_data != nullptr);
		return m_data[channel];
	}

	constexpr auto channels() const -> pi_ch_t
	{
		if constexpr (channelCount == DynamicChannelCount)
		{
			return m_channels;
		}
		else
		{
			return static_cast<pi_ch_t>(channelCount);
		}
	}

	auto frames() const -> f_cnt_t { return m_frames; }

	auto empty() const -> bool { return !m_data || m_channels == 0 || m_frames == 0; }

	/**
	 * WARNING: This method assumes that internally there is a single
	 *     contiguous buffer for all channels whose size is channels() * frames().
	 *     Whether this is true depends on the implementation of the source buffer.
	 */
	auto sourceBuffer() const -> Span<SampleT>
	{
		assert(m_data != nullptr);
		return Span<SampleT>{m_data[0], channels() * frames()};
	}

	auto data() const -> SampleT* const* { return m_data; }

private:
	SampleT* const* m_data = nullptr;
	pi_ch_t m_channels = 0;
	f_cnt_t m_frames = 0;
};


//! Converts between sample types
template<typename Out, typename In>
inline auto convertSample(const In sample) -> Out
{
	if constexpr (std::is_floating_point_v<In> && std::is_floating_point_v<Out>)
	{
		return static_cast<Out>(sample);
	}
	else
	{
		static_assert(always_false_v<In, Out>, "only implemented for floating point samples");
	}
}


} // namespace lmms

#endif // LMMS_AUDIO_DATA_H
