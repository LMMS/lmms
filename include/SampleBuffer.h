/*
 * SampleBuffer.h - container-class SampleBuffer
 *
 * Copyright (c) 2005-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef LMMS_SAMPLE_BUFFER_H
#define LMMS_SAMPLE_BUFFER_H

#include <QString>
#include <memory>
#include <vector>

#include "AudioBufferView.h"
#include "LmmsTypes.h"
#include "lmms_export.h"

namespace lmms {

/**
 * @brief A storage class for an interleaved buffer of audio.
 *
 * SampleBuffer is storage class that combines interleaved floating-point audio with its respective sample rate.
 * It can be created either from a file, Base64 string, or an initial empty buffer.

 * Base64 strings should represent a buffer that contains the floating-point audio data and nothing else. The channel
 * count and sample rate are to be provided separately.
 *
 * This class should not be used directly when there is a need to play the buffer back.
 * For this purpose, see @ref Sample, which uses this class internally but adds additional playback functionality.
 */
class LMMS_EXPORT SampleBuffer
{
public:
	//! The default sample rate
	static constexpr auto DefaultSampleRate = sample_rate_t{44100};

	//! The default channel count
	static constexpr auto DefaultChannels = ch_cnt_t{2};

	//! Creats an empty buffer with a sample rate and channel count of @a DefaultSampleRate and @a DefaultChannels
	//! respetively.
	SampleBuffer() = default;

	/**
	 * @brief Creates a buffer with @a channels channels, @a frames frames, and a sample rate of @a sampleRate.
	 *
	 * @param channels The number of channels
	 * @param frames The number of frames
	 * @param sampleRate The sample rate
	 * @param audioFile The audio file specified (optional)
	 */
	SampleBuffer(
		ch_cnt_t channels, f_cnt_t frames, sample_rate_t sampleRate = DefaultSampleRate, const QString& audioFile = "");

	//! @returns A pointer to the frame at the given frame index
	auto operator[](f_cnt_t frameIndex) -> float* { return &m_data[frameIndex * m_channels]; }

	//! @returns A pointer to the frame at the given frame index
	auto operator[](f_cnt_t frameIndex) const -> const float* { return &m_data[frameIndex * m_channels]; }

	//! @returns A span of the frame at the given frame index
	auto frame(f_cnt_t frameIndex) -> std::span<float> { return {&m_data[frameIndex * m_channels], m_channels}; }

	//! @returns A span of the frame at the given frame index
	auto frame(f_cnt_t frameIndex) const -> std::span<const float>
	{ return {&m_data[frameIndex * m_channels], m_channels}; }

	//! @returns A frame iterator at the beginning of the buffer
	auto begin() { return view().framesView().begin(); }

	//! @returns A frame iterator at the beginning of the buffer
	auto begin() const { return view().framesView().begin(); }

	//! @returns A frame iterator at the end of the buffer
	auto end() { return view().framesView().end(); }

	//! @returns A frame iterator at the end of the buffer
	auto end() const { return view().framesView().end(); }

	//! @returns An interleaved view of the buffer
	auto view() -> InterleavedBufferView<float> { return {m_data.data(), m_channels, frames()}; }

	//! @returns An interleaved view of the buffer
	auto view() const -> InterleavedBufferView<const float> { return {m_data.data(), m_channels, frames()}; }

	//! @returns A pointer to the floating point data of the buffer
	auto data() -> float* { return m_data.data(); }

	//! @returns A pointer to the floating point data of the buffer
	auto data() const -> const float* { return m_data.data(); }

	//! @returns The number of frames this buffer holds
	auto frames() const -> f_cnt_t { return m_data.size() / m_channels; }

	//! @returns True if the buffer contains no frames, false otherwise
	auto empty() const -> bool { return m_data.empty(); }

	//! @returns The number of channels this buffer has
	auto channels() const -> ch_cnt_t { return m_channels; }

	//! @returns The sample rate of the buffer
	auto sampleRate() const -> sample_rate_t { return m_sampleRate; }

	//! @returns The audio file (if any) associated with the buffer
	auto audioFile() const -> const QString& { return m_audioFile; }

	//! @brief Converts the buffer's audio into a Base64 string
	auto toBase64() const -> QString;

	//! @returns An empty buffer that can be shared
	static auto emptyBuffer() -> std::shared_ptr<const SampleBuffer>;

	/**
	 * @brief Creates a buffer from the given @a path.
	 *
	 * @param path
	 * @returns The buffer on success, and nullptr on failure.
	 */
	static std::shared_ptr<const SampleBuffer> fromFile(const QString& path);

	/**
	 * @brief Creates a buffer from a Base64 string containing encoded floating-point audio date.
	 *
	 * @param str The Base64 string to decode
	 * @param channels The number of channels for this buffer
	 * @param sampleRate The sample rate for this buffer
	 * @returns The buffer on success, and nullptr on failure.
	 */
	static std::shared_ptr<const SampleBuffer> fromBase64(
		const QString& str, ch_cnt_t channels = DefaultChannels, sample_rate_t sampleRate = DefaultSampleRate);

private:
	std::vector<float> m_data;
	ch_cnt_t m_channels = DefaultChannels;
	sample_rate_t m_sampleRate = DefaultSampleRate;
	QString m_audioFile;
};

} // namespace lmms

#endif // LMMS_SAMPLE_BUFFER_H
