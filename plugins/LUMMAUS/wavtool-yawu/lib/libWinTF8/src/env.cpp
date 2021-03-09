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
#include <cstdlib>
#include <string>
#include <vector>
#include "utils.h"
#include "u8str.h"

#ifdef _WIN32
#include <windows.h>
#endif

namespace WTF8 {

const char *getenv(const char *varname) {
#ifdef _WIN32
    std::wstring varname_ = u8string(varname).to_wide();
    std::vector<wchar_t> buffer(1024);
    DWORD size = GetEnvironmentVariableW(varname_.c_str(), buffer.data(), DWORD(buffer.size()));
    if(size == 0)
        return nullptr;
    else if(size_t(size) >= buffer.size()) {
        buffer.clear();
        buffer.resize(size); /* size includes the terminating NUL */
        size = GetEnvironmentVariableW(varname_.c_str(), buffer.data(), DWORD(buffer.size()));
    }
    if(size == 0)
        return nullptr;
    else
        return new_c_str(u8string::from_wide(std::wstring(buffer.data(), size)));
#else
    return std::getenv(varname);
#endif
}

const char *freeenv(const char *envstring) {
#ifdef _WIN32
    delete_c_str(envstring);
#else
    unused_arg(envstring);
#endif
    return nullptr;
}

int setenv(const char *varname, const char *value) {
#ifdef _WIN32
    if(SetEnvironmentVariableW(u8string(varname).to_wide().c_str(), u8string(value).to_wide().c_str()))
        return 0;
    else {
        errno = EINVAL;
        return -1;
    }
#else
    return ::setenv(varname, value, true);
#endif
}

int unsetenv(const char *varname) {
#ifdef _WIN32
    if(SetEnvironmentVariableW(u8string(varname).to_wide().c_str(), nullptr))
        return 0;
    else {
        errno = EINVAL;
        return -1;
    }
#else
    return ::unsetenv(varname);
#endif
}

}

extern "C" {

const char *WTF8_getenv(const char *name) {
    return WTF8::new_c_str(WTF8::getenv(name));
}

const char *WTF8_freeenv(const char *envstring) {
    return WTF8::freeenv(envstring);
}

int WTF8_setenv(const char *name, const char *value) {
    return WTF8::setenv(name, value);
}

int WTF8_unsetenv(const char *name) {
    return WTF8::unsetenv(name);
}

}
