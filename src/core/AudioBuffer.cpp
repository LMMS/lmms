/*
 * AudioBuffer.cpp
 *
 * Copyright (c) 2026 Dalton Messmer <messmer.dalton/at/gmail.com>
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

#include "AudioBuffer.h"

#include "ConfigManager.h"
#include "MixHelpers.h"

namespace lmms
{

namespace
{

//! @returns Bitset with all bits at or above `pos` set to `value` and the rest set to `!value`
template<bool value>
auto createMask(ch_cnt_t pos) noexcept -> AudioBuffer::ChannelFlags
{
	assert(pos <= MaxChannelsPerAudioBuffer);

	AudioBuffer::ChannelFlags mask;
	mask.set();

	if constexpr (value)
	{
		mask <<= pos;
	}
	else
	{
		mask >>= (MaxChannelsPerAudioBuffer - pos);
	}

	return mask;
}

} // namespace


AudioBuffer::AudioBuffer(f_cnt_t frames, ch_cnt_t channels,
	std::pmr::memory_resource* bufferResource)
	: m_sourceBuffer{bufferResource}
	, m_channelBuffers{bufferResource}
	, m_interleavedBuffer{bufferResource}
	, m_frames{frames}
	, m_silenceTrackingEnabled{ConfigManager::inst()->value("ui", "disableautoquit", "1").toInt() == 0}
{
	m_interleavedBuffer.resize(2 * frames);

	if (channels == 0)
	{
		m_silenceFlags.set();
		return;
	}

	if (!addGroup(channels))
	{
		throw std::runtime_error{"failed to add group"};
	}
}

auto AudioBuffer::getAllocatedSize(f_cnt_t frames, ch_cnt_t channels) -> std::size_t
{
	return frames * channels * sizeof(float) // for m_sourceBuffer
		+ channels * sizeof(float*) // for m_channelBuffers
		+ frames * 2 * sizeof(float); // for m_interleavedBuffer
}

auto AudioBuffer::addGroup(ch_cnt_t channels) -> ChannelGroup*
{
	if (m_groups.size() >= m_groups.capacity())
	{
		// Maximum groups reached
		return nullptr;
	}

	if (channels == 0)
	{
		// Invalid channel count for a group
		return nullptr;
	}

	const auto oldTotalChannels = totalChannels();
	const auto newTotalChannels = totalChannels() + channels;
	if (newTotalChannels > MaxChannelsPerAudioBuffer)
	{
		// Not enough room for requested channels
		return nullptr;
	}

	// Resize buffers
	m_sourceBuffer.resize(newTotalChannels * m_frames);
	m_channelBuffers.resize(newTotalChannels);

	// Fix channel buffers
	float* ptr = m_sourceBuffer.data();
	ch_cnt_t channel = 0;
	while (channel < newTotalChannels)
	{
		m_channelBuffers[channel] = ptr;

		ptr += m_frames;
		++channel;
	}

	// Fix group buffers
	channel = 0;
	for (ChannelGroup& group : m_groups)
	{
		group.setBuffers(&m_channelBuffers[channel]);
		channel += group.channels();
	}

	// Ensure the new channels (and all the higher, unused
	// channels) are set to "silent"
	m_silenceFlags |= createMask<true>(oldTotalChannels);

	// Append new group
	return &m_groups.emplace_back(&m_channelBuffers[oldTotalChannels], channels);
}

void AudioBuffer::enableSilenceTracking(bool enabled)
{
	const auto oldValue = m_silenceTrackingEnabled;
	m_silenceTrackingEnabled = enabled;
	if (!oldValue && enabled)
	{
		updateAllSilenceFlags();
	}
}

void AudioBuffer::mixSilenceFlags(const AudioBuffer& other)
{
	m_silenceFlags &= other.silenceFlags();
}

auto AudioBuffer::hasSignal(const ChannelFlags& channels) const -> bool
{
	auto nonSilent = ~m_silenceFlags;
	nonSilent &= channels;
	return nonSilent.any();
}

auto AudioBuffer::hasAnySignal() const -> bool
{
	// This is possible due to the invariant that any channel bits
	// at or above `totalChannels()` must always be marked silent
	return !m_silenceFlags.all();
}

void AudioBuffer::sanitize(const ChannelFlags& channels, ch_cnt_t upperBound)
{
	if (!MixHelpers::useNaNHandler()) { return; }

	bool changesMade = false;

	const auto totalChannels = std::min(upperBound, this->totalChannels());
	for (ch_cnt_t ch = 0; ch < totalChannels; ++ch)
	{
		if (channels[ch])
		{
			// This channel needs to be sanitized
			if (MixHelpers::sanitize(buffer(ch)))
			{
				// Inf/NaN detected and buffer cleared
				m_silenceFlags[ch] = true;
				changesMade = true;
			}
		}
	}

	if (changesMade && (channels[0] || channels[1]))
	{
		// Keep the temporary interleaved buffer in sync
		toInterleaved(groupBuffers(0), interleavedBuffer());
	}
}

void AudioBuffer::sanitizeAll()
{
	if (!MixHelpers::useNaNHandler()) { return; }

	bool changesMade = false;
	for (ch_cnt_t ch = 0; ch < totalChannels(); ++ch)
	{
		if (MixHelpers::sanitize(buffer(ch)))
		{
			// Inf/NaN detected and buffer cleared
			m_silenceFlags[ch] = true;
			changesMade = true;
		}
	}

	if (changesMade)
	{
		// Keep the temporary interleaved buffer in sync
		toInterleaved(groupBuffers(0), interleavedBuffer());
	}
}

auto AudioBuffer::updateSilenceFlags(const ChannelFlags& channels, ch_cnt_t upperBound) -> bool
{
	assert(upperBound <= MaxChannelsPerAudioBuffer);

	// Invariant: Any channel bits at or above `totalChannels()` must be marked silent
	assert((~m_silenceFlags & createMask<true>(totalChannels())).none());

	// If no channels are selected, return true (all selected channels are silent)
	if (channels.none()) { return true; }

	const auto totalChannels = std::min(upperBound, this->totalChannels());

	if (!m_silenceTrackingEnabled)
	{
		// Mark specified channels (up to the upper bound) as non-silent
		auto temp = ~channels;
		temp |= createMask<true>(totalChannels);
		m_silenceFlags &= temp;
		return false;
	}

	bool allQuiet = true;
	for (ch_cnt_t ch = 0; ch < totalChannels; ++ch)
	{
		if (channels[ch])
		{
			// This channel needs to be updated
			const auto quiet = MixHelpers::isSilent(buffer(ch));

			m_silenceFlags[ch] = quiet;
			allQuiet = allQuiet && quiet;
		}
	}

	return allQuiet;
}

auto AudioBuffer::updateAllSilenceFlags() -> bool
{
	// Invariant: Any channel bits at or above `totalChannels()` must be marked silent
	assert((~m_silenceFlags & createMask<true>(totalChannels())).none());

	// If there are no channels, return true (all channels are silent)
	if (totalChannels() == 0) { return true; }

	if (!m_silenceTrackingEnabled)
	{
		// Mark all channels below `totalChannels()` as non-silent
		m_silenceFlags &= createMask<true>(totalChannels());
		return false;
	}

	bool allQuiet = true;
	for (ch_cnt_t ch = 0; ch < totalChannels(); ++ch)
	{
		const auto quiet = MixHelpers::isSilent(buffer(ch));

		m_silenceFlags[ch] = quiet;
		allQuiet = allQuiet && quiet;
	}

	return allQuiet;
}

void AudioBuffer::silenceChannels(const ChannelFlags& channels, ch_cnt_t upperBound)
{
	auto needSilenced = ~m_silenceFlags;
	needSilenced &= channels;

	const auto totalChannels = std::min(upperBound, this->totalChannels());
	for (ch_cnt_t ch = 0; ch < totalChannels; ++ch)
	{
		if (needSilenced[ch])
		{
			std::ranges::fill(buffer(ch), 0.f);
		}
	}

	if (needSilenced[0] || needSilenced[1])
	{
		// Keep the temporary interleaved buffer in sync
		toInterleaved(groupBuffers(0), interleavedBuffer());
	}

	m_silenceFlags |= channels;
}

void AudioBuffer::silenceAllChannels()
{
	std::ranges::fill(m_sourceBuffer, 0);
	std::ranges::fill(m_interleavedBuffer, 0);

	m_silenceFlags.set();
}

auto AudioBuffer::absPeakValue(ch_cnt_t channel) const -> float
{
	if (m_silenceFlags[channel])
	{
		// Skip calculation if channel is already known to be silent
		return 0;
	}

	return std::ranges::max(buffer(channel), {}, static_cast<float(&)(float)>(std::abs));
}

} // namespace lmms
