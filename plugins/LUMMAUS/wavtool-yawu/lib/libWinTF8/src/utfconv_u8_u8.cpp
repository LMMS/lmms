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

static bool utf8_check_continuation(const std::string &utf8str, size_t start, size_t check_length) {
    if(utf8str.size() > start + check_length) {
        while(check_length--)
            if((uint8_t(utf8str[++start]) & 0xc0) != 0x80)
                return false;
        return true;
    } else
        return false;
}

std::string utf8_validify(const std::string &utf8str, bool strict) {
    std::string validstr;
    size_t i = 0;
    validstr.reserve(utf8str.size());
    while(i < utf8str.size()) {
        if(uint8_t(utf8str[i]) < 0x80) {
            validstr.push_back(utf8str[i]);
            ++i;
            continue;
        } else if(uint8_t(utf8str[i]) < 0xc0) {
        } else if(uint8_t(utf8str[i]) < 0xe0) {
            if(utf8_check_continuation(utf8str, i, 1)) {
                uint32_t ucs4 = uint32_t(utf8str[i] & 0x1f) << 6 | uint32_t(utf8str[i+1] & 0x3f);
                if(ucs4 >= 0x80) {
                    validstr.append({utf8str[i], utf8str[i+1]});
                    i += 2;
                    continue;
                }
            }
        } else if(uint8_t(utf8str[i]) < 0xf0) {
            if(utf8_check_continuation(utf8str, i, 2)) {
                uint32_t ucs4 = uint32_t(utf8str[i] & 0xf) << 12 | uint32_t(utf8str[i+1] & 0x3f) << 6 | (utf8str[i+2] & 0x3f);
                if(ucs4 >= 0x800 && (ucs4 & 0xf800) != 0xd800) {
                    validstr.append({utf8str[i], utf8str[i+1], utf8str[i+2]});
                    i += 3;
                    continue;
                }
            }
        } else if(uint8_t(utf8str[i]) < 0xf8) {
            if(utf8_check_continuation(utf8str, i, 3)) {
                uint32_t ucs4 = uint32_t(utf8str[i] & 0x7) << 18 | uint32_t(utf8str[i+1] & 0x3f) << 12 | uint32_t(utf8str[i+2] & 0x3f) << 6 | uint32_t(utf8str[i+3] & 0x3f);
                if(ucs4 >= 0x10000 && ucs4 < 0x110000) {
                    validstr.append({utf8str[i], utf8str[i+1], utf8str[i+2], utf8str[i+3]});
                    i += 4;
                    continue;
                }
            }
        }
        if(strict)
            throw unicode_conversion_error();
        else {
            validstr.append("\xef\xbf\xbd", 3);
            ++i;
        }
    }
    validstr.shrink_to_fit();
    return validstr;
}

}

extern "C" {

size_t WTF8_validify(char *validstr, const char *utf8str, int strict, size_t bufsize) {
    try {
        std::string validstrpp = WTF8::utf8_validify(std::string(utf8str), strict != 0);
        if(validstr && bufsize != 0) {
            std::memcpy(validstr, validstrpp.data(), std::min(validstrpp.length(), bufsize-1)*sizeof (char));
            validstr[std::min(validstrpp.length(), bufsize-1)] = '\0';
        }
        return validstrpp.length();
    } catch(WTF8::unicode_conversion_error) {
        return WTF8_UNICODE_CONVERT_ERROR;
    }
}

}
