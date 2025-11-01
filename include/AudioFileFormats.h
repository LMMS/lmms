/*
 * AudioFileFormats.h
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

#ifndef LMMS_AUDIO_FILE_FORMATS_H
#define LMMS_AUDIO_FILE_FORMATS_H

#include <array>
#include <string_view>

namespace lmms {

/**
 * @brief An enum consisting of all the recognized and supported audio formats.
 * @todo Currently, this only covers the formats we export, but should be extended to cover all other audio formats we
 * acknowledge.
 */
enum class AudioFileFormat
{
	WAV,
	FLAC,
	OGG,
	MP3
};

/**
 * @brief Information (such as the name and extension) for each format.
 *
 */
struct AudioFileFormatDescriptor
{
	AudioFileFormat format;
	std::string_view name;
	std::string_view extension;
};

/**
 * @brief A list of all the audio file formats, each combined with their respective descriptors.
 *
 */
static constexpr auto AudioFileFormats = std::array{AudioFileFormatDescriptor{AudioFileFormat::WAV, "WAV", ".wav"},
	AudioFileFormatDescriptor{AudioFileFormat::FLAC, "FLAC", ".flac"},
	AudioFileFormatDescriptor{AudioFileFormat::OGG, "OGG", ".ogg"},
	AudioFileFormatDescriptor{AudioFileFormat::MP3, "MP3", ".mp3"}};

} // namespace lmms

#endif // LMMS_AUDIO_FILE_FORMATS_H
