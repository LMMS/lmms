/*
 * AudioPortsConfig.h - Audio ports configuration
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

#ifndef LMMS_AUDIO_PORTS_CONFIG_H
#define LMMS_AUDIO_PORTS_CONFIG_H

#include "AudioData.h"

namespace lmms {

//! Compile time customizations for audio ports
struct AudioPortsConfig
{
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
	 *   will only need a single in/out buffer parameter rather than two separate parameters.
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
	 * NOTE: Regardless of whether `inplace` is true or false, if `buffered` is true, the process method
	 *   will have no buffer parameters.
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
	 *   For processors that manage their own buffers, passing those buffers to itself through
	 *   the process method is pointless, so this option also removes the buffer parameter(s)
	 *   from the process method. TODO: Remove this feature/quirk?
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

	constexpr auto operator==(const AudioPortsConfig& rhs) const -> bool = default;
};

} // namespace lmms

#endif // LMMS_AUDIO_PORTS_CONFIG_H
