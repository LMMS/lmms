/*
 * AudioBus.h
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

#ifndef LMMS_AUDIO_BUS_H
#define LMMS_AUDIO_BUS_H

#include <bitset>

#include "AudioBufferView.h"
#include "LmmsTypes.h"
#include "lmms_constants.h"
#include "lmms_export.h"
#include "SampleFrame.h"

namespace lmms
{

/**
 * A non-owning span of the track channels for an instrument or effect chain
 * which keeps track of signal flow.
 */
class LMMS_EXPORT AudioBus
{
public:
	AudioBus() = default;
	AudioBus(const AudioBus&) = default;

	//! `bus` is assumed to be silent
	AudioBus(SampleFrame* const* bus, track_ch_t channelPairs, f_cnt_t frames);

	auto trackChannelPair(track_ch_t pairIndex) const -> InterleavedBufferView<const float, 2>
	{
		return {m_bus[pairIndex], m_frames};
	}

	auto trackChannelPair(track_ch_t pairIndex) -> InterleavedBufferView<float, 2>
	{
		return {m_bus[pairIndex], m_frames};
	}

	//! @returns 2-channel interleaved buffer for the given track channel pair
	auto operator[](track_ch_t pairIndex) const -> const float*
	{
		return reinterpret_cast<const float*>(m_bus[pairIndex]);
	}

	//! @returns 2-channel interleaved buffer for the given track channel pair
	auto operator[](track_ch_t pairIndex) -> float*
	{
		return reinterpret_cast<float*>(m_bus[pairIndex]);
	}

	//! @returns 2D array that can be accessed like: bus()[channel pair index][sample index]
	auto bus() const -> const SampleFrame* const* { return m_bus; }

	//! @returns 2D array that can be accessed like: bus()[channel pair index][sample index]
	auto bus() -> SampleFrame* const* { return m_bus; }

	auto channels() const -> track_ch_t { return m_channelPairs * 2; }
	auto channelPairs() const -> track_ch_t { return m_channelPairs; }
	auto frames() const -> f_cnt_t { return m_frames; }

	/**
	 * Track channels which are known to be quiet, AKA the silence status.
	 * 1 = track channel is quiet
	 * 0 = track channel is assumed to carry a signal (non-quiet)
	 */
	auto quietChannels() const -> const std::bitset<MaxTrackChannels>& { return m_quietChannels; }

#ifdef LMMS_TESTING
	auto quietChannels() -> std::bitset<MaxTrackChannels>& { return m_quietChannels; }
#endif

	auto silenceTrackingEnabled() const -> bool { return m_silenceTrackingEnabled; }
	void enableSilenceTracking(bool enabled);

	//! Mixes the silence status of the other `AudioBus` with this `AudioBus`
	void mixQuietChannels(const AudioBus& other);

	/**
	 * Determines whether a processor has input noise given
	 * which track channels are routed to the processor's inputs.
	 *
	 * For `usedChannels`:
	 *   0 = track channel is not routed to any processor inputs
	 *   1 = track channel is routed to at least one processor input
	 *
	 * If the processor is sleeping and has input noise, it should wake up.
	 * If silence tracking is disabled, all channels are assumed to have input noise.
	 */
	auto hasInputNoise(const std::bitset<MaxTrackChannels>& usedChannels) const -> bool;

	//! Determines whether there is input noise on any channel. @see hasInputNoise
	auto hasAnyInputNoise() const -> bool;

	/**
	 * @brief Sanitizes specified track channels of any Inf/NaN values if "nanhandler" setting is enabled
	 *
	 * @param channels track channels to sanitize; 1 = selected, 0 = skip
	 * @param upperBound any track channel indexes at or above this are skipped
	 */
	void sanitize(const std::bitset<MaxTrackChannels>& channels, track_ch_t upperBound = MaxTrackChannels);

	//! Sanitizes all channels. @see sanitize
	void sanitizeAll();

	/**
	 * @brief Updates the silence status of the given channels, up to the upperBound index.
	 *
	 * @param channels track channels to update; 1 = selected, 0 = skip
	 * @param upperBound any track channel indexes at or above this are skipped
	 * @returns true if all updated channels were silent
	 */
	auto update(const std::bitset<MaxTrackChannels>& channels, track_ch_t upperBound = MaxTrackChannels) -> bool;

	//! Updates the silence status of all channels. @see update
	auto updateAll() -> bool;

	/**
	 * @brief Silences (zeroes) the given channels
	 *
	 * @param channels track channels to silence; 1 = selected, 0 = skip
	 * @param upperBound any track channel indexes at or above this are skipped
	 */
	void silenceChannels(const std::bitset<MaxTrackChannels>& channels, track_ch_t upperBound = MaxTrackChannels);

	//! Silences (zeroes) all channels. @see silenceChannels
	void silenceAllChannels();

private:
	SampleFrame* const* m_bus = nullptr; //!< [channel pair index][sample index]
	const track_ch_t m_channelPairs = 0;
	const f_cnt_t m_frames = 0;

	/**
	 * Stores which channels are known to be quiet.
	 *
	 * It must always be kept in sync with the buffer data when enabled - at minimum
	 * avoiding any false positives where a channel is marked as "quiet" when it isn't.
	 * Any channel bits at or above `channels()` must always be marked quiet.
	 */
	std::bitset<MaxTrackChannels> m_quietChannels;

	bool m_silenceTrackingEnabled = false;
};

} // namespace lmms

#endif // LMMS_AUDIO_BUS_H
