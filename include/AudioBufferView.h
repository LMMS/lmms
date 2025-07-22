/*
 * AudioBufferView.h - Non-owning views for interleaved and
 *                     non-interleaved (planar) buffers
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

#ifndef LMMS_AUDIO_BUFFER_VIEW_H
#define LMMS_AUDIO_BUFFER_VIEW_H

#include <cassert>
#include <span>
#include <type_traits>

#include "LmmsTypes.h"
#include "SampleFrame.h"

namespace lmms
{

//! Use when the number of channels is not known at compile time
inline constexpr auto DynamicChannelCount = static_cast<proc_ch_t>(-1);


namespace detail {

// For static channel count
template<typename SampleT, proc_ch_t channelCount>
class BufferViewData
{
public:
	constexpr BufferViewData() = default;
	constexpr BufferViewData(const BufferViewData&) = default;

	constexpr BufferViewData(SampleT* data, [[maybe_unused]] proc_ch_t channels, f_cnt_t frames) noexcept
		: m_data{data}
		, m_frames{frames}
	{
		assert(channels == channelCount);
	}

	constexpr BufferViewData(SampleT* data, f_cnt_t frames) noexcept
		: m_data{data}
		, m_frames{frames}
	{
	}

	constexpr auto data() const noexcept -> SampleT* { return m_data; }
	static constexpr auto channels() noexcept -> proc_ch_t { return channelCount; }
	constexpr auto frames() const noexcept -> f_cnt_t { return m_frames; }

protected:
	SampleT* m_data = nullptr;
	f_cnt_t m_frames = 0;
};

// For dynamic channel count
template<typename SampleT>
class BufferViewData<SampleT, DynamicChannelCount>
{
public:
	constexpr BufferViewData() = default;
	constexpr BufferViewData(const BufferViewData&) = default;

	constexpr BufferViewData(SampleT* data, proc_ch_t channels, f_cnt_t frames) noexcept
		: m_data{data}
		, m_channels{channels}
		, m_frames{frames}
	{
		assert(channels != DynamicChannelCount);
	}

	constexpr auto data() const noexcept -> SampleT* { return m_data; }
	constexpr auto channels() const noexcept -> proc_ch_t { return m_channels; }
	constexpr auto frames() const noexcept -> f_cnt_t { return m_frames; }

protected:
	SampleT* m_data = nullptr;
	proc_ch_t m_channels = 0;
	f_cnt_t m_frames = 0;
};

template<typename T, typename... AllowedTs>
inline constexpr bool OneOf = (std::is_same_v<T, AllowedTs> || ...);

} // namespace detail


//! Recognized sample types, either const or non-const
template<typename T>
concept SampleType = detail::OneOf<std::remove_const_t<T>,
	float,
	double,
	std::int8_t,
	std::uint8_t,
	std::int16_t,
	std::uint16_t,
	std::int32_t,
	std::uint32_t,
	std::int64_t,
	std::uint64_t
>;


/**
 * Non-owning view for multi-channel interleaved audio data
 *
 * TODO C++23: Use std::mdspan?
 */
template<SampleType SampleT, proc_ch_t channelCount = DynamicChannelCount>
class InterleavedBufferView : public detail::BufferViewData<SampleT, channelCount>
{
	using Base = detail::BufferViewData<SampleT, channelCount>;

public:
	using Base::Base;

	//! Contruct const from mutable (static channel count)
	template<typename T = SampleT> requires (std::is_const_v<T> && channelCount != DynamicChannelCount)
	constexpr InterleavedBufferView(InterleavedBufferView<std::remove_const_t<T>, channelCount> other) noexcept
		: Base{other.data(), other.frames()}
	{
	}

	//! Contruct const from mutable (dynamic channel count)
	template<typename T = SampleT> requires (std::is_const_v<T> && channelCount == DynamicChannelCount)
	constexpr InterleavedBufferView(InterleavedBufferView<std::remove_const_t<T>, channelCount> other) noexcept
		: Base{other.data(), other.channels(), other.frames()}
	{
	}

	//! Construct dynamic channel count from static
	template<proc_ch_t otherChannels>
		requires (channelCount == DynamicChannelCount && otherChannels != DynamicChannelCount)
	constexpr InterleavedBufferView(InterleavedBufferView<SampleT, otherChannels> other) noexcept
		: Base{other.data(), otherChannels, other.frames()}
	{
	}

	//! Construct from std::span<SampleFrame>
	InterleavedBufferView(std::span<SampleFrame> buffer) noexcept
		requires (std::is_same_v<std::remove_const_t<SampleT>, float> && channelCount == 2)
		: Base{reinterpret_cast<float*>(buffer.data()), buffer.size()}
	{
	}

	//! Construct from std::span<const SampleFrame>
	InterleavedBufferView(std::span<const SampleFrame> buffer) noexcept
		requires (std::is_same_v<SampleT, const float> && channelCount == 2)
		: Base{reinterpret_cast<const float*>(buffer.data()), buffer.size()}
	{
	}

	constexpr auto empty() const noexcept -> bool
	{
		return !this->m_data || Base::channels() == 0 || this->m_frames == 0;
	}

	constexpr auto dataSizeBytes() const noexcept -> std::size_t
	{
		return Base::channels() * this->m_frames * sizeof(SampleT);
	}

	constexpr auto dataView() noexcept -> std::span<SampleT>
	{
		return std::span<SampleT>{this->m_data, this->m_frames * Base::channels()};
	}

