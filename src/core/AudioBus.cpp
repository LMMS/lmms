/*
 * AudioBus.cpp
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

#include "AudioBus.h"

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
auto createMask(track_ch_t pos) noexcept -> AudioBus::ChannelFlags
{
	assert(pos <= MaxTrackChannels);

	AudioBus::ChannelFlags mask;
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

AudioBus::BusData::BusData(std::pmr::polymorphic_allocator<>& alloc,
	ch_cnt_t channels, f_cnt_t frames, track_ch_t startingChannel)
	: m_sourceBuffer{alloc.allocate_object<float>(channels * frames)}
	, m_channelBuffers{alloc.allocate_object<float*>(channels)}
	, m_interleavedBuffer{alloc.allocate_object<float>(2 * frames)}
	, m_channels{channels}
	, m_startingChannel{startingChannel}
{
	// Set uninitialized buffers to zero (silent)
	std::fill_n(m_sourceBuffer, channels * frames, 0.f);
	std::fill_n(m_interleavedBuffer, 2 * frames, 0.f);

	// Initialize channel buffers
	float* ptr = m_sourceBuffer;
	track_ch_t channel = 0;
	while (channel < channels)
	{
		m_channelBuffers[channel] = ptr;

		ptr += frames;
		++channel;
	}
}

AudioBus::AudioBus(f_cnt_t frames, ch_cnt_t channels, std::pmr::memory_resource* bufferResource)
	: m_frames{frames}
	, m_alloc{bufferResource}
	, m_silenceTrackingEnabled{ConfigManager::inst()->value("ui", "disableautoquit", "1").toInt() == 0}
{
	if (!addBus(channels))
	{
		throw std::runtime_error{"failed to add bus"};
	}

	m_silenceFlags.set();
}

AudioBus::~AudioBus()
{
	for (BusData& bus : m_busses)
	{
		m_alloc.deallocate_object(bus.m_sourceBuffer, bus.channels() * m_frames);
		m_alloc.deallocate_object(bus.channelBuffers(), bus.channels());
		m_alloc.deallocate_object(bus.m_interleavedBuffer, 2 * m_frames);
	}
}

auto AudioBus::addBus(ch_cnt_t channels) -> BusData*
{
	if (m_busses.size() >= m_busses.capacity())
	{
		// Maximum busses reached
		return nullptr;
	}

	if (channels > MaxChannelsPerBus || channels == 0)
	{
		// Invalid channel count for a bus
		return nullptr;
	}

	if (m_totalChannels + channels > MaxTrackChannels)
	{
		// Not enough room for requested track channels
		return nullptr;
	}

	const auto startingChannel = m_busses.empty()
		? track_ch_t{0}
		: static_cast<track_ch_t>(m_busses.back().startingChannel() + m_busses.back().channels());

	auto& data = m_busses.emplace_back(m_alloc, channels, m_frames, startingChannel);

	// Ensure the new track channels (and all the higher, unused
	// track channels) are set to "silent"
	m_silenceFlags |= createMask<true>(m_totalChannels);

	m_totalChannels += channels;

	return &data;
}

void AudioBus::enableSilenceTracking(bool enabled)
{
	const auto oldValue = m_silenceTrackingEnabled;
	m_silenceTrackingEnabled = enabled;
	if (!oldValue && enabled)
	{
		updateAllSilenceFlags();
	}
}

void AudioBus::mixQuietChannels(const AudioBus& other)
{
	m_silenceFlags &= other.silenceFlags();
}

auto AudioBus::hasInputNoise(const ChannelFlags& usedChannels) const -> bool
{
	auto nonSilent = ~m_silenceFlags;
	nonSilent &= usedChannels;
	return nonSilent.any();
}

auto AudioBus::hasAnyInputNoise() const -> bool
{
	return !m_silenceFlags.all();
}

void AudioBus::sanitize(const ChannelFlags& channels, track_ch_t upperBound)
{
	if (!MixHelpers::useNaNHandler()) { return; }

	auto trackChannelsLeft = std::min(upperBound, totalChannels());

	auto trackChannel = track_ch_t{0};
	for (BusData& bus : m_busses)
	{
		const auto busChannels = std::min<track_ch_t>(trackChannelsLeft, bus.channels());
		assert(busChannels <= MaxChannelsPerBus);

		for (ch_cnt_t ch = 0; ch < static_cast<ch_cnt_t>(busChannels); ++ch)
		{
			if (channels[trackChannel])
			{
				// This channel needs to be sanitized
				if (MixHelpers::sanitize(std::span{bus.channelBuffer(ch), m_frames}))
				{
					// Inf/NaN detected and buffer cleared
					m_silenceFlags[trackChannel] = true;
				}
			}

			++trackChannel;
		}

		trackChannelsLeft -= busChannels;
		if (trackChannelsLeft == 0) { break; }
	}

	if (channels[0] || channels[1])
	{
		// Keep the temporary interleaved buffer in sync
		MixHelpers::copy(interleavedBuffer(0), buffers(0));
	}
}

void AudioBus::sanitizeAll()
{
	if (!MixHelpers::useNaNHandler()) { return; }

	auto trackChannel = track_ch_t{0};
	for (BusData& bus : m_busses)
	{
		for (ch_cnt_t ch = 0; ch < bus.channels(); ++ch)
		{
			if (MixHelpers::sanitize(std::span{bus.channelBuffer(ch), m_frames}))
			{
				// Inf/NaN detected and buffer cleared
				m_silenceFlags[trackChannel] = true;
			}

			++trackChannel;
		}
	}

	// Keep the temporary interleaved buffer in sync
	MixHelpers::copy(interleavedBuffer(0), buffers(0));
}

auto AudioBus::updateSilenceFlags(const ChannelFlags& channels, track_ch_t upperBound) -> bool
{
	assert(upperBound <= MaxTrackChannels);

	// Invariant: Any channel bits at or above `totalChannels()` must be marked silent
	assert((~m_silenceFlags & createMask<true>(totalChannels())).none());

	auto trackChannelsLeft = std::min(upperBound, totalChannels());

	if (!m_silenceTrackingEnabled)
	{
		// Mark specified channels (up to the upper bound) as non-silent
		auto temp = ~channels;
		temp |= createMask<true>(trackChannelsLeft);
		m_silenceFlags &= temp;
		return false;
	}

	bool allQuiet = true;

	auto trackChannel = track_ch_t{0};
	for (BusData& bus : m_busses)
	{
		const auto busChannels = std::min<track_ch_t>(trackChannelsLeft, bus.channels());
		assert(busChannels <= MaxChannelsPerBus);

		for (ch_cnt_t ch = 0; ch < static_cast<ch_cnt_t>(busChannels); ++ch)
		{
			if (channels[trackChannel])
			{
				// This channel needs to be updated
				const auto buffer = std::span{bus.channelBuffer(ch), m_frames};
				const auto quiet = std::ranges::all_of(buffer, [](const float sample) {
					return std::abs(sample) < SilenceThreshold;
				});

				m_silenceFlags[trackChannel] = quiet;
				allQuiet = allQuiet && quiet;
			}

			++trackChannel;
		}

		trackChannelsLeft -= busChannels;
		if (trackChannelsLeft == 0) { break; }
	}

	return allQuiet;
}

auto AudioBus::updateAllSilenceFlags() -> bool
{
	// Invariant: Any channel bits at or above `totalChannels()` must be marked silent
	assert((~m_silenceFlags & createMask<true>(totalChannels())).none());

	if (!m_silenceTrackingEnabled)
	{
		// Mark all channels below `channels()` as non-silent
		m_silenceFlags &= createMask<true>(totalChannels());
		return false;
	}

	bool allQuiet = true;

	auto trackChannel = track_ch_t{0};
	for (BusData& bus : m_busses)
	{
		for (ch_cnt_t ch = 0; ch < bus.channels(); ++ch)
		{
			const auto buffer = std::span{bus.channelBuffer(ch), m_frames};
			const auto quiet = std::ranges::all_of(buffer, [](const float sample) {
				return std::abs(sample) < SilenceThreshold;
			});

			m_silenceFlags[trackChannel] = quiet;
			allQuiet = allQuiet && quiet;

			++trackChannel;
		}
	}

	return allQuiet;
}

void AudioBus::silenceChannels(const ChannelFlags& channels, track_ch_t upperBound)
{
	auto needSilenced = ~m_silenceFlags;
	needSilenced &= channels;

	auto trackChannelsLeft = std::min(upperBound, totalChannels());

	auto trackChannel = track_ch_t{0};
	for (BusData& bus : m_busses)
	{
		const auto busChannels = std::min<track_ch_t>(trackChannelsLeft, bus.channels());
		assert(busChannels <= MaxChannelsPerBus);

		for (ch_cnt_t ch = 0; ch < static_cast<ch_cnt_t>(busChannels); ++ch)
		{
			if (needSilenced[trackChannel])
			{
				std::ranges::fill(std::span{bus.channelBuffer(ch), m_frames}, 0);
			}

			++trackChannel;
		}

		trackChannelsLeft -= busChannels;
		if (trackChannelsLeft == 0) { break; }
	}

	if (needSilenced[0] || needSilenced[1])
	{
		// Keep the temporary interleaved buffer in sync
		MixHelpers::copy(interleavedBuffer(0), buffers(0));
	}

	m_silenceFlags |= channels;
}

void AudioBus::silenceAllChannels()
{
	for (BusData& bus : m_busses)
	{
		std::fill_n(bus.m_sourceBuffer, bus.channels() * m_frames, 0);
		std::fill_n(bus.interleavedBuffer(), 2 * m_frames, 0);
	}

	m_silenceFlags.set();
}

auto AudioBus::absPeakValue(bus_cnt_t busIndex, ch_cnt_t busChannel) const -> float
{
	if (m_silenceFlags[m_busses[busIndex].startingChannel() + busChannel])
	{
		// Skip calculation if channel is already known to be silent
		return 0;
	}

	const float* buffer = m_busses[busIndex].channelBuffer(busChannel);

	float max = 0;
	for (f_cnt_t frame = 0; frame < m_frames; ++frame)
	{
		max = std::max(std::abs(buffer[frame]), max);
	}

	return max;
}

} // namespace lmms
