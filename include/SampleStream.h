/*
 * SampleStream.h
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

namespace lmms {

class SampleFrame;
class SampleDecoder;

class SampleStream
{
public:
	/**
		Creates a new sample stream object that streams data from the given sample decoder in a way that is suitable
		for real-time playback.

		`size` specifies the maximum number of sample frames the stream can hold at once.

		The sample stream delegates disk reading to a dedicated thread running on the `ThreadPool`.
	**/
	SampleStream(const SampleDecoder* decoder, std::size_t size);

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
		the remaining frames are returned and the stream is considered complete.
	*/
	void read(SampleFrame* dst, std::size_t size);

	//! Returns the number of sample frames in the stream.
	auto size() const -> std::size_t;

	//! Returns the sample rate of the stream.
	auto sampleRate() const -> std::size_t;

	//! Returns `true` if the stream is complete (i.e., no more sample frames can be read), and `false` otherwise.
	auto complete() const -> bool;

private:
	void fetch();
	void runDiskStream();
	std::vector<SampleFrame> m_buffer;
    std::future<void> m_diskStream;
    std::atomic<std::size_t> m_readIndex = 0;
    std::atomic<std::size_t> m_writeIndex = 0;
};

} // namespace lmms

#endif // LMMS_SAMPLE_STREAM_H
