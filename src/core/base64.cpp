/*
 * base64.cpp - namespace base64 with methods for encoding/decoding binary data
 *              to/from base64
 *
 * Copyright (c) 2006-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include "base64.h"

#include <QBuffer>
#include <QDataStream>

namespace lmms::base64
{


QVariant decode( const QString & _b64, QVariant::Type _force_type )
{
	char * dst = nullptr;
	int dsize = 0;
	base64::decode( _b64, &dst, &dsize );
	QByteArray ba( dst, dsize );
	QBuffer buf( &ba );
	buf.open( QBuffer::ReadOnly );
	QDataStream in( &buf );
	QVariant ret;
	in >> ret;
	if( _force_type != QVariant::Invalid && ret.type() != _force_type )
	{
		buf.reset();
		in.setVersion( QDataStream::Qt_3_3 );
		in >> ret;
	}
	delete[] dst;
	return( ret );
}


//TODO C++20: It *may* be possible to make the following functions constepxr given C++20's constexpr std::string.

/**
 * @brief Base64 encodes data.
 * @param data
 * @return std::string containing the Base64 encoded data.
 */
auto encode(std::string_view data) -> std::string
{
	if (data.empty()) { return ""; }

	// base64 encoded string
	std::string result;

	// number of chunks to process + padding
	auto div_result = std::div(data.length(), numBytesPerChunk);
	auto numChunks = div_result.quot;
	auto numTrailingBytes = div_result.rem;

	// add 1 chunk to handle last padded chunk
	if (numTrailingBytes) { ++numChunks; }

	// allocate one time to prevent in-loop reallocations
	result.reserve(numChunks * numBase64CharPerChunk);

	/*
		char1               char2               char3
		7 6 5 4 3 2   1 0 | 7 6 5 4   3 2 1 0 | 7 6   5 4 3 2 1 0
		5 4 3 2 1 0 | 5 4   3 2 1 0 | 5 4 3 2   1 0 | 5 4 3 2 1 0
		b64c1         b64c2           b64c3           b64c4
	*/
	// Get the first base64 character offset from the char chunk
	auto b64char1 = [](std::string_view chunk) {
		return static_cast<std::string::value_type>(chunk[0] >> 2);
	};
	// Get the second base64 character offset from the char chunk
	auto b64char2 = [](std::string_view chunk) {
		return static_cast<std::string::value_type>(
			((chunk[0] & 0x03) << 4)
			|
			(chunk.size() > 0 ? ((chunk[1] & 0xF0) >> 4) : 0)
		);
	};
	// Get the third base64 character offset from the char chunk
	auto b64char3 = [](std::string_view chunk) {
		return static_cast<std::string::value_type>(
			(chunk.size() > 0 ? ((chunk[1] & 0x0F) << 2) : 0)
			|
			(chunk.size() > 1 ? ((chunk[2] & 0xC0) >> 6) : 0)
		);
	};
	// Get the fourth base64 character offset from the char chunk
	auto b64char4 = [](std::string_view chunk) {
		return static_cast<std::string::value_type>(chunk.size() > 1 ? (chunk[2] & 0x3F) : 0);
	};
	for (int currentChunk = 0; currentChunk < numChunks; ++currentChunk)
	{
		std::string_view chunk = data.substr(currentChunk * numBytesPerChunk, numBytesPerChunk);
		std::array<char, 4> output{
			map[b64char1(chunk)],
			map[b64char2(chunk)],
			pad,
			pad
		};
		switch (chunk.length()) {
			case 3:
				output[3] = map[b64char4(chunk)];
			case 2:
				output[2] = map[b64char3(chunk)];
			default: /* no-op */;
		};
		result.append(output.data(), output.size());
	}
	return result;
}

/**
 * @brief Decodes data in Base64.
 * @param data
 * @return std::string containing the original data.
 */
auto decode(std::string_view data) -> std::string
{
	if (data.empty()) { return ""; }
	if (data.length() % numBase64CharPerChunk != 0) {
		throw std::length_error("base64::decode : data length not a multiple of 4");
	}

	std::string result;

	// number of chunks to process
	auto numChunks = std::div(data.length(), numBase64CharPerChunk);

	// allocate one time to prevent in-loop reallocations
	result.reserve(numChunks.quot * numBytesPerChunk);
	/*
		char1               char2               char3
		7 6 5 4 3 2   1 0 | 7 6 5 4   3 2 1 0 | 7 6   5 4 3 2 1 0
		5 4 3 2 1 0 | 5 4   3 2 1 0 | 5 4 3 2   1 0 | 5 4 3 2 1 0
		b64c1         b64c2           b64c3           b64c4
	*/
	// Get the first character from the base64 chunk
	auto char1 = [](std::string_view chunk) {
		return static_cast<std::string::value_type>(
			(rmap.at(chunk[0]) << 2) | (rmap.at(chunk[1]) >> 4)
		);
	};
	// Get the second character from the base64 chunk
	auto char2 = [](std::string_view chunk) {
		return static_cast<std::string::value_type>(
			chunk[2] == pad
			? 0
			: ((rmap.at(chunk[1]) & 0x0F) << 4) | (rmap.at(chunk[2]) >> 2)
		);
	};
	// Get the third character from the base64 chunk
	auto char3 = [](std::string_view chunk) {
		return static_cast<std::string::value_type>(
			chunk[3] == pad
			? 0
			: ((rmap.at(chunk[2]) & 0x03) << 6) | rmap.at(chunk[3])
		);
	};
	for (int currentChunk = 0; currentChunk < numChunks.quot; ++currentChunk) {
		std::string_view chunk = data.substr(currentChunk * numBase64CharPerChunk, numBase64CharPerChunk);
		result.push_back(char1(chunk));
		result.push_back(char2(chunk));
		result.push_back(char3(chunk));
	}
	return result;
}

} // namespace lmms::base64
