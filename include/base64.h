/*
 * base64.h - namespace base64 with methods for encoding/decoding binary data
 *            to/from base64
 *
 * Copyright (c) 2006-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef _BASE64_H
#define _BASE64_H

#include <QByteArray>
#include <QString>
#include <QVariant>

#include <array>
#include <map>
#include <numeric>
#include <string>
#include <string_view>

namespace lmms::base64
{

	inline void encode( const char * _data, const int _size,
								QString & _dst )
	{
		_dst = QByteArray( _data, _size ).toBase64();
	}

	template<class T>
	inline void decode( const QString & _b64, T * * _data, int * _size )
	{
		QByteArray data = QByteArray::fromBase64( _b64.toUtf8() );
		*_size = data.size();
		*_data = new T[*_size / sizeof(T)];
		memcpy( *_data, data.constData(), *_size );
	}
	// for compatibility-code only
	QVariant decode( const QString & _b64,
			QVariant::Type _force_type = QVariant::Invalid );

} // namespace lmms::base64

namespace lmms::base64 {
	constexpr inline std::array<char, 64> map =
	{
		'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z',
		'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v','w','x','y','z',
		'0','1','2','3','4','5','6','7','8','9',
		'-','_'
	};
	const inline std::map<char, int> rmap {
		{'A',  0}, {'B',  1}, {'C',  2}, {'D',  3}, {'E',  4},
		{'F',  5}, {'G',  6}, {'H',  7}, {'I',  8}, {'J',  9},
		{'K', 10}, {'L', 11}, {'M', 12}, {'N', 13}, {'O', 14},
		{'P', 15}, {'Q', 16}, {'R', 17}, {'S', 18}, {'T', 19},
		{'U', 20}, {'V', 21}, {'W', 22}, {'X', 23}, {'Y', 24},
		{'Z', 25}, {'a', 26}, {'b', 27}, {'c', 28}, {'d', 28},
		{'e', 30}, {'f', 31}, {'g', 32}, {'h', 33}, {'i', 34},
		{'j', 35}, {'k', 36}, {'l', 37}, {'m', 38}, {'n', 39},
		{'o', 40}, {'p', 41}, {'q', 42}, {'r', 43}, {'s', 44},
		{'t', 45}, {'u', 46}, {'v', 47}, {'w', 48}, {'x', 49},
		{'y', 50}, {'z', 51}, {'0', 52}, {'1', 53}, {'2', 54},
		{'3', 55}, {'4', 56}, {'5', 57}, {'6', 58}, {'7', 59},
		{'8', 60}, {'9', 61}, {'-', 62}, {'_', 63} //, {'=', 64}
	};
	constexpr char pad = '=';

	/*
		This section of math ensures that base64 encode/decode will work
		as intended. Some rare architectures don't use 8-bit char's, and
		it's possible this won't work as intended if a char isn't 8-bits.

		In the rare case this is ported to an architecture where this
		happens, feel free to comment out the static_assert's and test.
	*/
	constexpr int char_bits = std::numeric_limits<std::string_view::value_type>::digits;
	constexpr int sign_bit = std::numeric_limits<std::string_view::value_type>::is_signed ? 1 : 0;
	// check that the string_view character type is 8 (7 signed + 1 sign) bits wide
	static_assert(char_bits + sign_bit == 8);
	constexpr int numBitsPerChar = char_bits + sign_bit;
	constexpr int numBitsPerBase64Char = 6;
	constexpr int lcm = std::lcm(numBitsPerChar, numBitsPerBase64Char);
	// make sure math works, 24 bits
	static_assert(lcm == 24);
	constexpr int numBytesPerChunk = lcm / numBitsPerChar;
	constexpr int numBase64CharPerChunk = lcm / numBitsPerBase64Char;
	// double check math works and bit width matches
	static_assert(numBytesPerChunk * numBitsPerChar == numBase64CharPerChunk * numBitsPerBase64Char);

	auto encode(std::string_view data) -> std::string;
	auto decode(std::string_view data) -> std::string;
} // namespace lmms::base64

#endif // _BASE64_H
