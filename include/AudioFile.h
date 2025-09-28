/*
 * AudioFile.h
 *
 * Copyright (c) 2025 Sotonye Atemie <sakertooth@gmail.com>
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

#ifndef LMMS_AUDIO_FILE_H
#define LMMS_AUDIO_FILE_H

#include <filesystem>

#include "AudioBufferView.h"
#include "AudioFileFormats.h"
#include "OutputSettings.h"

namespace lmms {
/**
 * @brief An abstraction for a handle to an audio file on disk.
 *
 */
class AudioFile
{
public:
	/**
	 * @brief Construct a new audio file object for reading using the given @a path.
	 *
	 * @param path The path to the audio file to read from.
	 */
	explicit AudioFile(std::filesystem::path path);

	/**
	 * @brief Construct a new Audio File object for writing to the given @a path.
	 *
	 * @param path The path to the audio file to write to.
	 * @param format The audio file format to use
	 * @param settings The various output settings for the write (e.g., bit rate, sample rate, bit depth, etc).
	 */
	AudioFile(std::filesystem::path path, AudioFileFormat format, OutputSettings settings);

	/**
	 * @brief Destroy the audio file object, closing the internal handle if necessary.
	 *
	 */
	~AudioFile();

    //! Deleted copy constructor.
	AudioFile(const AudioFile&) = delete;

    //! Deleted move constructor.
	AudioFile(AudioFile&&) = default;

    //! Deleted copy assignment operator.
	AudioFile& operator=(const AudioFile&) = delete;

    //! Deleted move assignment operator.
	AudioFile& operator=(AudioFile&&) = default;

    /**
     * @brief Read from the audio file using the given @a dst buffer
     * 
     * @param dst The buffer to read the data into.
     * @returns The number of frames written into @a dst.
     */
	auto read(InterleavedBufferView<float> dst) -> std::size_t;

    /**
     * @brief Write audio to the audio file using the given @a src buffer.
     * 
     * @param src The audio data to write into the audio file.
     * @returns The number of frames read from @a src.
     */
	auto write(InterleavedBufferView<const float> src) -> std::size_t;

    //! @returns the number of frames within the audio file.
	auto frames() const -> f_cnt_t;

    //! @returns the number of channels the audio file has.
	auto channels() const -> ch_cnt_t;

    //! @returns the sample rate of the audio file.
	auto sampleRate() const -> sample_rate_t;

    //! @returns the path to the audio file.
	auto path() const -> const std::filesystem::path&;

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};
} // namespace lmms

#endif // LMMS_AUDIO_FILE_H
