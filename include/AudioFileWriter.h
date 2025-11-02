/*
 * AudioFileWriter.h
 *
 * Copyright (c) 2025 saker <sakertooth@gmail.com>
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

#ifndef LMMS_AUDIO_FILE_WRITER_H
#define LMMS_AUDIO_FILE_WRITER_H

#include <filesystem>

#include "AudioBufferView.h"
#include "AudioFileFormats.h"
#include "OutputSettings.h"

namespace lmms {
/**
 * @brief A class that can be used to write to audio files on disk.
 *
 */
class AudioFileWriter
{
public:
	AudioFileWriter(const AudioFileWriter&) = delete;
	AudioFileWriter(AudioFileWriter&&) = delete;
	AudioFileWriter& operator=(const AudioFileWriter&) = delete;
	AudioFileWriter& operator=(AudioFileWriter&&) = delete;

	/**
	 * @brief Construct a new Audio File object for writing to the given @a path.
	 *
	 * @param path The path to the audio file to write to.
	 * @param format The audio file format to use
	 * @param settings The various output settings for the write (e.g., bit rate, sample rate, bit depth, etc).
	 */
	AudioFileWriter(std::filesystem::path path, AudioFileFormat format, OutputSettings settings);

	/**
	 * @brief Destroy the audio file object, closing the internal handle if necessary.
	 *
	 */
	~AudioFileWriter();

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
	std::filesystem::path m_path;
};
} // namespace lmms

#endif // LMMS_AUDIO_FILE_WRITER_H
