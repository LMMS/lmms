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
#include "utils.h"
#include "u8str.h"
#include "utfconv.h"
#include "fileio.h"

#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

namespace WTF8 {

int access(const char *path, int amode) {
#ifdef _WIN32
    try {
        return _waccess_s(u8string(path).to_wide(true).c_str(), amode);
    } catch(unicode_conversion_error) {
        errno = EINVAL;
        return -1;
    }
#else
    return ::access(path, amode);
#endif
}

}

extern "C" {

int WTF8_access(const char *path, int amode) {
    return WTF8::access(path, amode);
}

}
