/*
 * AudioBufferSpan.h - Non-owning views for interleaved and
 *                     non-interleaved (planar) buffers
 *
 * Copyright (c) 2025-2026 Dalton Messmer <messmer.dalton/at/gmail.com>
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

#ifndef LMMS_AUDIO_BUFFER_SPAN_H
#define LMMS_AUDIO_BUFFER_SPAN_H

#include <cassert>
#include <concepts>
#include <ranges>
#include <span>
#include <type_traits>

#include "LmmsTypes.h"
#include "SampleFrame.h"

namespace lmms
{

//! Use when the number of channels is not known at compile time
inline constexpr auto DynamicChannelCount = static_cast<ch_cnt_t>(-1);


namespace detail {

// For buffer views with static channel count
template<typename T, ch_cnt_t channelCount>
class BufferViewData
{
public:
	constexpr BufferViewData() = default;
	constexpr BufferViewData(const BufferViewData&) = default;

	constexpr BufferViewData(T* data, [[maybe_unused]] ch_cnt_t channels, f_cnt_t frames) noexcept
		: m_data{data}
		, m_frames{frames}
	{
		assert(channels == channelCount);
	}

	constexpr BufferViewData(T* data, f_cnt_t frames) noexcept
		: m_data{data}
		, m_frames{frames}
	{
	}

	constexpr auto data() const noexcept -> T* { return m_data; }
	static constexpr auto channels() noexcept -> ch_cnt_t { return channelCount; }
	constexpr auto frames() const noexcept -> f_cnt_t { return m_frames; }

protected:
	T* m_data = nullptr;
	f_cnt_t m_frames = 0;
};

// For buffer views with dynamic channel count
template<typename T>
class BufferViewData<T, DynamicChannelCount>
{
public:
	constexpr BufferViewData() = default;
	constexpr BufferViewData(const BufferViewData&) = default;

	constexpr BufferViewData(T* data, ch_cnt_t channels, f_cnt_t frames) noexcept
		: m_data{data}
		, m_channels{channels}
		, m_frames{frames}
	{
		assert(channels != DynamicChannelCount);
	}

	constexpr auto data() const noexcept -> T* { return m_data; }
	constexpr auto channels() const noexcept -> ch_cnt_t { return m_channels; }
	constexpr auto frames() const noexcept -> f_cnt_t { return m_frames; }

protected:
	T* m_data = nullptr;
	ch_cnt_t m_channels = 0;
	f_cnt_t m_frames = 0;
};

// For interleaved frame iterators with static channel count
template<typename T, ch_cnt_t channelCount>
class InterleavedFrameIteratorData
{
public:
	constexpr InterleavedFrameIteratorData() = default;
	constexpr InterleavedFrameIteratorData(const InterleavedFrameIteratorData&) = default;

	constexpr explicit InterleavedFrameIteratorData(T* data) noexcept
		: m_data{data}
	{
	}

	static constexpr auto channels() noexcept -> ch_cnt_t { return channelCount; }

protected:
	T* m_data = nullptr;
};

// For interleaved frame iterators with dynamic channel count
template<typename T>
class InterleavedFrameIteratorData<T, DynamicChannelCount>
{
public:
	constexpr InterleavedFrameIteratorData() = default;
	constexpr InterleavedFrameIteratorData(const InterleavedFrameIteratorData&) = default;

	constexpr InterleavedFrameIteratorData(T* data, ch_cnt_t channels) noexcept
		: m_data{data}
		, m_channels{channels}
	{
	}

	constexpr auto channels() const noexcept -> ch_cnt_t { return m_channels; }

protected:
	T* m_data = nullptr;
	ch_cnt_t m_channels = 0;
};

// Allows for iterating over the frames of `InterleavedBufferSpan`
template<typename T, ch_cnt_t channelCount = DynamicChannelCount>
class InterleavedFrameIterator : public InterleavedFrameIteratorData<T, channelCount>
{
	using Base = InterleavedFrameIteratorData<T, channelCount>;

public:
	using iterator_concept = std::random_access_iterator_tag;
	using value_type = T*;
	using difference_type = std::ptrdiff_t;

	using Base::Base;

	constexpr auto operator*() const noexcept -> value_type { return this->m_data; }

	constexpr auto operator[](difference_type frames) const noexcept -> value_type
	{
		return this->m_data[frames * Base::channels()];
	}

	constexpr auto operator++() noexcept -> InterleavedFrameIterator&
	{
		this->m_data += Base::channels();
		return *this;
	}

	constexpr auto operator++(int) noexcept -> InterleavedFrameIterator
	{
		auto temp = *this;
		++(*this);
		return temp;
	}

	constexpr auto operator--() noexcept -> InterleavedFrameIterator&
	{
		this->m_data -= Base::channels();
		return *this;
	}

	constexpr auto operator--(int) noexcept -> InterleavedFrameIterator
	{
		auto temp = *this;
		--(*this);
		return temp;
	}

	constexpr auto operator+=(difference_type channels) noexcept -> InterleavedFrameIterator&
	{
		this->m_data += channels * Base::channels();
		return *this;
	}

	constexpr auto operator-=(difference_type channels) noexcept -> InterleavedFrameIterator&
	{
		this->m_data -= channels * Base::channels();
		return *this;
	}

	friend constexpr auto operator+(InterleavedFrameIterator iter, difference_type frames) noexcept
		-> InterleavedFrameIterator
	{
		if constexpr (channelCount == DynamicChannelCount)
		{
			return {iter.m_data + frames * Base::channels(), Base::channels()};
		}
		else
		{
			return InterleavedFrameIterator{iter.m_data + frames * Base::channels()};
		}
	}

	friend constexpr auto operator+(difference_type frames, InterleavedFrameIterator iter) noexcept
		-> InterleavedFrameIterator
	{
		return iter + frames;
	}

	constexpr auto operator-(difference_type frames) const noexcept -> InterleavedFrameIterator
	{
		if constexpr (channelCount == DynamicChannelCount)
		{
			return {this->m_data - frames * Base::channels(), Base::channels()};
		}
		else
		{
			return InterleavedFrameIterator{this->m_data - frames * Base::channels()};
		}
	}

	constexpr auto operator-(InterleavedFrameIterator other) const noexcept -> difference_type
	{
		return this->m_data - other.m_data;
	}

	constexpr auto operator<=>(InterleavedFrameIterator other) const noexcept
	{
		return this->m_data <=> other.m_data;
	}

	constexpr auto operator==(InterleavedFrameIterator other) const noexcept -> bool
	{
		return this->m_data == other.m_data;
	}

	constexpr auto operator<=>(T* sentinel) const noexcept
	{
		return this->m_data <=> sentinel;
	}

	constexpr auto operator==(T* sentinel) const noexcept -> bool
	{
		return this->m_data == sentinel;
	}

	constexpr auto base() const noexcept -> T* { return this->m_data; }
};

static_assert(std::random_access_iterator<InterleavedFrameIterator<float, 2>>);

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
template<SampleType T, ch_cnt_t channelCount = DynamicChannelCount>
class InterleavedBufferSpan : public detail::BufferViewData<T, channelCount>
{
	using Base = detail::BufferViewData<T, channelCount>;

	using FrameIter = detail::InterleavedFrameIterator<T, channelCount>;
	using ConstFrameIter = detail::InterleavedFrameIterator<const T, channelCount>;

public:
	using Base::Base;

	//! Construct const from mutable (static channel count)
	template<typename U = T> requires (std::is_const_v<U> && channelCount != DynamicChannelCount)
	constexpr InterleavedBufferSpan(InterleavedBufferSpan<std::remove_const_t<U>, channelCount> other) noexcept
		: Base{other.data(), other.frames()}
	{
	}

	//! Construct const from mutable (dynamic channel count)
	template<typename U = T> requires (std::is_const_v<U> && channelCount == DynamicChannelCount)
	constexpr InterleavedBufferSpan(InterleavedBufferSpan<std::remove_const_t<U>, channelCount> other) noexcept
		: Base{other.data(), other.channels(), other.frames()}
	{
	}

	//! Construct dynamic channel count from static
	template<ch_cnt_t otherChannels>
		requires (channelCount == DynamicChannelCount && otherChannels != DynamicChannelCount)
	constexpr InterleavedBufferSpan(InterleavedBufferSpan<T, otherChannels> other) noexcept
		: Base{other.data(), otherChannels, other.frames()}
	{
	}

	//! Construct from SampleFrame*
	InterleavedBufferSpan(SampleFrame* data, f_cnt_t frames) noexcept
		requires (std::is_same_v<std::remove_const_t<T>, float> && channelCount == 2)
		: Base{reinterpret_cast<float*>(data), frames}
	{
	}

	//! Construct from const SampleFrame*
	InterleavedBufferSpan(const SampleFrame* data, f_cnt_t frames) noexcept
		requires (std::is_same_v<T, const float> && channelCount == 2)
		: Base{reinterpret_cast<const float*>(data), frames}
	{
	}

	//! Construct from std::span<SampleFrame>
	InterleavedBufferSpan(std::span<SampleFrame> data) noexcept
		requires (std::is_same_v<std::remove_const_t<T>, float> && channelCount == 2)
		: Base{reinterpret_cast<float*>(data.data()), static_cast<f_cnt_t>(data.size())}
	{
	}

	//! Construct from std::span<const SampleFrame>
	InterleavedBufferSpan(std::span<const SampleFrame> data) noexcept
		requires (std::is_same_v<std::remove_const_t<T>, float> && channelCount == 2)
		: Base{reinterpret_cast<const float*>(data.data()), static_cast<f_cnt_t>(data.size())}
	{
	}

	constexpr auto empty() const noexcept -> bool
	{
		return !this->m_data || Base::channels() == 0 || this->m_frames == 0;
	}

	constexpr auto dataSizeBytes() const noexcept -> std::size_t
	{
		return Base::channels() * this->m_frames * sizeof(T);
	}

	constexpr auto dataView() noexcept -> std::span<T>
	{
		return {this->m_data, static_cast<std::size_t>(this->m_frames * Base::channels())};
	}

	//! @return the sample at the given channel and frame indicies
	constexpr auto sample(ch_cnt_t channel, f_cnt_t frame) const noexcept -> T&
	{
		return framePtr(frame)[channel];
	}

	//! @return the frame at the given index
	constexpr auto frame(f_cnt_t index) const noexcept
	{
		if constexpr (channelCount == DynamicChannelCount)
		{
			return std::span<T>{framePtr(index), Base::channels()};
		}
		else
		{
			return std::span<T, channelCount>{framePtr(index), Base::channels()};
		}
	}

	/**
	 * @return pointer to the frame at the given index.
	 * The size of the frame is `channels()`.
	 */
	constexpr auto framePtr(f_cnt_t index) const noexcept -> T*
	{
		assert(index < this->m_frames);
		return this->m_data + index * Base::channels();
	}

	/**
	 * @return pointer to the frame at the given index.
	 * The size of the frame is `channels()`.
	 */
	constexpr auto operator[](f_cnt_t index) const noexcept -> T*
	{
		return framePtr(index);
	}

	/**
	 * @returns a new span with a subset of this span's frames
	 * @param start the new first frame
	 * @pre !empty()
	 * @pre start <= frames()
	 */
	constexpr auto subspan(f_cnt_t start) const noexcept -> InterleavedBufferSpan<T, channelCount>
	{
		assert(!empty());
		assert(start <= this->m_frames);
		if constexpr (channelCount == DynamicChannelCount)
		{
			return {this->m_data + start * Base::channels(), Base::channels(), this->m_frames - start};
		}
		else
		{
			return {this->m_data + start * Base::channels(), this->m_frames - start};
		}
	}

	/**
	 * @returns a new span with a subset of this span's frames
	 * @param start the new first frame
	 * @param count the number of frames following @p start
	 * @pre !empty()
	 * @pre start <= frames()
	 * @pre count <= frames() - start
	 */
	constexpr auto subspan(f_cnt_t start, f_cnt_t count) const noexcept
		-> InterleavedBufferSpan<T, channelCount>
	{
		assert(!empty());
		assert(start <= this->m_frames);
		assert(count <= this->m_frames - start);
		if constexpr (channelCount == DynamicChannelCount)
		{
			return {this->m_data + start * Base::channels(), Base::channels(), count};
		}
		else
		{
			return {this->m_data + start * Base::channels(), count};
		}
	}

	//! @returns a const view over the frames. Iterates in chunks containing `channels()` elements.
	constexpr auto framesView() const noexcept -> std::ranges::subrange<ConstFrameIter, const T*>
	{
		const T* end = this->m_data + Base::channels() * this->m_frames;
		if constexpr (channelCount == DynamicChannelCount)
		{
			return std::ranges::subrange{ConstFrameIter{this->m_data, Base::channels()}, end};
		}
		else
		{
			return std::ranges::subrange{ConstFrameIter{this->m_data}, end};
		}
	}

	//! @returns a view over the frames. Iterates in chunks containing `channels()` elements.
	constexpr auto framesView() noexcept -> std::ranges::subrange<FrameIter, T*>
	{
		T* end = this->m_data + Base::channels() * this->m_frames;
		if constexpr (channelCount == DynamicChannelCount)
		{
			return std::ranges::subrange{FrameIter{this->m_data, Base::channels()}, end};
		}
		else
		{
			return std::ranges::subrange{FrameIter{this->m_data}, end};
		}
	}

	auto sampleFrameAt(f_cnt_t index) noexcept -> SampleFrame&
		requires (std::is_same_v<T, float> && channelCount == 2)
	{
		assert(index < this->m_frames);
		return reinterpret_cast<SampleFrame*>(this->m_data)[index];
	}

	auto sampleFrameAt(f_cnt_t index) const noexcept -> const SampleFrame&
		requires (std::is_same_v<T, const float> && channelCount == 2)
	{
		assert(index < this->m_frames);
		return reinterpret_cast<const SampleFrame*>(this->m_data)[index];
	}

	auto asSampleFrames() noexcept -> std::span<SampleFrame>
		requires (std::is_same_v<T, float> && channelCount == 2)
	{
		return {reinterpret_cast<SampleFrame*>(this->m_data), static_cast<std::size_t>(this->m_frames)};
	}

	auto asSampleFrames() const noexcept -> std::span<const SampleFrame>
		requires (std::is_same_v<T, const float> && channelCount == 2)
	{
		return {reinterpret_cast<const SampleFrame*>(this->m_data), static_cast<std::size_t>(this->m_frames)};
	}

	/**
	 * @returns a new view with an equal or smaller frame count
	 * @pre newFrames <= frames()
	 */
	constexpr auto first(f_cnt_t newFrames) const noexcept -> InterleavedBufferSpan<T, channelCount>
	{
		assert(newFrames <= this->m_frames);
		if constexpr (channelCount == DynamicChannelCount)
		{
			return {this->m_data, this->m_channels, newFrames};
		}
		else { return InterleavedBufferSpan<T, channelCount>{this->m_data, newFrames}; }
	}
};

