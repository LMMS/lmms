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

class AudioPortsModel;

/**
 * A non-owning span of the track channels for an instrument or effect chain
 * which keeps track of signal flow.
 *
 * Access like this:
 *   myAudioBus[channel pair index][sample index]
 *
 * where
 *   0 <= channel pair index < channelPairs()
 *   0 <= sample index < frames() * 2
 */
class LMMS_EXPORT AudioBus
{
public:
	AudioBus() = default;
	AudioBus(const AudioBus&) = default;

	AudioBus(SampleFrame* const* bus, track_ch_t channelPairs, f_cnt_t frames)
		: m_bus{bus}
		, m_channelPairs{channelPairs}
		, m_frames{frames}
	{
	}

	auto trackChannelPair(track_ch_t pairIndex) const -> InterleavedBufferView<const float, 2>
	{
		return {m_bus[pairIndex], m_frames};
	}

	auto trackChannelPair(track_ch_t pairIndex) -> InterleavedBufferView<float, 2>
	{
		return {m_bus[pairIndex], m_frames};
	}

	//! @return 2-channel interleaved buffer for the given track channel pair
	auto operator[](track_ch_t pairIndex) const -> const float*
	{
		return reinterpret_cast<const float*>(m_bus[pairIndex]);
	}

	//! @return 2-channel interleaved buffer for the given track channel pair
	auto operator[](track_ch_t pairIndex) -> float*
	{
		return reinterpret_cast<float*>(m_bus[pairIndex]);
	}

	auto bus() const -> const SampleFrame* const* { return m_bus; }
	auto bus() -> SampleFrame* const* { return m_bus; }

	auto channels() const -> track_ch_t { return m_channelPairs * 2; }
	auto channelPairs() const -> track_ch_t { return m_channelPairs; }
	auto frames() const -> f_cnt_t { return m_frames; }

	auto quietChannels() const -> const std::bitset<MaxTrackChannels>& { return m_quietChannels; }
	auto quietChannels() -> std::bitset<MaxTrackChannels>& { return m_quietChannels; }

	/**
	 * Determines whether a processor has input noise given
	 * which track channels are routed to the processor's inputs.
	 *
	 * For `trackChannelInputs`:
	 *   0 = track channel is not routed to any processor inputs
	 *   1 = track channel is routed to at least one processor input
	 *
	 * If the processor is sleeping and has input noise, it should wake up.
	 */
	auto hasInputNoise(const AudioPortsModel& apm) const -> bool;

	//! Sanitizes all used output-side track channels of any Inf/NaN values if "nanhandler" setting is enabled
	void sanitize(const AudioPortsModel& apm);

	void sanitizeAll();

	/**
	 * @brief Updates the silence status of the audio ports' used output-side track channels
	 *
	 * @returns true if all used track channels (up to the upper bound) were silent
	 */
	auto update(const AudioPortsModel& apm) -> bool;

	auto updateAll() -> bool;

	//! Silences all of the audio ports' used output track channels
	void silenceChannels(const AudioPortsModel& apm);

	void silenceAllChannels();

private:
	/**
	 * @brief Updates the silence status of the given channels, up to the upperBound index.
	 *
	 * @param channels track channels to update; 1 = selected, 0 = skip
	 * @param upperBound any track channel indexes at or above this are skipped
	 * @returns true if all updated channels were silent
	 */
	auto update(const std::bitset<MaxTrackChannels>& channels, track_ch_t upperBound = MaxTrackChannels) -> bool;

	SampleFrame* const* m_bus = nullptr; //!< [channel pair index][sample index]
	const track_ch_t m_channelPairs = 0;
	const f_cnt_t m_frames = 0;

	/**
	 * Track channels which are known to be quiet.
	 * 1 = track channel is quiet
	 * 0 = track channel is assumed to carry a signal
	 */
	std::bitset<MaxTrackChannels> m_quietChannels;
};

} // namespace lmms

#endif // LMMS_AUDIO_BUS_H
