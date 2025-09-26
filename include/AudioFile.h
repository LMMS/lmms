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

#include "AudioFileFormats.h"
#include "OutputSettings.h"
#include "AudioBufferView.h"

namespace lmms {
class AudioFile
{
public:
	explicit AudioFile(std::filesystem::path path);

	AudioFile(std::filesystem::path path, AudioFileFormat format, OutputSettings settings);

    ~AudioFile();

	AudioFile(const AudioFile&) = delete;

	AudioFile(AudioFile&&) = default;

	AudioFile& operator=(const AudioFile&) = delete;

	AudioFile& operator=(AudioFile&&) = default;

    void read(InterleavedBufferView<float> dst);

    void write(InterleavedBufferView<const float> src);

    auto frames() const -> f_cnt_t;

    auto channels() const -> ch_cnt_t;

    auto sampleRate() const -> sample_rate_t;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};
} // namespace lmms

#endif // LMMS_AUDIO_FILE_H
