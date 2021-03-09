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
#endif

namespace WTF8 {

std::string local_to_utf8(const std::string &localstr, bool strict) {
#ifdef _WIN32
    if(localstr.empty())
        return localstr;
    int wide_size = MultiByteToWideChar(CP_ACP, strict ? MB_ERR_INVALID_CHARS : 0, localstr.data(), int(localstr.length()), nullptr, 0);
    if(wide_size == 0)
        throw unicode_conversion_error();
    std::vector<wchar_t> wide_buffer(wide_size);
    wide_size = MultiByteToWideChar(CP_ACP, strict ? MB_ERR_INVALID_CHARS : 0, localstr.data(), int(localstr.length()), wide_buffer.data(), int(wide_buffer.size()));
    return wide_to_utf8(std::wstring(wide_buffer.data(), wide_size), strict);
#else
    if(strict)
        return utf8_validify(localstr, strict);
    else
        return localstr;
#endif
}

}

extern "C" {

size_t WTF8_local_to_utf8(char *utf8str, const char *localstr, int strict, size_t bufsize) {
#ifdef _WIN32
    try {
        std::string utf8strpp = WTF8::local_to_utf8(std::string(localstr), strict != 0);
        if(utf8str && bufsize != 0) {
            std::memcpy(utf8str, utf8strpp.data(), (std::min)(utf8strpp.length(), bufsize-1)*sizeof (char));
            utf8str[(std::min)(utf8strpp.length(), bufsize-1)] = '\0';
        }
        return utf8strpp.length();
    } catch(WTF8::unicode_conversion_error) {
        return WTF8_UNICODE_CONVERT_ERROR;
    }
#else
    if(strict != 0)
        return WTF8_utf8_validify(utf8str, localstr, strict, bufsize);
    else {
        size_t locallen = std::strlen(localstr);
        if(utf8str && bufsize != 0) {
            std::memcpy(utf8str, localstr, std::min(locallen, bufsize-1)*sizeof (char));
            utf8str[std::min(locallen, bufsize-1)] = '\0';
        }
        return locallen;
    }
#endif
}

}
