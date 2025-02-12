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

#include <filesystem>

#include "SampleFrame.h"

namespace lmms {

class SampleStream
{
public:
	/**
		Creates a new sample stream object targeting the audio file at the given`path`.

		`readSize` specifies the amount of sample frames that will be read at any given time.

		When there is not enough data in the stream, an underrun occurs.
		`fetchSize` specifes how much data to fetch when an underrun happens.

		The `threshold` specifies the minimum number of bytes that can exist in the stream before more sample frames is
		retrieved.

		The number of sample frames achieved after crossing `threshold` equals the `fetchSize`.

		The sample stream delegates disk reading to a dedicated thread running on the `ThreadPool`.
	**/
	SampleStream(std::filesystem::path& path, std::size_t readSize, std::size_t fetchSize, std::size_t threshold);

	//! Stops the sample stream and its dedicated disk streaming thread.
	~SampleStream();

	/**
		Reads a buffer the size of `readSize` into `dst`.

		This function outputs silence on an underrun.

		If there are less than `readSize` frames left to read,
        the remaining sample frames are returned and the stream is considered complete.
	*/
	void read(SampleFrame* dst);

	//! Returns the number of sample frames in the stream.
	auto size() const -> std::size_t;

	//! Returns the sample rate of the stream.
	auto sampleRate() const -> std::size_t;

	//! Returns `true` if the stream is complete (i.e., no more sample frames can be read), and `false` otherwise.
	auto complete() const -> bool;

private:
	void fetch();
	std::filesystem::path m_path;
	std::size_t readSize = 0;
	std::size_t fetchSize = 0;
	std::size_t threshold = 0;
};

} // namespace lmms

#endif // LMMS_SAMPLE_STREAM_H
