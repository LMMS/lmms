/*
 * TrackChannelContainer.cpp
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

#include "TrackChannelContainer.h"

#include <algorithm>
#include <type_traits>

#include "ConfigManager.h"
#include "LmmsPolyfill.h"
#include "MixHelpers.h"

namespace lmms
{

namespace
{

/*
 * In the past, the RMS was calculated then compared with a threshold of 10^(-10).
 * Now we use a different algorithm to determine whether a buffer is non-quiet, so
 * a new threshold is needed for the best compatibility. The following is how it's derived.
 *
 * Old method:
 * RMS = average (L^2 + R^2) across stereo buffer.
 * RMS threshold = 10^(-10)
 *
 * So for a single channel, it would be:
 * RMS/2 = average M^2 across single channel buffer.
 * RMS/2 threshold = 5^(-11)
 *
 * The new algorithm for determining whether a buffer is non-silent compares M with the threshold,
 * not M^2, so the square root of M^2's threshold should give us the most compatible threshold for
 * the new algorithm:
 *
 * (RMS/2)^0.5 = (5^(-11))^0.5 = 0.0001431 (approx.)
 *
 * In practice though, the exact value shouldn't really matter so long as it's sufficiently small.
 */
constexpr auto SilenceThreshold = 0.0001431f;

//! @returns Bitset with all bits at or above `pos` set to `value` and the rest set to `!value`
template<bool value>
auto createMask(track_ch_t pos) noexcept -> TrackChannelContainer::ChannelFlags
{
	assert(pos <= MaxTrackChannels);

	TrackChannelContainer::ChannelFlags mask;
	mask.set();

	if constexpr (value)
	{
		mask <<= pos;
	}
	else
	{
		mask >>= (MaxTrackChannels - pos);
	}

	return mask;
}

} // namespace