// Check that the std::span-like space optimization works
static_assert(sizeof(InterleavedBufferSpan<float>) > sizeof(InterleavedBufferSpan<float, 2>));
static_assert(sizeof(InterleavedBufferSpan<float, 2>) == sizeof(void*) + sizeof(f_cnt_t));

// Deduction guides
template<typename T> InterleavedBufferSpan(T*, ch_cnt_t, f_cnt_t) -> InterleavedBufferSpan<T>;
InterleavedBufferSpan(const SampleFrame*, f_cnt_t) -> InterleavedBufferSpan<const float, 2>;
InterleavedBufferSpan(SampleFrame*, f_cnt_t) -> InterleavedBufferSpan<float, 2>;
InterleavedBufferSpan(std::span<const SampleFrame>) -> InterleavedBufferSpan<const float, 2>;
InterleavedBufferSpan(std::span<SampleFrame>) -> InterleavedBufferSpan<float, 2>;


/**
 * Non-owning view for multi-channel non-interleaved audio data
 *
 * The data type is `T* const*` which is a 2D array accessed as data[channel][frame index]
 * where each channel's buffer contains `frames()` frames.
 *
 * TODO C++23: Use std::mdspan?
 */
template<SampleType T, ch_cnt_t channelCount = DynamicChannelCount>
class PlanarBufferView : public detail::BufferViewData<T* const, channelCount>
{
	using Base = detail::BufferViewData<T* const, channelCount>;

public:
	using Base::Base;

