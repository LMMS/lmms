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
#include <stdexcept>
#include <fstream>
#include <vector>
#include "utils.h"
#include "u8str.h"
#include "argv.h"

#if defined(_WIN32)
#include <windows.h>
#elif defined(__APPLE__) && defined(__MACH__)
extern "C" {
    extern int *_NSGetArgc();
    extern char ***_NSGetArgv();
}
#endif

namespace WTF8 {

std::vector<u8string> getargv() {
#if defined(_WIN32)
    int argc;
    wchar_t **wargv = CommandLineToArgvW(GetCommandLineW(), &argc);
    std::vector<u8string> result;
    result.reserve(argc);
    for(int i = 0; i < argc; ++i)
        result.push_back(u8string::from_wide(wargv[i]));
    LocalFree(static_cast<void *>(wargv));
    return result;
#elif defined(__APPLE__) && defined(__MACH__)
    int argc = *_NSGetArgc();
    char **argv = *_NSGetArgv();
    std::vector<u8string> result;
    result.reserve(argc);
    for(int i = 0; i < argc; ++i)
        result.push_back(u8string(argv[i]));
    return result;
#else
    std::ifstream cmdline("/proc/self/cmdline");
    if(cmdline.is_open()) {
        std::vector<u8string> result;
        for(;;) {
            u8string argi;
            for(;;) {
                char c;
                if(cmdline.get(c))
                    if(c != '\0')
                        argi.push_back(c);
                    else
                        break;
                else if(cmdline.eof() && argi.empty())
                    return result;
                else
                    throw std::runtime_error("Unable to get commandline arguments");
            }
            result.push_back(std::move(argi));
        }
    } else
        throw std::runtime_error("Unable to get commandline arguments");
#endif
}

}

extern "C" {

char **WTF8_getargv(int *argc) {
#if defined(_WIN32)
    int argc_;
    wchar_t **wargv = CommandLineToArgvW(GetCommandLineW(), &argc_);
    if(argc)
        *argc = argc_;
    char **result = new char *[argc_+1];
    for(int i = 0; i < argc_; ++i)
        result[i] = WTF8::new_c_str(WTF8::u8string::from_wide(wargv[i]));
    result[argc_] = nullptr;
    LocalFree(static_cast<void *>(wargv));
    return result;
#elif defined(__APPLE__) && defined(__MACH__)
    if(argc)
        *argc = *_NSGetArgc();
    return *_NSGetArgv();
#else
    try {
        std::vector<WTF8::u8string> argv = WTF8::getargv();
        if(argc)
            *argc = argv.size();
        char **result = new char *[argv.size()+1];
        for(size_t i = 0; i < argv.size(); ++i)
            result[i] = WTF8::new_c_str(argv.at(i));
        result[argv.size()] = nullptr;
        return result;
    } catch(std::runtime_error) {
        if(argc)
            *argc = 0;
        return nullptr;
    }
#endif
}

char **WTF8_freeargv(char **argv) {
#if !defined(__APPLE__) || !defined(__MACH__)
    if(argv) {
        for(size_t i = 0; argv[i]; ++i)
            argv[i] = WTF8::delete_c_str(argv[i]);
        delete[] argv;
    }
#else
    WTF8::unused_arg(argv);
#endif
    return nullptr;
}

}
