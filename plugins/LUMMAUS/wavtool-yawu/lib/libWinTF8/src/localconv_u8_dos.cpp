/*
  Copyright (c) 2014 StarBrilliant <m13253@hotmail.com>
  All rights reserved.

  Redistribution and use in source and binary forms are permitted
  provided that the above copyright notice and this paragraph are
  duplicated in all such forms and that any documentation,
  advertising materials, and other materials related to such
  distribution and use acknowledge that the software was developed by
  StarBrilliant.
  The name of StarBrilliant may not be used to endorse or promote
  products derived from this software without specific prior written
  permission.

  THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
  IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
*/
#include <algorithm>
#include <cstddef>
#include <cstring>
#include <vector>
#include "u8str.h"
#include "utils.h"
#include "utfconv.h"
#include "localconv.h"

#ifdef _WIN32
#include <windows.h>
/* As of MingW-w64 3.3.0, WC_ERR_INVALID_CHARS is still missing. */
/* See http://sourceforge.net/p/mingw-w64/mailman/message/28541441/ */
#ifndef WC_ERR_INVALID_CHARS
#define WC_ERR_INVALID_CHARS 0x00000080
#endif
#endif

namespace WTF8 {

std::string utf8_to_dos_filename(const std::string &utf8_filename) {
#ifdef _WIN32
    if(utf8_filename.empty())
        return utf8_filename;
    try {
        return utf8_to_local(utf8_filename, true);
    } catch(unicode_conversion_error) {
    }
    std::wstring wide_filename = utf8_to_wide(utf8_filename, true);
    DWORD wide_dos_size = GetShortPathNameW(wide_filename.c_str(), nullptr, 0);
    if(wide_dos_size == 0)
        throw unicode_conversion_error();
    std::vector<wchar_t> wide_dos_buffer(wide_dos_size);
    wide_dos_size = GetShortPathNameW(wide_filename.c_str(), wide_dos_buffer.data(), DWORD(wide_dos_buffer.size()));
    if(wide_dos_size == 0)
        throw unicode_conversion_error();
    BOOL used_replace_char = false;
    int dos_size = WideCharToMultiByte(CP_ACP, WC_ERR_INVALID_CHARS | WC_NO_BEST_FIT_CHARS, wide_dos_buffer.data(), wide_dos_size, nullptr, 0, nullptr, &used_replace_char);
    if(dos_size == 0 || !!used_replace_char)
        throw unicode_conversion_error();
    std::vector<char> dos_buffer(dos_size);
    dos_size = WideCharToMultiByte(CP_ACP, WC_ERR_INVALID_CHARS | WC_NO_BEST_FIT_CHARS, wide_dos_buffer.data(), wide_dos_size, dos_buffer.data(), int(dos_buffer.size()), nullptr, nullptr);
    return std::string(dos_buffer.data(), dos_size);
#else
    return utf8_to_local(utf8_filename, true);
#endif
}

}

extern "C" {

size_t WTF8_utf8_to_dos_filename(char *dos_filename, const char *utf8_filename, size_t bufsize) {
#ifdef _WIN32
    try {
        std::string dosstrpp = WTF8::utf8_to_dos_filename(std::string(utf8_filename));
        if(dos_filename && bufsize != 0) {
            std::memcpy(dos_filename, dosstrpp.data(), (std::min)(dosstrpp.length(), bufsize-1)*sizeof (char));
            dos_filename[(std::min)(dosstrpp.length(), bufsize-1)] = '\0';
        }
        return dosstrpp.length();
    } catch(WTF8::unicode_conversion_error) {
        return WTF8_UNICODE_CONVERT_ERROR;
    }
#else
    return WTF8_utf8_to_local(dos_filename, utf8_filename, true, bufsize);
#endif
}

}