	//! Construct const from mutable (static channel count)
	template<typename U = T> requires (std::is_const_v<U> && channelCount != DynamicChannelCount)
	constexpr PlanarBufferView(PlanarBufferView<std::remove_const_t<U>, channelCount> other) noexcept
		: Base{other.data(), other.frames()}
	{
	}

	//! Construct const from mutable (dynamic channel count)
	template<typename U = T> requires (std::is_const_v<U> && channelCount == DynamicChannelCount)
	constexpr PlanarBufferView(PlanarBufferView<std::remove_const_t<U>, channelCount> other) noexcept
		: Base{other.data(), other.channels(), other.frames()}
	{
	}

	//! Construct dynamic channel count from static
	template<ch_cnt_t otherChannels>
		requires (channelCount == DynamicChannelCount && otherChannels != DynamicChannelCount)
	constexpr PlanarBufferView(PlanarBufferView<T, otherChannels> other) noexcept
		: Base{other.data(), otherChannels, other.frames()}
	{
	}

	constexpr auto empty() const noexcept -> bool
	{
		return !this->m_data || Base::channels() == 0 || this->m_frames == 0;
	}

	//! @return the sample at the given channel and frame indicies
	constexpr auto sample(ch_cnt_t channel, f_cnt_t frame) const noexcept -> T&
	{
		return bufferPtr(channel)[frame];
	}

