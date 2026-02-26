/*
 * AudioBuffer.h
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

#ifndef LMMS_TRACK_CHANNEL_CONTAINER_H
#define LMMS_TRACK_CHANNEL_CONTAINER_H

#include <bitset>
#include <memory_resource>

#include "AudioBufferView.h"
#include "ArrayVector.h"
#include "LmmsTypes.h"
#include "lmms_constants.h"
#include "lmms_export.h"

namespace lmms
{

class AudioBuffer;

//! Non-owning collection of audio channels + metadata
class ChannelGroup
{
public:
	ChannelGroup(float** buffers, ch_cnt_t channels)
		: m_buffers{buffers}
		, m_channels{channels}
	{}

	auto buffers() const -> const float* const* { return m_buffers; }
	auto buffers() -> float** { return m_buffers; }

	auto buffer(ch_cnt_t channel) const -> const float*
	{
		assert(channel < m_channels);
		return m_buffers[channel];
	}

	auto buffer(ch_cnt_t channel) -> float*
	{
		assert(channel < m_channels);
		return m_buffers[channel];
	}

	auto channels() const -> ch_cnt_t { return m_channels; }

	void setBuffers(float** newBuffers) { m_buffers = newBuffers; }

	// TODO: Future additions: Group names, type (main/aux), speaker arrangements (for surround sound), ...

private:
	//! Provides access to individual channel buffers within the source buffer
	float** m_buffers = nullptr;

	//! Number of channels in `m_channelBuffers` (`MaxChannelsPerGroup` maximum) - currently only 2 is used
	ch_cnt_t m_channels = 0;
};


/**
 * A collection of audio channels for an instrument track or mixer channel
 * which keeps track of signal flow.
 */
class LMMS_EXPORT AudioBuffer
{
public:
	using ChannelFlags = std::bitset<MaxChannelsPerTrack>;

	AudioBuffer() = delete;
	~AudioBuffer();

	AudioBuffer(const AudioBuffer&) = delete;
	AudioBuffer(AudioBuffer&&) noexcept = default;
	auto operator=(const AudioBuffer&) -> AudioBuffer& = delete;
	auto operator=(AudioBuffer&&) noexcept -> AudioBuffer& = default;

	/**
	 * Creates AudioBuffer with a 1st (main) channel group.
	 *
	 * Silence tracking is enabled or disabled depending on the auto-quit setting.
	 *
	 * @param frames frame count for all channels
	 * @param channels channel count for the 1st group, or zero to skip adding the 1st group
	 * @param bufferResource allocator for all buffers
	 */
	explicit AudioBuffer(f_cnt_t frames, ch_cnt_t channels = DEFAULT_CHANNELS,
		std::pmr::memory_resource* bufferResource = std::pmr::get_default_resource());

	//! @returns current number of channel groups
	auto groupCount() const -> group_cnt_t { return static_cast<group_cnt_t>(m_groups.size()); }

	auto group(group_cnt_t index) const -> const ChannelGroup& { return m_groups[index]; }
	auto group(group_cnt_t index) -> ChannelGroup& { return m_groups[index]; }

	//! @returns the buffers for all channel groups
	auto allBuffers() const -> PlanarBufferView<const float>
	{
		return {m_channelBuffers, m_totalChannels, m_frames};
	}

	//! @returns the buffers for all channel groups
	auto allBuffers() -> PlanarBufferView<float>
	{
		return {m_channelBuffers, m_totalChannels, m_frames};
	}

	//! @returns the buffers of the given channel group
	auto groupBuffers(group_cnt_t index) const -> PlanarBufferView<const float>
	{
		assert(index < groupCount());
		const ChannelGroup& g = m_groups[index];
		return {g.buffers(), g.channels(), m_frames};
	}

	//! @returns the buffers of the given channel group
	auto groupBuffers(group_cnt_t index) -> PlanarBufferView<float>
	{
		assert(index < groupCount());
		ChannelGroup& g = m_groups[index];
		return {g.buffers(), g.channels(), m_frames};
	}

	//! @returns the buffer for the given channel
	auto buffer(ch_cnt_t channel) const -> std::span<const float>
	{
		return {m_channelBuffers[channel], m_frames};
	}

	//! @returns the buffer for the given channel
	auto buffer(ch_cnt_t channel) -> std::span<float>
	{
		return {m_channelBuffers[channel], m_frames};
	}

	//! @returns sum of all groups' channel counts
	auto totalChannels() const -> ch_cnt_t { return m_totalChannels; }

	//! @returns the frame count for each channel buffer
	auto frames() const -> f_cnt_t { return m_frames; }

	//! @returns scratch buffer for conversions between interleaved and planar TODO: Remove once using planar only
	auto interleavedBuffer() const -> InterleavedBufferView<const float, 2>
	{
		return {m_interleavedBuffer, m_frames};
	}

	//! @returns scratch buffer for conversions between interleaved and planar TODO: Remove once using planar only
	auto interleavedBuffer() -> InterleavedBufferView<float, 2>
	{
		return {m_interleavedBuffer, m_frames};
	}

	/**
	 * @brief Adds a new channel group at the end of the list
	 * @returns the newly created group, or nullptr upon failure
	 */
	auto addGroup(ch_cnt_t channels) -> ChannelGroup*;

