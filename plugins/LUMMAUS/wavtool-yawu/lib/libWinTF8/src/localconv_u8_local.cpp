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

std::string utf8_to_local(const std::string &utf8str, bool strict) {
#ifdef _WIN32
    if(utf8str.empty())
        return utf8str;
    std::wstring widestr = utf8_to_wide(utf8str, strict);
    BOOL used_replace_char = false;
    int local_size = WideCharToMultiByte(CP_ACP, strict ? WC_ERR_INVALID_CHARS | WC_NO_BEST_FIT_CHARS : 0, widestr.data(), int(widestr.length()), nullptr, 0, nullptr, &used_replace_char);
    if(local_size == 0 || (strict && !!used_replace_char))
        throw unicode_conversion_error();
    std::vector<char> local_buffer(local_size);
    local_size = WideCharToMultiByte(CP_ACP, strict ? WC_ERR_INVALID_CHARS | WC_NO_BEST_FIT_CHARS : 0, widestr.data(), int(widestr.length()), local_buffer.data(), int(local_buffer.size()), nullptr, nullptr);
    return std::string(local_buffer.data(), local_size);
#else
    if(strict)
        return utf8_validify(utf8str, strict);
    else
        return utf8str;
#endif
}

}

extern "C" {

size_t WTF8_utf8_to_local(char *localstr, const char *utf8str, int strict, size_t bufsize) {
#ifdef _WIN32
    try {
        std::string localstrpp = WTF8::utf8_to_local(std::string(utf8str), strict != 0);
        if(localstr && bufsize != 0) {
            std::memcpy(localstr, localstrpp.data(), (std::min)(localstrpp.length(), bufsize-1)*sizeof (char));
            localstr[(std::min)(localstrpp.length(), bufsize-1)] = '\0';
        }
        return localstrpp.length();
    } catch(WTF8::unicode_conversion_error) {
        return WTF8_UNICODE_CONVERT_ERROR;
    }
#else
    if(strict != 0)
        return WTF8_utf8_validify(localstr, utf8str, strict, bufsize);
    else {
        size_t utf8len = std::strlen(utf8str);
        if(localstr && bufsize != 0) {
            std::memcpy(localstr, utf8str, std::min(utf8len, bufsize-1)*sizeof (char));
            localstr[std::min(utf8len, bufsize-1)] = '\0';
        }
        return utf8len;
    }
#endif
}

}