	//! @return the buffer of the given channel
	constexpr auto buffer(ch_cnt_t channel) const noexcept -> std::span<T>
	{
		return {bufferPtr(channel), static_cast<std::size_t>(this->m_frames)};
	}

	//! @return the buffer of the given channel
	template<ch_cnt_t channel> requires (channelCount != DynamicChannelCount)
	constexpr auto buffer() const noexcept -> std::span<T>
	{
		return {bufferPtr<channel>(), static_cast<std::size_t>(this->m_frames)};
	}

	/**
	 * @return pointer to the buffer of the given channel.
	 * The size of the buffer is `frames()`.
	 */
	constexpr auto bufferPtr(ch_cnt_t channel) const noexcept -> T*
	{
		assert(channel < Base::channels());
		assert(this->m_data != nullptr);
		return this->m_data[channel];
	}

	/**
	 * @return pointer to the buffer of the given channel.
	 * The size of the buffer is `frames()`.
	 */
	template<ch_cnt_t channel> requires (channelCount != DynamicChannelCount)
	constexpr auto bufferPtr() const noexcept -> T*
	{
		static_assert(channel < channelCount);
		assert(this->m_data != nullptr);
		return this->m_data[channel];
	}

	/**
	 * @return pointer to the buffer of a given channel.
	 * The size of the buffer is `frames()`.
	 */
	constexpr auto operator[](ch_cnt_t channel) const noexcept -> T*
	{
		return bufferPtr(channel);
	}

