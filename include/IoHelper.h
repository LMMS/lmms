/*
 * IoHelper.h - helper functions for file I/O
 *
 * Copyright (c) 2018 Hyunjin Song <tteu.ingog/at/gmail.com>
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

// NOTE: The LMMS/zynaddsubfx repo contains a copy of this header.
//       If you modify this file, consider modifying it there as well.

#ifndef LMMS_IO_HELPER_H
#define LMMS_IO_HELPER_H

#include <cstdio>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>

#ifdef _WIN32
#	ifndef NOMINMAX
#		define NOMINMAX
#	endif
#	include <windows.h>
#endif

#if defined(_WIN32) && !defined(__WINE__)
#	include <io.h>
#elif __has_include(<unistd.h>)
#	include <unistd.h>
#endif

namespace lmms
{


#ifdef _WIN32

/**
 * UTF-8 to wide string conversion
 * NOTE: Avoids using std::wstring because it does not work correctly with wineg++
 */
inline std::unique_ptr<wchar_t[]> toWString(std::string_view utf8)
{
	std::unique_ptr<wchar_t[]> ret;
	if (utf8.empty())
	{
		ret = std::make_unique<wchar_t[]>(1);
		return ret;
	}

	if (utf8.length() > static_cast<std::size_t>(std::numeric_limits<int>::max()))
	{
		throw std::overflow_error{"toWString: input string is too long"};
	}
	const auto utf8Len = static_cast<int>(utf8.length());

	int result = ::MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8.data(), utf8Len, nullptr, 0);
	if (result <= 0)
	{
		const DWORD error = ::GetLastError();
		throw std::invalid_argument{"toWString: failed to get size of result string. error code: " + std::to_string(error)};
	}

	ret = std::make_unique<wchar_t[]>(result + 1); // includes null terminator
	result = ::MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8.data(), utf8Len, ret.get(), result);
	if (result <= 0)
	{
		const DWORD error = ::GetLastError();
		throw std::invalid_argument{"toWString: failed to convert. error code: " + std::to_string(error)};
	}

	return ret;
}

#endif // _WIN32

//! std::fopen wrapper that expects UTF-8 encoded `filename` and `mode`
inline std::FILE* fopenUtf8(const std::string& filename, const char* mode)
{
#if defined(_WIN32) && !defined(__WINE__)
	return _wfopen(toWString(filename).get(), toWString(mode).get());
#else
	return std::fopen(filename.c_str(), mode);
#endif
}

//! Returns the POSIX file descriptor of the given FILE
inline int fileToDescriptor(std::FILE* file, bool closeFile = true)
{
	if (file == nullptr) { return -1; }

#if defined(_WIN32) && !defined(__WINE__)
	int fh = _dup(_fileno(file));
#else
	int fh = dup(fileno(file));
#endif

	if (closeFile) { std::fclose(file); }
	return fh;
}


} // namespace lmms

#endif // LMMS_IO_HELPER_H
