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

#ifndef LMMS_IO_HELPER_H
#define LMMS_IO_HELPER_H

#include "lmmsconfig.h"

#include <cstdio>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>

#ifdef _WIN32
#include <windows.h>
#endif

#ifdef LMMS_BUILD_WIN32
#include <io.h>
#else
#ifdef LMMS_HAVE_UNISTD_H
#include <unistd.h>
#endif
#endif // LMMS_BUILD_WIN32

namespace lmms
{


#ifdef _WIN32

// NOTE: Not using std::wstring because it does not work correctly when building with wineg++
inline std::unique_ptr<wchar_t[]> toWString(const std::string& utf8)
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

	ret = std::make_unique<wchar_t[]>(result);
	result = ::MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8.data(), utf8Len, ret.get(), result);
	if (result <= 0)
	{
		const DWORD error = ::GetLastError();
		throw std::invalid_argument{"toWString: failed to convert. error code: " + std::to_string(error)};
	}

	return ret;
}

#endif


inline FILE* F_OPEN_UTF8(std::string const& fname, const char* mode){
#ifdef LMMS_BUILD_WIN32
	return _wfopen(toWString(fname).get(), toWString(mode).get());
#else
	return fopen(fname.data(), mode);
#endif
}


inline int fileToDescriptor(FILE* f, bool closeFile = true)
{
	if (f == nullptr) {return -1;}

#ifdef LMMS_BUILD_WIN32
	int fh = _dup(_fileno(f));
#else
	int fh = dup(fileno(f));
#endif

	if (closeFile) {fclose(f);}
	return fh;
}


} // namespace lmms

#endif // LMMS_IO_HELPER_H