	/**
	 * @returns a new view with an equal or smaller frame count
	 * @pre newFrames <= frames()
	 */
	constexpr auto first(f_cnt_t newFrames) const noexcept -> PlanarBufferView<T, channelCount>
	{
		assert(newFrames <= this->m_frames);
		if constexpr (channelCount == DynamicChannelCount)
		{
			return {this->m_data, this->m_channels, newFrames};
		}
		else { return PlanarBufferView<T, channelCount>{this->m_data, newFrames}; }
	}

	/**
	 * @returns a new view with a subset of this view's channel buffers
	 * @param start the new first channel
	 * @param count the number of channels following @p start
	 * @pre !empty()
	 * @pre start <= channels()
	 * @pre count <= channels() - start
	 */
	constexpr auto channelSubset(ch_cnt_t start, ch_cnt_t count) const noexcept -> PlanarBufferView<T>
	{
		assert(!empty());
		assert(start <= Base::channels());
		assert(count <= Base::channels() - start);
		return {this->m_data + start, count, this->m_frames};
	}

	/**
	 * @returns a new view with a subset of this view's channel buffers
	 * @param start the new first channel
	 * @pre !empty()
	 * @pre start <= channels()
	 */
	constexpr auto channelSubset(ch_cnt_t start) const noexcept -> PlanarBufferView<T>
	{
		assert(!empty());
		assert(start <= Base::channels());
		return {this->m_data + start, Base::channels() - start, this->m_frames};
	}

	/**
	 * @returns a new view with a subset of this view's channel buffers
	 * @tparam start the new first channel
	 * @tparam count the number of channels following @p start
	 * @pre !empty()
	 * @pre start <= channels()
	 * @pre count <= channels() - start
	 */
	template<ch_cnt_t start, ch_cnt_t count = channelCount - start>
		requires (channelCount != DynamicChannelCount)
	constexpr auto channelSubset() const noexcept -> PlanarBufferView<T, count>
	{
		assert(!empty());
		static_assert(start <= channelCount);
		static_assert(count <= channelCount - start);
		return PlanarBufferView<T, count>{this->m_data + start, this->m_frames};
	}
};