	//! @return the frame at the given index
	constexpr auto frame(f_cnt_t index) const noexcept
	{
		if constexpr (channelCount == DynamicChannelCount)
		{
			return std::span<SampleT>{framePtr(index), Base::channels()};
		}
		else
		{
			return std::span<SampleT, channelCount>{framePtr(index), Base::channels()};
		}
	}

	/**
	 * @return pointer to the frame at the given index.
	 * The size of the frame is `channels()`.
	 */
	constexpr auto framePtr(f_cnt_t index) const noexcept -> SampleT*
	{
		assert(index < this->m_frames);
		return this->m_data + index * Base::channels();
	}

	/**
	 * @return pointer to the frame at the given index.
	 * The size of the frame is `channels()`.
	 */
	constexpr auto operator[](f_cnt_t index) const noexcept -> SampleT*
	{
		return framePtr(index);
	}

	auto sampleFrameAt(f_cnt_t index) noexcept -> SampleFrame&
		requires (std::is_same_v<SampleT, float> && channelCount == 2)
	{
		assert(index < this->m_frames);
		return reinterpret_cast<SampleFrame*>(this->m_data)[index];
	}

	auto sampleFrameAt(f_cnt_t index) const noexcept -> const SampleFrame&
		requires (std::is_same_v<SampleT, const float> && channelCount == 2)
	{
		assert(index < this->m_frames);
		return reinterpret_cast<const SampleFrame*>(this->m_data)[index];
	}

	auto toSampleFrames() noexcept -> std::span<SampleFrame>
		requires (std::is_same_v<SampleT, float> && channelCount == 2)
	{
		return {reinterpret_cast<SampleFrame*>(this->m_data), this->m_frames};
	}

	auto toSampleFrames() const noexcept -> std::span<const SampleFrame>
		requires (std::is_same_v<SampleT, const float> && channelCount == 2)
	{
		return {reinterpret_cast<const SampleFrame*>(this->m_data), this->m_frames};
	}
};

// Check that the std::span-like space optimization works
static_assert(sizeof(InterleavedBufferView<float>) > sizeof(InterleavedBufferView<float, 2>));
static_assert(sizeof(InterleavedBufferView<float, 2>) == sizeof(void*) + sizeof(f_cnt_t));


/**
 * Non-owning view for multi-channel non-interleaved audio data
 *
 * The data type is `SampleT* const*` which is a 2D array accessed as data[channel][frame index]
 * where each channel's buffer contains `frames()` frames.
 *
 * TODO C++23: Use std::mdspan?
 */
template<SampleType SampleT, proc_ch_t channelCount = DynamicChannelCount>
class PlanarBufferView : public detail::BufferViewData<SampleT* const, channelCount>
{
	using Base = detail::BufferViewData<SampleT* const, channelCount>;

public:
	using Base::Base;

	//! Contruct const from mutable (static channel count)
	template<typename T = SampleT> requires (std::is_const_v<T> && channelCount != DynamicChannelCount)
	constexpr PlanarBufferView(PlanarBufferView<std::remove_const_t<T>, channelCount> other) noexcept
		: Base{other.data(), other.frames()}
	{
	}

	//! Contruct const from mutable (dynamic channel count)
	template<typename T = SampleT> requires (std::is_const_v<T> && channelCount == DynamicChannelCount)
	constexpr PlanarBufferView(PlanarBufferView<std::remove_const_t<T>, channelCount> other) noexcept
		: Base{other.data(), other.channels(), other.frames()}
	{
	}

	//! Construct dynamic channel count from static
	template<proc_ch_t otherChannels>
		requires (channelCount == DynamicChannelCount && otherChannels != DynamicChannelCount)
	constexpr PlanarBufferView(PlanarBufferView<SampleT, otherChannels> other) noexcept
		: Base{other.data(), otherChannels, other.frames()}
	{
	}

	constexpr auto empty() const noexcept -> bool
	{
		return !this->m_data || Base::channels() == 0 || this->m_frames == 0;
	}

	//! @return the buffer of the given channel
	constexpr auto buffer(proc_ch_t channel) const noexcept -> std::span<SampleT>
	{
		return {bufferPtr(channel), this->m_frames};
	}

	//! @return the buffer of the given channel
	template<proc_ch_t channel> requires (channelCount != DynamicChannelCount)
	constexpr auto buffer() const noexcept -> std::span<SampleT>
	{
		return {bufferPtr<channel>(), this->m_frames};
	}

	/**
	 * @return pointer to the buffer of the given channel.
	 * The size of the buffer is `frames()`.
	 */
	constexpr auto bufferPtr(proc_ch_t channel) const noexcept -> SampleT*
	{
		assert(channel < Base::channels());
		assert(this->m_data != nullptr);
		return this->m_data[channel];
	}

	/**
	 * @return pointer to the buffer of the given channel.
	 * The size of the buffer is `frames()`.
	 */
	template<proc_ch_t channel> requires (channelCount != DynamicChannelCount)
	constexpr auto bufferPtr() const noexcept -> SampleT*
	{
		static_assert(channel < channelCount);
		assert(this->m_data != nullptr);
		return this->m_data[channel];
	}

	/**
	 * @return pointer to the buffer of a given channel.
	 * The size of the buffer is `frames()`.
	 */
	constexpr auto operator[](proc_ch_t channel) const noexcept -> SampleT*
	{
		return bufferPtr(channel);
	}
};

// Check that the std::span-like space optimization works
static_assert(sizeof(PlanarBufferView<float>) > sizeof(PlanarBufferView<float, 2>));
static_assert(sizeof(PlanarBufferView<float, 2>) == sizeof(void**) + sizeof(f_cnt_t));

} // namespace lmms

#endif // LMMS_AUDIO_BUFFER_VIEW_H
