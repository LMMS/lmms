/*
 * AudioPortsSettings.h - Audio ports compile-time settings
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

#ifndef LMMS_AUDIO_PORTS_SETTINGS_H
#define LMMS_AUDIO_PORTS_SETTINGS_H

#include "LmmsPolyfill.h"
#include "LmmsTypes.h"

namespace lmms {

//! Types of audio data supported by audio ports
enum class AudioDataKind : std::uint8_t
{
	F32,
	// F64,
	// I16,
	// etc.
};


namespace detail {

//! Specialize this struct to enable the use of an audio data kind
template<AudioDataKind kind>
struct GetAudioDataTypeHelper
{
	static_assert(always_false_v<GetAudioDataTypeHelper<kind>>, "Unsupported audio data kind");
};

template<>
struct GetAudioDataTypeHelper<AudioDataKind::F32> { using type = float; };

} // namespace detail


//! Metafunction to convert `AudioDataKind` to its type
template<AudioDataKind kind>
using GetAudioDataType = typename detail::GetAudioDataTypeHelper<kind>::type;


//! Compile-time customizations for audio ports
struct AudioPortsSettings
{
	/**
	 * NOTE: The following five members (`kind`, `interleaved`, `inputs`, `outputs`, and `inplace`)
	 *       determine the signature of the process method that will need to be implemented.
	 *
	 * For an `Effect` or a MIDI-based `Instrument`, the signature will one of two options:
	 *     ProcessStatus processImpl(InputBuffers in, OutputBuffers out);
	 * Or:
	 *     ProcessStatus processImpl(InputOutputBuffers inOut);
	 *
	 * Explanation:
	 * - If `inplace == false`, the 1st option (separate in and output buffers) is used, otherwise the
	 *     2nd option (combined in/out buffers) is used. The channel count when `inplace == true`
	 *     is `std::max(inputs, outputs)`.
	 * - If `kind == AudioDataKind::F32`, `float` samples are used for the input and output buffers.
	 * - If `interleaved == false`, `PlanarBufferView` is used for the input and output buffers,
	 *     otherwise `InterleavedBufferView` is used.
	 * - The `DynamicChannelCount` constant works the same way that `std::dynamic_extent` does
	 *     for `std::span`, so when the channel count is not explicitly specified in `PlanarBufferView` or
	 *     `InterleavedBufferView`, it is `DynamicChannelCount` by default.
	 *
	 * Here are a couple examples:
	 *
	 * - When `kind == AudioDataKind::F32`, `interleaved == false`, `inputs == 3`, `outputs == DynamicChannelCount`,
	 *   and `inplace == false`, then the `processImpl` signature is:
	 *
	 *       `ProcessStatus processImpl(PlanarBufferView<const float, 3> in, PlanarBufferView<float> out);`
	 *
	 *     (Note that the input is always const, and just like `std::span` the `PlanarBufferView` output buffers
	 *      have a dynamic channel count by default since the count is not explicitly specified.)
	 *
	 * - When `kind == AudioDataKind::F32`, `interleaved == true`, `inputs == 2`, `outputs == 0`,
	 *   and `inplace == true`, then the `processImpl` signature is:
	 *
	 *       `ProcessStatus processImpl(InterleavedBufferView<float, 2> inOut);`
	 *
	 *     (Note that when `inplace == true`, the channel count is `std::max(inputs, outputs)` and the channel counts
	 *      are not allowed to differ unless one of them is zero. This cannot be an instrument because the output
	 *      count is zero.)
	 *
	 * Also keep in mind that if you are unsure whether your desired `AudioPortsSettings` is valid, you may check it
	 *   by calling `validate<settings>()`.
	 */

	//! The audio data type used by the processor
	AudioDataKind kind;

	//! The audio data layout: interleaved or non-interleaved
	bool interleaved;

	//! The number of input channels, or `DynamicChannelCount` if unknown at compile time
	proc_ch_t inputs = DynamicChannelCount;

	//! The number of output channels, or `DynamicChannelCount` if unknown at compile time
	proc_ch_t outputs = DynamicChannelCount;

	/**
	 * In-place processing
	 *
	 * When true, the processor always uses in-place processing and its process method
	 *   will have a single in/out buffer parameter rather than two separate parameters.
	 *
	 * When false, the processor may be dynamically in-place if the audio buffer implementation
	 *   supports it, otherwise the processor never uses in-place processing.
	 *
	 *   In order to support dynamically in-place processing, an audio buffer implementation can
	 *   return the same buffer for both its input buffer and its output buffer. Any processors
	 *   using such a buffer implementation must design their process method to check whether the
	 *   input and output buffers are the same buffer (indicating in-place processing is active),
	 *   and safely handle both possibilities.
	 *
	 * TODO: Once `RemotePluginAudioPorts` supports statically in-place processing, replace this setting with
	 *       `forceInplace` and use statically in-place processing when `forceInplace || inputs == 0 || outputs == 0`.
	 */
	bool inplace = false;

	/**
	 * Audio buffer usage
	 *
	 * When true, the processor's audio buffers are always written to and read from when routing
	 *   to and from the processor. This lessens the extent to which the audio port can apply the
	 *   "direct routing" optimization, but some processors with custom audio buffers (such as Vestige)
	 *   have no choice because they require their buffers to always be written to and read from.
	 *
	 * When false, the processor's audio buffers may not always be written to or read from when routing
	 *   to and from the processor. This can allow for better performance from the "direct routing"
	 *   optimization, though it may not be suitable for all processor implementations.
	 */
	bool buffered = true;


	//! Whether channel counts are known at compile time
	constexpr auto staticChannelCount() const -> bool
	{
		return inputs != DynamicChannelCount && outputs != DynamicChannelCount;
	}

	/**
	 * Whether the audio port buffers can be converted to/from
	 * `std::span<SampleFrame>` or `InterleavedBufferView<float, 2>`
	 */
	constexpr auto sampleFrameCompatible() const -> bool
	{
		return kind == AudioDataKind::F32
			&& interleaved && inplace
			&& (inputs == 0 || inputs == 2)
			&& (outputs == 0 || outputs == 2);
	}

	constexpr auto operator==(const AudioPortsSettings& rhs) const -> bool = default;
};

/**
 * Checks that an `AudioPortsSettings` is valid.
 * Intended for use in static_assert().
 */
template<AudioPortsSettings settings>
constexpr auto validate() -> bool
{
	static_assert(settings.inputs != 0 || settings.outputs != 0, "The input and output counts cannot both be zero");

	static_assert(!settings.interleaved
		|| ((settings.inputs == 0 || settings.inputs == 2) && (settings.outputs == 0 || settings.outputs == 2)),
		"AudioPortsSettings: When using interleaved samples, there must be exactly 0 or 2 input and output channels");

	static_assert(!settings.interleaved || settings.kind == AudioDataKind::F32,
		"AudioPortsSettings: Interleaved samples must be float");

	static_assert(!settings.interleaved || settings.inplace,
		"AudioPortsSettings: Interleaved samples must use in-place processing");

	static_assert(!settings.inplace || settings.inputs == settings.outputs
		|| settings.inputs == 0 || settings.outputs == 0,
		"AudioPortsSettings: In-place processors must have equivalent input/output counts, or one must be zero.");

	return true;
}

} // namespace lmms

#endif // LMMS_AUDIO_PORTS_SETTINGS_H