// Check that the std::span-like space optimization works
static_assert(sizeof(PlanarBufferView<float>) > sizeof(PlanarBufferView<float, 2>));
static_assert(sizeof(PlanarBufferView<float, 2>) == sizeof(void**) + sizeof(f_cnt_t));

// Deduction guides
template<typename T> PlanarBufferView(T**, ch_cnt_t, f_cnt_t) -> PlanarBufferView<T>;
template<typename T> PlanarBufferView(T* const*, ch_cnt_t, f_cnt_t) -> PlanarBufferView<T>;


//! A @ref PlanarBufferView with a frame offset which allows @a subspan functionality
template<SampleType T, ch_cnt_t channelCount = DynamicChannelCount>
class PlanarBufferSpan : private PlanarBufferView<T, channelCount>
{
	using Base = PlanarBufferView<T, channelCount>;
	using SuperBase = detail::BufferViewData<T* const, channelCount>;
	struct Private {};

public:
	using Base::Base;

	//! Construct from PlanarBufferView
	template<typename U> requires (std::is_convertible_v<U&, T&>)
	constexpr PlanarBufferSpan(PlanarBufferView<U, channelCount> view) noexcept
		: Base{view.data(), view.channels(), view.frames()}
	{
	}

	//! Construct from PlanarBufferView + start
	template<typename U> requires (std::is_convertible_v<U&, T&>)
	constexpr PlanarBufferSpan(PlanarBufferView<U, channelCount> view, f_cnt_t start) noexcept
		: Base{view.data(), view.channels(), view.frames() - start}
		, m_offset{start}
	{
		assert(start <= view.frames());
	}

	//! Construct from PlanarBufferView + start + count
	template<typename U> requires (std::is_convertible_v<U&, T&>)
	constexpr PlanarBufferSpan(PlanarBufferView<U, channelCount> view, f_cnt_t start, f_cnt_t count) noexcept
		: Base{view.data(), view.channels(), count}
		, m_offset{start}
	{
		assert(start <= view.frames());
		assert(count <= view.frames() - start);
	}

	//! Construct dynamic channel count from static PlanarBufferView
	template<ch_cnt_t otherChannels>
		requires (channelCount == DynamicChannelCount && otherChannels != DynamicChannelCount)
	constexpr PlanarBufferSpan(PlanarBufferView<T, otherChannels> other) noexcept
		: Base{other.data(), otherChannels, other.frames()}
		, m_offset{0}
	{
	}

	//! Construct from raw data
	constexpr PlanarBufferSpan(T* const* data, ch_cnt_t channels, f_cnt_t frames, f_cnt_t start) noexcept
		: Base{data, channels, frames - start}
		, m_offset{start}
	{
		assert(start <= frames);
	}

	//! Copy construct
	template<typename U> requires (std::is_convertible_v<U&, T&>)
	constexpr PlanarBufferSpan(PlanarBufferSpan<U, channelCount> other) noexcept
		: Base{other.data(), other.channels(), other.frames()}
		, m_offset{other.m_offset}
	{
	}

	//! Construct dynamic channel count from static
	template<ch_cnt_t otherChannels>
		requires (channelCount == DynamicChannelCount && otherChannels != DynamicChannelCount)
	constexpr PlanarBufferSpan(PlanarBufferSpan<T, otherChannels> other) noexcept
		: Base{other.data(), otherChannels, other.frames()}
		, m_offset{other.m_offset}
	{
	}

	// For converting constructors, to avoid needing to provide an m_offset getter
	template<SampleType, ch_cnt_t>
	friend class PlanarBufferSpan;

private:
	//! Private constructor (static channel count)
	constexpr PlanarBufferSpan(Private, T* const* data, f_cnt_t frames, f_cnt_t offset) noexcept
		requires (channelCount != DynamicChannelCount)
		: Base{data, frames}
		, m_offset{offset}
	{
	}

	//! Private constructor (dynamic channel count)
	constexpr PlanarBufferSpan(Private, T* const* data, ch_cnt_t channels, f_cnt_t frames, f_cnt_t offset) noexcept
		requires (channelCount == DynamicChannelCount)
		: Base{data, channels, frames}
		, m_offset{offset}
	{
	}

public:
	using SuperBase::channels;
	using SuperBase::frames;
	using Base::empty;
	// NOTE: Deliberately missing data()