	/**
	 * Channels which are known to be quiet, AKA the silence status.
	 * 1 = channel is known to be silent
	 * 0 = channel is assumed to be non-silent (or, when silence tracking
	 *     is enabled, known to be non-silent)
	 *
	 * NOTE: If any channel buffers are used and their data modified outside of this class,
	 *       their silence flags will be invalidated until `updateSilenceFlags()` is called.
	 *       Therefore, calling code must be careful to always keep the silence flags up-to-date.
	 */
	auto silenceFlags() const -> const ChannelFlags& { return m_silenceFlags; }

	//! Forcibly pessimizes silence tracking for a specific channel
	void assumeNonSilent(ch_cnt_t channel) { m_silenceFlags[channel] = false; }

	/**
	 * When silence tracking is enabled, channels will be checked for silence whenever their data may
	 * have changed, so it'll always be known whether they are silent or non-silent. There is a performance cost
	 * to this, but it is likely worth it since this information allows many effects to be put to sleep
	 * when their inputs are silent ("auto-quit"). When a channel is known to be silent, it also
	 * enables optimizations in buffer sanitization, buffer zeroing, and finding the absolute peak sample value.
	 *
	 * When silence tracking is disabled, channels are not checked for silence, so a silence flag may be
	 * unset despite the channel being silent. Non-silence must be assumed whenever the silence status is not
	 * known, so the optimizations which silent buffers allow will not be possible as often.
	 */
	void enableSilenceTracking(bool enabled);
	auto silenceTrackingEnabled() const -> bool { return m_silenceTrackingEnabled; }

	//! Mixes the silence flags of the other `AudioBuffer` with this `AudioBuffer`
	void mixSilenceFlags(const AudioBuffer& other);

	/**
	 * Determines whether a processor has input noise given
	 * which channels are routed to the processor's inputs.
	 *
	 * For `usedChannels`:
	 *   0 = channel is not routed to any processor inputs
	 *   1 = channel is routed to at least one processor input
	 *
	 * If the processor is sleeping and has input noise, it should wake up.
	 * If silence tracking is disabled, all channels that aren't marked
	 * as silent are assumed to have input noise.
	 */
	auto hasInputNoise(const ChannelFlags& usedChannels) const -> bool;

	//! Determines whether there is input noise on any channel. @see hasInputNoise
	auto hasAnyInputNoise() const -> bool;

	/**
	 * @brief Sanitizes specified channels of any Inf/NaN values if "nanhandler" setting is enabled
	 *
	 * @param channels channels to sanitize; 1 = selected, 0 = skip
	 * @param upperBound any channel indexes at or above this are skipped
	 */
	void sanitize(const ChannelFlags& channels, ch_cnt_t upperBound = MaxChannelsPerTrack);

	//! Sanitizes all channels. @see sanitize
	void sanitizeAll();

	/**
	 * @brief Updates the silence status of the given channels, up to the upperBound index.
	 *
	 * @param channels channels to update; 1 = selected, 0 = skip
	 * @param upperBound any channel indexes at or above this are skipped
	 * @returns true if all selected channels were silent
	 */
	auto updateSilenceFlags(const ChannelFlags& channels, ch_cnt_t upperBound = MaxChannelsPerTrack) -> bool;

	//! Updates the silence status of all channels. @see updateSilenceFlags
	auto updateAllSilenceFlags() -> bool;

	/**
	 * @brief Silences (zeroes) the given channels
	 *
	 * @param channels channels to silence; 1 = selected, 0 = skip
	 * @param upperBound any channel indexes at or above this are skipped
	 */
	void silenceChannels(const ChannelFlags& channels, ch_cnt_t upperBound = MaxChannelsPerTrack);

	//! Silences (zeroes) all channels. @see silenceChannels
	void silenceAllChannels();

	//! @returns absolute peak sample value for the given channel
	auto absPeakValue(ch_cnt_t channel) const -> float;

private:
	//! Large buffer that all channel buffers are sourced from
	float* m_sourceBuffer = nullptr;

	//! Provides access to individual channel buffers within the source buffer
	float** m_channelBuffers = nullptr;

	//! Interleaved scratch buffer for conversions between interleaved and planar TODO: Remove once using planar only
	float* m_interleavedBuffer = nullptr;

	ArrayVector<ChannelGroup, MaxGroupsPerTrack> m_groups;

	//! Caches the sum of `m_groups[idx].channels()` - must never exceed MaxChannelsPerTrack
	ch_cnt_t m_totalChannels = 0;

	const f_cnt_t m_frames = 0;

	//! Allocator used by all buffers
	std::pmr::polymorphic_allocator<> m_alloc;

	/**
	 * Stores which channels are known to be quiet, AKA the silence status.
	 *
	 * This must always be kept in sync with the buffer data when enabled - at minimum
	 * avoiding any false positives where a channel is marked as "silent" when it isn't.
	 * Any channel bits at or above `m_totalChannels` must always be marked silent.
	 *
	 * 1 = channel is known to be silent
	 * 0 = channel is assumed to be non-silent (or, when silence tracking
	 *     is enabled, known to be non-silent)
	 */
	ChannelFlags m_silenceFlags;

	bool m_silenceTrackingEnabled = false;
};

} // namespace lmms

#endif // LMMS_TRACK_CHANNEL_CONTAINER_H
