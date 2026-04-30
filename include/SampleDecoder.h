/*
 * SampleDecoder.h - Decodes audio files in various formats
 *
 * Copyright (c) 2023 saker <sakertooth@gmail.com>
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

#ifndef LMMS_SAMPLE_DECODER_H
#define LMMS_SAMPLE_DECODER_H

#include <QString>
#include <optional>
#include <string>
#include <vector>

#include "SampleFrame.h"

namespace lmms {
class SampleDecoder
{
public:
	struct Result
	{
		std::vector<SampleFrame> data;
		int sampleRate;
	};

	struct AudioType
	{
		std::string name;
		std::string extension;
	};

	static auto decode(const QString& audioFile) -> std::optional<Result>;
	static auto supportedAudioTypes() -> const std::vector<AudioType>&;
};
} // namespace lmms

#endif // LMMS_SAMPLE_DECODER_H