	//! @return the sample at the given channel and frame indicies
	constexpr auto sample(ch_cnt_t channel, f_cnt_t frame) const noexcept -> T&
	{
		return bufferPtr(channel)[frame];
	}

	//! @return the buffer of the given channel
	constexpr auto buffer(ch_cnt_t channel) const noexcept -> std::span<T>
	{
		return {bufferPtr(channel), static_cast<std::size_t>(this->m_frames)};
	}

	//! @return the buffer of the given channel
	template<ch_cnt_t channel> requires (channelCount != DynamicChannelCount)
	constexpr auto buffer() const noexcept -> std::span<T>
	{
		return {bufferPtr<channel>(), static_cast<std::size_t>(this->m_frames)};
	}

	/**
	 * @return pointer to the buffer of the given channel.
	 * The size of the buffer is `frames()`.
	 */
	constexpr auto bufferPtr(ch_cnt_t channel) const noexcept -> T*
	{
		assert(channel < Base::channels());
		assert(this->m_data != nullptr);
		return this->m_data[channel] + m_offset;
	}

	/**
	 * @return pointer to the buffer of the given channel.
	 * The size of the buffer is `frames()`.
	 */
	template<ch_cnt_t channel> requires (channelCount != DynamicChannelCount)
	constexpr auto bufferPtr() const noexcept -> T*
	{
		static_assert(channel < channelCount);
		assert(this->m_data != nullptr);
		return this->m_data[channel] + m_offset;
	}

	/**
	 * @return pointer to the buffer of a given channel.
	 * The size of the buffer is `frames()`.
	 */
	constexpr auto operator[](ch_cnt_t channel) const noexcept -> T*
	{
		return bufferPtr(channel);
	}

	/**
	 * @returns a new span with an equal or smaller frame count
	 * @pre newFrames <= frames()
	 */
	constexpr auto first(f_cnt_t newFrames) const noexcept -> PlanarBufferSpan<T, channelCount>
	{
		assert(newFrames <= this->m_frames);
		if constexpr (channelCount == DynamicChannelCount)
		{
			return {Private{}, this->m_data, Base::channels(), newFrames, m_offset};
		}
		else
		{
			return {Private{}, this->m_data, newFrames, m_offset};
		}
	}

	/**
	 * @returns a new span with a subset of this span's channel buffers
	 * @param start the new first channel
	 * @param count the number of channels following @p start
	 * @pre !empty()
	 * @pre start <= channels()
	 * @pre count <= channels() - start
	 */
	constexpr auto channelSubset(ch_cnt_t start, ch_cnt_t count) const noexcept -> PlanarBufferSpan<T>
	{
		assert(!empty());
		assert(start <= Base::channels());
		assert(count <= Base::channels() - start);
		return {Private{}, this->m_data + start, count, this->m_frames, m_offset};
	}

	/**
	 * @returns a new span with a subset of this span's channel buffers
	 * @param start the new first channel
	 * @pre !empty()
	 * @pre start <= channels()
	 */
	constexpr auto channelSubset(ch_cnt_t start) const noexcept -> PlanarBufferSpan<T>
	{
		assert(!empty());
		assert(start <= Base::channels());
		return {Private{}, this->m_data + start, Base::channels() - start, this->m_frames, m_offset};
	}

	/**
	 * @returns a new span with a subset of this span's channel buffers
	 * @tparam start the new first channel
	 * @tparam count the number of channels following @p start
	 * @pre !empty()
	 * @pre start <= channels()
	 * @pre count <= channels() - start
	 */
	template<ch_cnt_t start, ch_cnt_t count = channelCount - start>
		requires (channelCount != DynamicChannelCount)
	constexpr auto channelSubset() const noexcept -> PlanarBufferSpan<T, count>
	{
		assert(!empty());
		static_assert(start <= channelCount);
		static_assert(count <= channelCount - start);

		return {Private{}, this->m_data + start, this->m_frames, m_offset};
	}

	/**
	 * @returns a new span with a subset of this span's frames
	 * @param start the new first frame
	 * @pre !empty()
	 * @pre start <= frames()
	 */
	constexpr auto subspan(f_cnt_t start) const noexcept -> PlanarBufferSpan<T, channelCount>
	{
		assert(!empty());
		assert(start <= this->m_frames);
		if constexpr (channelCount == DynamicChannelCount)
		{
			return {Private{}, this->m_data, this->m_channels, this->m_frames - start, m_offset + start};
		}
		else
		{
			return {Private{}, this->m_data, this->m_frames - start, m_offset + start};
		}
	}

