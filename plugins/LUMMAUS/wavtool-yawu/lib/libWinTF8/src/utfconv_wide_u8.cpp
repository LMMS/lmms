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
#include <cstdint>
#include <cstring>
#include <string>
#include "utils.h"
#include "utfconv.h"

namespace WTF8 {

std::string wide_to_utf8(const std::wstring &widestr, bool strict) {
    std::string utf8str;
    size_t i = 0;
    utf8str.reserve(widestr.size()*4);
    while(i < widestr.size()) {
        if(uint32_t(widestr[i]) < 0x80) {
            utf8str.push_back(char(widestr[i]));
            ++i;
        } else if(uint32_t(widestr[i]) < 0x800) {
            utf8str.append({
                char(uint32_t(widestr[i]) >> 6 | 0xc0),
                char((uint32_t(widestr[i]) & 0x3f) | 0x80)
            });
            ++i;
        } else if(sizeof (wchar_t) >= 4) {
            if(uint32_t(widestr[i]) < 0x10000) {
                if((uint32_t(widestr[i]) & 0xf800) != 0xd800) {
                    utf8str.append({
                        char(uint32_t(widestr[i]) >> 12 | 0xe0),
                        char(((uint32_t(widestr[i]) >> 6) & 0x3f) | 0x80),
                        char((uint32_t(widestr[i]) & 0x3f) | 0x80)
                    });
                    ++i;
                } else if(strict) {
                    throw unicode_conversion_error();
                } else {
                    utf8str.append("\xef\xbf\xbd", 3);
                    ++i;
                }
            } else if(uint32_t(widestr[i]) < 0x110000) {
                utf8str.append({
                    char(uint32_t(widestr[i]) >> 18 | 0xf0),
                    char(((uint32_t(widestr[i]) >> 12) & 0x3f) | 0x80),
                    char(((uint32_t(widestr[i]) >> 6) & 0x3f) | 0x80),
                    char((uint32_t(widestr[i]) & 0x3f) | 0x80)
                });
                ++i;
            } else if(strict) {
                throw unicode_conversion_error();
            } else {
                utf8str.append("\xef\xbf\xbd", 3);
                ++i;
            }
        } else {
            if((uint16_t(widestr[i]) & 0xf800) != 0xd800) {
                    utf8str.append({
                        char(uint16_t(widestr[i]) >> 12 | 0xe0),
                        char(((uint16_t(widestr[i]) >> 6) & 0x3f) | 0x80),
                        char((uint16_t(widestr[i]) & 0x3f) | 0x80)
                    });
                    ++i;
            } else if(i+1 < widestr.size() && uint16_t(widestr[i] & 0xfc00) == 0xd800 && uint16_t(widestr[i+1] & 0xfc00) == 0xdc00) {
                uint32_t ucs4 = (uint32_t(widestr[i] & 0x3ff) << 10 | (widestr[i+1] & 0x3ff)) + 0x10000;
                utf8str.append({
                    char(ucs4 >> 18 | 0xf0),
                    char(((ucs4 >> 12) & 0x3f) | 0x80),
                    char(((ucs4 >> 6) & 0x3f) | 0x80),
                    char((ucs4 & 0x3f) | 0x80),
                });
                i += 2;
            } else if(strict) {
                throw unicode_conversion_error();
            } else {
                utf8str.append("\xef\xbf\xbd", 3);
                ++i;
            }
        }
    }
    utf8str.shrink_to_fit();
    return utf8str;
}

}

extern "C" {

size_t WTF8_wide_to_utf8(char *utf8str, const wchar_t *widestr, int strict, size_t bufsize) {
    try {
        std::string utf8strpp = WTF8::wide_to_utf8(std::wstring(widestr), strict != 0);
        if(utf8str && bufsize != 0) {
            std::memcpy(utf8str, utf8strpp.data(), std::min(utf8strpp.length(), bufsize-1)*sizeof (char));
            utf8str[std::min(utf8strpp.length(), bufsize-1)] = '\0';
        }
        return utf8strpp.length();
    } catch(WTF8::unicode_conversion_error) {
        return WTF8_UNICODE_CONVERT_ERROR;
    }
}

}
