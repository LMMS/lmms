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
#include <memory_resource>

#include "AudioBufferView.h"
#include "ArrayVector.h"
#include "LmmsTypes.h"
#include "lmms_constants.h"
#include "lmms_export.h"

namespace lmms
{

/**
 * A collection of track channels for an instrument or effect chain
 * which keeps track of signal flow.
 */
class LMMS_EXPORT AudioBus
{
public:
	class LMMS_EXPORT BusData
	{
	public:
		BusData(std::pmr::polymorphic_allocator<>& alloc,
			ch_cnt_t channels, f_cnt_t frames, track_ch_t startingChannel);

		BusData(const BusData&) = delete;
		BusData(BusData&&) noexcept = default;
		auto operator=(const BusData&) -> BusData& = delete;
		auto operator=(BusData&&) noexcept -> BusData& = default;

		auto channelBuffers() const -> const float* const* { return m_channelBuffers; }
		auto channelBuffers() -> float** { return m_channelBuffers; }

		auto channelBuffer(ch_cnt_t channel) const -> const float*
		{
			assert(channel < m_channels);
			return m_channelBuffers[channel];
		}

		auto channelBuffer(ch_cnt_t channel) -> float*
		{
			assert(channel < m_channels);
			return m_channelBuffers[channel];
		}

		auto interleavedBuffer() const -> const float* { return m_interleavedBuffer; }
		auto interleavedBuffer() -> float* { return m_interleavedBuffer; }

		auto channels() const -> ch_cnt_t { return m_channels; }

		auto startingChannel() const -> track_ch_t { return m_startingChannel; }

		friend class AudioBus;

	private:
		//! Large buffer that all channel buffers are sourced from
		float*     m_sourceBuffer = nullptr;

		//! Provides access to individual channel buffers within the source buffer
		float**    m_channelBuffers = nullptr;

		//! Interleaved scratch buffer for conversions between interleaved and planar TODO: Remove once using planar only
		float*     m_interleavedBuffer = nullptr;

		//! Number of channels in `m_channelBuffers` (`MaxChannelsPerBus` maximum) - currently only 2 is used
		ch_cnt_t   m_channels = 0;

		//! Maps channel #0 of this bus to its track channel # within AudioBus (for performance)
		track_ch_t m_startingChannel = 0;
	};

	AudioBus() = default;
	~AudioBus();

	AudioBus(const AudioBus&) = delete;
	AudioBus(AudioBus&&) noexcept = default;
	auto operator=(const AudioBus&) -> AudioBus& = delete;
	auto operator=(AudioBus&&) noexcept -> AudioBus& = default;

	//! Single stereo bus with `frames` frames
	explicit AudioBus(f_cnt_t frames, std::pmr::memory_resource* bufferResource = std::pmr::get_default_resource());

	auto busCount() const -> bus_cnt_t { return static_cast<bus_cnt_t>(m_busses.size()); }

	//! @returns the buffers of the given bus
	auto buffers(bus_cnt_t busIndex) const -> PlanarBufferView<const float>
	{
		assert(busIndex < busCount());
		const BusData& b = m_busses[busIndex];
		return {b.channelBuffers(), b.channels(), m_frames};
	}

	//! @returns the buffers of the given bus
	auto buffers(bus_cnt_t busIndex) -> PlanarBufferView<float>
	{
		assert(busIndex < busCount());
		BusData& b = m_busses[busIndex];
		return {b.channelBuffers(), b.channels(), m_frames};
	}

	//! @returns planar channel buffers for the given bus
	auto operator[](bus_cnt_t busIndex) const -> const float* const*
	{
		return m_busses[busIndex].channelBuffers();
	}

	//! @returns planar channel buffers for the given bus
	auto operator[](bus_cnt_t busIndex) -> float**
	{
		return m_busses[busIndex].channelBuffers();
	}

	//! @returns array of busses
	auto busses() const -> const ArrayVector<BusData, MaxBussesPerTrack>& { return m_busses; }

	//! @returns array of busses
	auto busses() -> ArrayVector<BusData, MaxBussesPerTrack>& { return m_busses; }

	//! @returns sum of all bus channel counts
	auto totalChannels() const -> track_ch_t { return m_totalChannels; }

	//! @returns the frame count for each channel buffer
	auto frames() const -> f_cnt_t { return m_frames; }

	//! @returns scratch buffer for conversions between interleaved and planar TODO: Remove once using planar only
	auto interleavedBuffer(bus_cnt_t busIndex) const -> InterleavedBufferView<const float, 2>
	{
		assert(m_busses[busIndex].channels() == 2);
		return {m_busses[busIndex].interleavedBuffer(), m_frames};
	}

	//! @returns scratch buffer for conversions between interleaved and planar TODO: Remove once using planar only
	auto interleavedBuffer(bus_cnt_t busIndex) -> InterleavedBufferView<float, 2>
	{
		assert(m_busses[busIndex].channels() == 2);
		return {m_busses[busIndex].interleavedBuffer(), m_frames};
	}

	/**
	 * @brief Adds a new bus at the end of the list
	 * @returns the newly created bus, or nullptr upon failure
	 */
	auto addBus(ch_cnt_t channels) -> BusData*;

	/**
	 * Track channels which are known to be quiet, AKA the silence status.
	 * 1 = track channel is known to be silent
	 * 0 = track channel is assumed to be non-silent
	 */
	auto silenceFlags() const -> const std::bitset<MaxTrackChannels>& { return m_silenceFlags; }

#ifdef LMMS_TESTING
	auto silenceFlags() -> std::bitset<MaxTrackChannels>& { return m_silenceFlags; }
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

	//! @returns absolute peak sample value for the given channel
	auto absPeakValue(bus_cnt_t busIndex, ch_cnt_t busChannel) const -> float;

private:
	ArrayVector<BusData, MaxBussesPerTrack> m_busses;

	//! Caches the sum of `m_busses[idx].channels()` - must never exceed MaxTrackChannels
	track_ch_t m_totalChannels = 0;

	const f_cnt_t m_frames = 0;

	//! Allocator used by all buffers
	std::pmr::polymorphic_allocator<> m_alloc;

	/**
	 * Stores which track channels are known to be quiet, AKA the silence status.
	 *
	 * This must always be kept in sync with the buffer data when enabled - at minimum
	 * avoiding any false positives where a channel is marked as "silent" when it isn't.
	 * Any channel bits at or above `m_totalChannels` must always be marked silent.
	 *
	 * 1 = track channel is known to be silent
	 * 0 = track channel is assumed to be non-silent
	 */
	std::bitset<MaxTrackChannels> m_silenceFlags;

	bool m_silenceTrackingEnabled = false;
};

} // namespace lmms

#endif // LMMS_AUDIO_BUS_H
