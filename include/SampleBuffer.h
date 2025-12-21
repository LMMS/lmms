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

#include "LmmsTypes.h"
#include "SampleFrame.h"
#include "lmms_export.h"

namespace lmms {
//! @class SampleBuffer
//! @brief Represents a stereo, interleaved audio buffer.
//! SampleBuffer can be loaded from an audio file on disk, a Base64 string, or raw audio frame data.
//! @note This class implements the CoW (copy-on-write) technique. It stores a shared pointer to the raw audio data, so
//! if copies are made, the data is shared automatically. When modifications to this buffer are made, a deep copy of the
//! data happens if being shared.
//! @todo This class should support any number of channels, rather than being fixed at stereo.
class LMMS_EXPORT SampleBuffer
{
public:
	//! Constructs an empty buffer with no sample rate.
	SampleBuffer() = default;

	//! Constructs a buffer from the file @a audioFile on disk.
	//! @throws If an error occurred while loading the audio file.
	explicit SampleBuffer(const QString& audioFile);

	//! Constructs a buffer from the given audio Base64 string @a base64 with a sample rate of @a sampleRate.
	//! @note The Base64 string is expected to contain stereo floating point audio data.
	SampleBuffer(const QString& base64, sample_rate_t sampleRate);

	//! Constructs a buffer with size of @a numFrames and a sample rate of @a sampleRate from the raw @a data.
	SampleBuffer(const SampleFrame* data, f_cnt_t numFrames, sample_rate_t sampleRate);

	//! Constructs a silent buffer with a size of @a numFrames and a sample Rate of @a sampleRate.
	SampleBuffer(f_cnt_t numFrames, sample_rate_t sampleRate);

	//! Converts the buffer to a Base64 string representation.
	auto toBase64() const -> QString;

	//! @returns the source of the buffer. If was from an audio file, returns the path to the audio file. If instead it
	//! was from a Base64 string, that Base64 string is returned. Otherwise, an empty string is returned.
	auto source() const -> const QString& { return m_source; }

	//! @returns an immutable raw pointer to the audio data.
	auto data() const -> const SampleFrame* { return m_data->data(); }

	//! @returns true if the buffer has no audio frames, false otherwise.
	auto empty() const -> bool { return m_data->empty(); }

	//! @returns the associated sample rate with this buffer.
	auto sampleRate() const -> sample_rate_t { return m_sampleRate; }

	//! @returns the number of audio frames within the buffer.
	auto numFrames() const -> f_cnt_t { return m_data->size(); }

private:
	static auto emptyData() -> std::shared_ptr<std::vector<SampleFrame>>;
	std::shared_ptr<std::vector<SampleFrame>> m_data = emptyData();
	QString m_source;
	sample_rate_t m_sampleRate = 0;
};

} // namespace lmms

#endif // LMMS_SAMPLE_BUFFER_H
