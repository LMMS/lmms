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
#include <optional>
#include <vector>

#include "LmmsTypes.h"
#include "SampleFrame.h"
#include "lmms_export.h"

namespace lmms {
/**
 * @class SampleBuffer
 *
 * @brief A shared container of immutable PCM audio data.
 *
 * SampleBuffer is meant to be used for storing audio from user samples or Base64 assets. It enforces shared
 * ownership: copying a SampleBuffer increments a ref count for the shared audio data rather than deep copying it.
 *
 * @warning This class is immutable. If you need a mutable buffer for
 * realtime audio processing, use @ref AudioBuffer instead.
 *
 * @invariant A SampleBuffer is always either empty or not empty.
 */
class LMMS_EXPORT SampleBuffer
{
public:
	/** @brief Creates an empty SampleBuffer. */
	SampleBuffer() = default;

	/**
	 * @brief Constructs a buffer with explicit data.
	 * @param data A vector of SampleFrame containing the audio data.
	 * @param sampleRate The recording rate of the source (e.g., 44100).
	 * @param path The filesystem path the data originated from.
	 */
	SampleBuffer(std::vector<SampleFrame> data, sample_rate_t sampleRate, const QString& path = "");

	/** @returns a const reference to the audio frame at the given frame index. */
	auto operator[](f_cnt_t index) const -> const SampleFrame& { return m_data[index]; }

	/** @brief Serializes the raw PCM data into a Base64 string for project saving.
		@todo This function should be removed once Base64 is migrated to audio files on disk.
	 */
	auto toBase64() const -> QString;

	/** @returns true if the buffer is empty, false otherwise. */
	auto empty() const -> bool { return m_frames == 0; }

	/** @returns direct access to the raw audio data.
		@invariant If this buffer is empty, data() == nullptr
		@invariant If this buffer is not empty, data() != nullptr
	*/
	auto data() const -> const SampleFrame* { return m_data.get(); }

	/** @returns the file path associated with this buffer, if any.
		@invariant this function is guaranteed to give back a non empty string pointing to a valid path
		when loaded successfully via @ref fromFile.
	*/
	auto path() const -> const QString& { return m_path; }

	/** @returns the total number of audio frames.
		@invariant If this buffer is empty, frames() == 0
		@invariant If this buffer is not empty, frames() > 0
	*/
	auto frames() const -> f_cnt_t { return m_frames; }

	/** @returns the original sample rate of the data.
		@invariant If this buffer is empty, sampleRate() == 0
		@invariant If this buffer is not empty, sampleRate() > 0
	*/
	auto sampleRate() const -> sample_rate_t { return m_sampleRate; }

	/**
	 * @brief Factory method to load and decode an audio file into a SampleBuffer.
	 * @param path The absolute path to the audio file (WAV, FLAC, OGG, etc.).
	 * @return std::optional containing the buffer on success, or std::nullopt on failure.
	 */
	static std::optional<SampleBuffer> fromFile(const QString& path);

	/**
	 * @brief Factory method to reconstruct a buffer from a Base64 string.
	 * @param str The Base64 encoded audio data.
	 * @param sampleRate The rate at which the encoded data should be interpreted.
	 * @return std::optional containing the buffer on success.
	 * @todo This function should be removed once Base64 is migrated to audio files on disk.
	 */
	static std::optional<SampleBuffer> fromBase64(const QString& str, sample_rate_t sampleRate);

private:
	QString m_path;
	f_cnt_t m_frames = 0;
	sample_rate_t m_sampleRate = 0;
	std::shared_ptr<SampleFrame[]> m_data;
};

} // namespace lmms

#endif // LMMS_SAMPLE_BUFFER_H
