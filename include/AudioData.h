/*
 * AudioData.h - Audio data types
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

#ifndef LMMS_AUDIO_DATA_H
#define LMMS_AUDIO_DATA_H

#include <type_traits>

#include "lmms_basics.h"

namespace lmms
{

//! Conventions for passing audio data
enum class AudioDataLayout
{
	/*
	 * Given:
	 *   - N == Frame count
	 *   - C == Number of channels
	 *   - i == Sample index, where 0 <= i < N
	 *   - `samples` has the type sample_t*
	 *   - `samples` size == N * C
	 */

	/**
	 * Layout where the samples for each channel are interleaved.
	 * i.e. "LRLRLRLR"
	 *
	 * Samples for individual channels can be accessed like this:
	 * - Channel #0 samples: samples[C*i]
	 * - Channel #1 samples: samples[C*i + 1]
	 * - Channel #2 samples: samples[C*i + 2]
	 * - Channel #3 samples: samples[C*i + 3]
	 * - ...
	 */
	Interleaved,

	/**
	 * Layout where all samples for a particular channel are grouped together.
	 * i.e. "LLLLRRRR"
	 *
	 * Samples for individual channels can be accessed like this:
	 * - Channel #0 samples: samples[i]
	 * - Channel #1 samples: samples[1*N + i]
	 * - Channel #2 samples: samples[2*N + i]
	 * - Channel #3 samples: samples[3*N + i]
	 * - ...
	 */
	Split
};


namespace detail {

//! Based on BOOST_STRONG_TYPEDEF
template<auto tag, typename T>
struct alignas(T) StrongTypedef
{
public:
	using TagType = decltype(tag);
	using Type = T;

	static_assert(std::is_enum_v<TagType> || std::is_integral_v<TagType>);
	static_assert(!std::is_const_v<Type>, "Use `const StrongTypedef<T>` instead of `StrongTypedef<const T>`");
	static_assert(std::is_trivially_copyable_v<Type>);

	static constexpr TagType Tag = tag;

	constexpr StrongTypedef() = default;
	constexpr /* implicit */ StrongTypedef(Type value) : m_t{value} {};
	constexpr StrongTypedef(const StrongTypedef& value) = default;

	constexpr operator const Type&() const { return m_t; }
	constexpr operator Type&() { return m_t; }

	constexpr auto operator==(StrongTypedef rhs) const noexcept -> bool { return m_t == rhs.m_t; }
	constexpr auto operator<(StrongTypedef rhs) const noexcept -> bool { return m_t < rhs.m_t; }

private:
	Type m_t;
};

static_assert(std::is_trivially_copyable_v<StrongTypedef<123, float>>,
	"Trivially copyable for no performance penalty");
static_assert(sizeof(StrongTypedef<123, float>) == sizeof(float),
	"Strong typedef must be same size as the type it wraps");
static_assert(alignof(StrongTypedef<123, float>) == alignof(float),
	"Strong typedef must have same alignment as the type it wraps");

//! If `T` is const, returns `const U`, else returns `U`
template<typename T, typename U>
using copy_const_t = std::conditional_t<std::is_const_v<T>, std::add_const_t<U>, U>;

} // namespace detail


/**
 * A strongly typed fundamental data type for audio that ensures
 * the audio data layout is respected.
 *
 * Use `const SampleType<T>` for immutable data rather than `SampleType<const T>`.
 * `const SampleType<T>*` and `SampleType<T>*` are intended as substitutes for `const T*` and `T*` respectively.
 */
template<AudioDataLayout layout, typename T, std::enable_if_t<std::is_arithmetic_v<T>, bool> = true>
using SampleType = detail::StrongTypedef<layout, T>;

template<typename T>
using SplitSampleType = SampleType<AudioDataLayout::Split, T>;

template<typename T>
using InterleavedSampleType = SampleType<AudioDataLayout::Interleaved, T>;


namespace detail {

//! Metafunction that converts a raw audio data type to a `SampleType` equivalent
template<AudioDataLayout layout, typename T, std::enable_if_t<std::is_arithmetic_v<T>, bool> = true>
using wrap_sample_t = detail::copy_const_t<T, SampleType<layout, std::remove_const_t<T>>>;

template<class T>
struct unwrap_sample;

template<AudioDataLayout layout, class U>
struct unwrap_sample<SampleType<layout, U>>
{
	using type = U;
};

template<AudioDataLayout layout, class U>
struct unwrap_sample<const SampleType<layout, U>>
{
	using type = const U;
};

//! Metafunction that converts `SampleType` to the corresponding raw audio data type
template<typename T>
using unwrap_sample_t = typename unwrap_sample<T>::type;

} // namespace detail


/**
 * A span for storing audio data of a particular layout.
 *
 * All data is contiguous in memory.
 * The size should be equal to the frame count * the channel count.
 */
template<AudioDataLayout layout, typename T>
using AudioBuffer = Span<detail::wrap_sample_t<layout, T>>;

template<typename T>
using SplitAudioBuffer = AudioBuffer<AudioDataLayout::Split, T>;

template<typename T>
using InterleavedAudioBuffer = AudioBuffer<AudioDataLayout::Interleaved, T>;


//! Cast a raw audio samples pointer (i.e. `float*`) to `SampleType*`
template<typename T, typename S = detail::unwrap_sample_t<std::remove_pointer_t<T>>>
inline auto audio_cast(S* samples1) -> T
{
	static_assert(std::is_pointer_v<T>, "T must be a pointer");
	return reinterpret_cast<T>(samples1);
}

//! Cast `SampleType*` to a raw audio samples pointer (i.e. `float*`)
template<typename T, AudioDataLayout layout>
inline auto audio_cast(SampleType<layout, std::remove_pointer_t<T>>* samples) -> T
{
	static_assert(std::is_pointer_v<T>, "T must be a pointer");
	return reinterpret_cast<T>(samples);
}

//! Cast `const SampleType*` to a raw audio samples pointer (i.e. `const float*`)
template<typename T, AudioDataLayout layout>
inline auto audio_cast(const SampleType<layout, std::remove_const_t<std::remove_pointer_t<T>>>* samples) -> T
{
	static_assert(std::is_pointer_v<T>, "T must be a pointer");
	static_assert(std::is_const_v<std::remove_pointer_t<T>>, "Cannot remove const");
	return reinterpret_cast<T>(samples);
}


} // namespace lmms

#endif // LMMS_AUDIO_DATA_H
