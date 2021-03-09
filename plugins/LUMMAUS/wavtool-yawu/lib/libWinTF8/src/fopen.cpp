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
#include <cerrno>
#include <cstdio>
#include "utils.h"
#include "u8str.h"
#include "utfconv.h"
#include "fileio.h"

#ifdef _WIN32
#include <share.h>
#endif

namespace WTF8 {

std::FILE *fopen(const char *path, const char *mode) {
#ifdef _WIN32
    try {
        return _wfsopen(u8string(path).to_wide(true).c_str(), u8string(mode).to_wide(true).c_str(), _SH_DENYNO);
    } catch(unicode_conversion_error) {
        errno = EINVAL;
        return nullptr;
    }
#else
    return std::fopen(path, mode);
#endif
}

}

extern "C" {

std::FILE *WTF8_fopen(const char *path, const char *mode) {
    return WTF8::fopen(path, mode);
}

}
