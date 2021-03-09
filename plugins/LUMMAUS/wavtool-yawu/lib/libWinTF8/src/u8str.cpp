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
#include "utils.h"
#include "u8str.h"
#include "utfconv.h"

namespace WTF8 {

u8string::u8string(const std::wstring &s, bool strict) :
    u8string(wide_to_utf8(s, strict)) {
}

u8string::u8string(const wchar_t *s, bool strict) :
    u8string(wide_to_utf8(std::wstring(s), strict)) {
}

std::wstring u8string::to_wide(bool strict) const {
    return utf8_to_wide(*this, strict);
}

u8string u8string::validify(bool strict) const {
    return u8string(utf8_validify(*this, strict));
}

size_t u8string::count_codepoints(bool strict) const {
    return utf8_count_codepoints(*this, strict);
}

}
