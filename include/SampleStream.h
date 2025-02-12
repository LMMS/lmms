/*
 * SampleStream.h - an object meant to stream audio files
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

#ifndef LMMS_SAMPLE_STREAM_H
#define LMMS_SAMPLE_STREAM_H

#include <future>
#include <vector>

#include "AudioFile.h"

namespace lmms {

class SampleFrame;
class SampleDecoder;

class SampleStream
{
public:
	static constexpr auto DefaultStreamCapacity = 8192;

	/**
		Creates a new sample stream object that streams data from the given audio file at `path` in a way that is
	suitable for real-time playback.

		`size` specifies the maximum number of sample frames the stream can hold at once.

		The sample stream delegates disk reading to a dedicated thread running on the `ThreadPool`.
	**/
	SampleStream(const std::filesystem::path& path, std::size_t size = DefaultStreamCapacity);

	SampleStream(const SampleStream& stream);
	SampleStream& operator=(const SampleStream& stream);

	SampleStream(SampleStream&& stream) noexcept;
	SampleStream& operator=(SampleStream&& stream) noexcept;

	//! Stops the sample stream and its dedicated disk streaming thread.
	~SampleStream();

	/**
		Reads `size` frames from the stream into the buffer `dst`.

		This function outputs silence if there is an underrun.
		To keep underruns at a minimum, the size of the stream should be a specified appropriately according
		to the number of bytes being read.

		A call to `read` tells the dedicated disk streaming thread to start fetching more data if more can
		be fetched (given the constraints of the stream size).

		If there are less than `size` frames left to read from the stream,
		the remaining frames are copied into `dst` and the stream is considered complete.

		Returns the number of frames actually written into `dst`.
	*/
	auto read(SampleFrame* dst, std::size_t size) -> std::size_t;

	//! Returns the number of sample frames that can be read from the stream.
	auto streamSize() const -> std::size_t { return m_writeIndex - m_readIndex; }

	//! Returns the maximum number of sample frames that can be contained within the stream.
	auto streamCapacity() const -> std::size_t { return m_buffer.size(); }

	//! Returns the sample rate of the stream.
	auto sampleRate() const -> int { return m_audioFile.sampleRate();}

private:
	void runDiskStream();
	AudioFile m_audioFile;
	std::vector<SampleFrame> m_buffer;
	std::future<void> m_diskStream;
	std::atomic<bool> m_quit = false;
	std::atomic<std::size_t> m_readIndex = 0;
	std::atomic<std::size_t> m_writeIndex = 0;
};

} // namespace lmms

#endif // LMMS_SAMPLE_STREAM_H