TrackChannelContainer::TrackChannelContainer(f_cnt_t frames, ch_cnt_t channels,
	std::pmr::memory_resource* bufferResource)
	: m_frames{frames}
	, m_alloc{bufferResource}
	, m_silenceTrackingEnabled{ConfigManager::inst()->value("ui", "disableautoquit", "1").toInt() == 0}
{
	m_interleavedBuffer = m_alloc.allocate_object<float>(2 * frames);

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

TrackChannelContainer::~TrackChannelContainer()
{
	if (m_sourceBuffer)
	{
		m_alloc.deallocate_object(m_sourceBuffer, m_totalChannels * m_frames);
	}

	if (m_channelBuffers)
	{
		m_alloc.deallocate_object(m_channelBuffers, m_totalChannels);
	}

	if (m_interleavedBuffer)
	{
		m_alloc.deallocate_object(m_interleavedBuffer, 2 * m_frames);
	}
}

auto TrackChannelContainer::addGroup(ch_cnt_t channels) -> ChannelGroup*
{
	if (m_groups.size() >= m_groups.capacity())
	{
		// Maximum groups reached
		return nullptr;
	}

	if (channels > MaxChannelsPerGroup || channels == 0)
	{
		// Invalid channel count for a group
		return nullptr;
	}

	const auto newTotalChannels = m_totalChannels + channels;
	if (newTotalChannels > MaxTrackChannels)
	{
		// Not enough room for requested track channels
		return nullptr;
	}

	// Allocate new buffers
	auto newSourceBuffer = m_alloc.allocate_object<float>(newTotalChannels * m_frames);
	auto newChannelBuffers = m_alloc.allocate_object<float*>(newTotalChannels);

	// Copy old buffer contents to new buffers
	if (m_sourceBuffer)
	{
		std::copy_n(m_sourceBuffer, m_totalChannels * m_frames, newSourceBuffer);
	}

	// Set new channel buffers to zero (silent)
	std::fill_n(&newSourceBuffer[m_totalChannels * m_frames], (newTotalChannels - m_totalChannels) * m_frames, 0.f);

	// Initialize new channel buffers
	float* ptr = newSourceBuffer;
	track_ch_t channel = 0;
	while (channel < newTotalChannels)
	{
		newChannelBuffers[channel] = ptr;

		ptr += m_frames;
		++channel;
	}

	// Deallocate old buffers
	if (m_sourceBuffer)
	{
		m_alloc.deallocate_object(m_sourceBuffer, m_totalChannels * m_frames);
	}

	if (m_channelBuffers)
	{
		m_alloc.deallocate_object(m_channelBuffers, m_totalChannels);
	}

	// Use new buffers
	m_sourceBuffer = newSourceBuffer;
	m_channelBuffers = newChannelBuffers;

	channel = 0;
	for (ChannelGroup& group : m_groups)
	{
		group.setBuffers(&m_channelBuffers[channel]);
		channel += group.channels();
	}

	// Ensure the new track channels (and all the higher, unused
	// track channels) are set to "silent"
	m_silenceFlags |= createMask<true>(m_totalChannels);

	// Append new group
	auto& newGroup = m_groups.emplace_back(&m_channelBuffers[m_totalChannels], channels);

	m_totalChannels = newTotalChannels;

	return &newGroup;
}

void TrackChannelContainer::enableSilenceTracking(bool enabled)
{
	const auto oldValue = m_silenceTrackingEnabled;
	m_silenceTrackingEnabled = enabled;
	if (!oldValue && enabled)
	{
		updateAllSilenceFlags();
	}
}

void TrackChannelContainer::mixSilenceFlags(const TrackChannelContainer& other)
{
	m_silenceFlags &= other.silenceFlags();
}

auto TrackChannelContainer::hasInputNoise(const ChannelFlags& usedChannels) const -> bool
{
	auto nonSilent = ~m_silenceFlags;
	nonSilent &= usedChannels;
	return nonSilent.any();
}

auto TrackChannelContainer::hasAnyInputNoise() const -> bool
{
	// This is possible due to the invariant that any channel bits
	// at or above `totalChannels()` must always be marked silent
	return !m_silenceFlags.all();
}

void TrackChannelContainer::sanitize(const ChannelFlags& channels, track_ch_t upperBound)
{
	if (!MixHelpers::useNaNHandler()) { return; }

	bool changesMade = false;

	const auto totalChannels = std::min(upperBound, m_totalChannels);
	for (track_ch_t tc = 0; tc < totalChannels; ++tc)
	{
		if (channels[tc])
		{
			// This channel needs to be sanitized
			if (MixHelpers::sanitize(buffer(tc)))
			{
				// Inf/NaN detected and buffer cleared
				m_silenceFlags[tc] = true;
				changesMade = true;
			}
		}
	}

	if (changesMade && (channels[0] || channels[1]))
	{
		// Keep the temporary interleaved buffer in sync
		toInterleaved(buffers(0), interleavedBuffer());
	}
}

void TrackChannelContainer::sanitizeAll()
{
	if (!MixHelpers::useNaNHandler()) { return; }

	bool changesMade = false;
	for (track_ch_t tc = 0; tc < m_totalChannels; ++tc)
	{
		if (MixHelpers::sanitize(buffer(tc)))
		{
			// Inf/NaN detected and buffer cleared
			m_silenceFlags[tc] = true;
			changesMade = true;
		}
	}

	if (changesMade)
	{
		// Keep the temporary interleaved buffer in sync
		toInterleaved(buffers(0), interleavedBuffer());
	}
}

auto TrackChannelContainer::updateSilenceFlags(const ChannelFlags& channels, track_ch_t upperBound) -> bool
{
	assert(upperBound <= MaxTrackChannels);

	// Invariant: Any channel bits at or above `totalChannels()` must be marked silent
	assert((~m_silenceFlags & createMask<true>(m_totalChannels)).none());

	// If no channels are selected, return true (all selected channels are silent)
	if (channels.none()) { return true; }

	const auto totalChannels = std::min(upperBound, m_totalChannels);

	if (!m_silenceTrackingEnabled)
	{
		// Mark specified channels (up to the upper bound) as non-silent
		auto temp = ~channels;
		temp |= createMask<true>(totalChannels);
		m_silenceFlags &= temp;
		return false;
	}

	bool allQuiet = true;
	for (track_ch_t tc = 0; tc < totalChannels; ++tc)
	{
		if (channels[tc])
		{
			// This channel needs to be updated
			const auto quiet = std::ranges::all_of(buffer(tc), [](const float sample) {
				return std::abs(sample) < SilenceThreshold;
			});

			m_silenceFlags[tc] = quiet;
			allQuiet = allQuiet && quiet;
		}
	}

	return allQuiet;
}

auto TrackChannelContainer::updateAllSilenceFlags() -> bool
{
	// Invariant: Any channel bits at or above `totalChannels()` must be marked silent
	assert((~m_silenceFlags & createMask<true>(m_totalChannels)).none());

	// If there are no channels, return true (all channels are silent)
	if (m_totalChannels == 0) { return true; }

	if (!m_silenceTrackingEnabled)
	{
		// Mark all channels below `totalChannels()` as non-silent
		m_silenceFlags &= createMask<true>(m_totalChannels);
		return false;
	}

	bool allQuiet = true;
	for (track_ch_t tc = 0; tc < m_totalChannels; ++tc)
	{
		const auto quiet = std::ranges::all_of(buffer(tc), [](const float sample) {
			return std::abs(sample) < SilenceThreshold;
		});

		m_silenceFlags[tc] = quiet;
		allQuiet = allQuiet && quiet;
	}

	return allQuiet;
}

void TrackChannelContainer::silenceChannels(const ChannelFlags& channels, track_ch_t upperBound)
{
	auto needSilenced = ~m_silenceFlags;
	needSilenced &= channels;

	const auto totalChannels = std::min(upperBound, m_totalChannels);
	for (track_ch_t tc = 0; tc < totalChannels; ++tc)
	{
		if (needSilenced[tc])
		{
			std::ranges::fill(buffer(tc), 0.f);
		}
	}

	if (needSilenced[0] || needSilenced[1])
	{
		// Keep the temporary interleaved buffer in sync
		toInterleaved(buffers(0), interleavedBuffer());
	}

	m_silenceFlags |= channels;
}

void TrackChannelContainer::silenceAllChannels()
{
	std::fill_n(m_sourceBuffer, m_totalChannels * m_frames, 0);
	std::fill_n(m_interleavedBuffer, 2 * m_frames, 0);

	m_silenceFlags.set();
}

auto TrackChannelContainer::absPeakValue(track_ch_t channel) const -> float
{
	if (m_silenceFlags[channel])
	{
		// Skip calculation if channel is already known to be silent
		return 0;
	}

	return std::ranges::max(buffer(channel), {}, static_cast<float(&)(float)>(std::abs));
}

} // namespace lmms