	/**
	 * @returns a new span with a subset of this span's frames
	 * @param start the new first frame
	 * @param count the number of frames following @p start
	 * @pre !empty()
	 * @pre start <= frames()
	 * @pre count <= frames() - start
	 */
	constexpr auto subspan(f_cnt_t start, f_cnt_t count) const noexcept -> PlanarBufferSpan<T, channelCount>
	{
		assert(!empty());
		assert(start <= this->m_frames);
		assert(count <= this->m_frames - start);
		if constexpr (channelCount == DynamicChannelCount)
		{
			return {Private{}, this->m_data, this->m_channels, count, m_offset + start};
		}
		else
		{
			return {Private{}, this->m_data, count, m_offset + start};
		}
	}

private:
	f_cnt_t m_offset = 0;
};

// Check that the std::span-like space optimization works
static_assert(sizeof(PlanarBufferSpan<float>) > sizeof(PlanarBufferSpan<float, 2>));
static_assert(sizeof(PlanarBufferSpan<float, 2>) == sizeof(void**) + sizeof(f_cnt_t) * 2);

// Deduction guides
template<typename T> PlanarBufferSpan(T**, ch_cnt_t, f_cnt_t) -> PlanarBufferSpan<T>;
template<typename T> PlanarBufferSpan(T* const*, ch_cnt_t, f_cnt_t) -> PlanarBufferSpan<T>;
template<typename T, ch_cnt_t ch> PlanarBufferSpan(PlanarBufferView<T, ch>) -> PlanarBufferSpan<T, ch>;
template<typename T, ch_cnt_t ch> PlanarBufferSpan(PlanarBufferView<T, ch>, f_cnt_t) -> PlanarBufferSpan<T, ch>;
template<typename T, ch_cnt_t ch> PlanarBufferSpan(PlanarBufferView<T, ch>, f_cnt_t, f_cnt_t) -> PlanarBufferSpan<T>;
template<typename T> PlanarBufferSpan(T* const*, ch_cnt_t, f_cnt_t, f_cnt_t) -> PlanarBufferSpan<T>;


//! Concept for any audio buffer view, interleaved or planar
template<class T, typename U, ch_cnt_t channels = DynamicChannelCount>
concept AudioBufferSpan = SampleType<U> && (std::convertible_to<T, InterleavedBufferSpan<U, channels>>
	|| std::convertible_to<T, PlanarBufferSpan<U, channels>>);


//! Converts planar buffers to interleaved buffers
template<class T, ch_cnt_t inputs, ch_cnt_t outputs>
constexpr void toInterleaved(PlanarBufferView<T, inputs> src,
	InterleavedBufferSpan<std::remove_const_t<T>, outputs> dst)
{
	assert(src.frames() == dst.frames());
	if constexpr (inputs == DynamicChannelCount || outputs == DynamicChannelCount)
	{
		assert(src.channels() == dst.channels());
	}
	else { static_assert(inputs == outputs); }

	for (f_cnt_t frame = 0; frame < dst.frames(); ++frame)
	{
		auto* framePtr = dst.framePtr(frame);
		for (ch_cnt_t channel = 0; channel < dst.channels(); ++channel)
		{
			framePtr[channel] = src.bufferPtr(channel)[frame];
		}
	}
}

//! Converts interleaved buffers to planar buffers
template<class T, ch_cnt_t inputs, ch_cnt_t outputs>
constexpr void toPlanar(InterleavedBufferSpan<T, inputs> src,
	PlanarBufferView<std::remove_const_t<T>, outputs> dst)
{
	assert(src.frames() == dst.frames());
	if constexpr (inputs == DynamicChannelCount || outputs == DynamicChannelCount)
	{
		assert(src.channels() == dst.channels());
	}
	else { static_assert(inputs == outputs); }

	for (ch_cnt_t channel = 0; channel < dst.channels(); ++channel)
	{
		auto* channelPtr = dst.bufferPtr(channel);
		for (f_cnt_t frame = 0; frame < dst.frames(); ++frame)
		{
			channelPtr[frame] = src.framePtr(frame)[channel];
		}
	}
}

} // namespace lmms

#endif // LMMS_AUDIO_BUFFER_SPAN_H
