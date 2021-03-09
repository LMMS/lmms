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
#include "initcon.h"

#ifdef _WIN32
#include <windows.h>
#endif

namespace WTF8 {

#ifdef _WIN32
static bool console_already_set = false;

static bool test_windows_version_vista() {
    OSVERSIONINFOEXW os_version_info = { sizeof os_version_info, 6 };
    ULONGLONG ver_set_condition_mask = 0;
    ver_set_condition_mask = VerSetConditionMask(ver_set_condition_mask, VER_MAJORVERSION, VER_GREATER_EQUAL);
    ver_set_condition_mask = VerSetConditionMask(ver_set_condition_mask, VER_MINORVERSION, VER_GREATER_EQUAL);
    ver_set_condition_mask = VerSetConditionMask(ver_set_condition_mask, VER_SERVICEPACKMAJOR, VER_GREATER_EQUAL);
    return !!VerifyVersionInfoW(&os_version_info, VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR, ver_set_condition_mask);
}
#endif

bool set_console_font() {
#ifdef _WIN32
    if(!console_already_set) {
        console_already_set = true;
        if(test_windows_version_vista()) {
            HMODULE kernel32_dll = LoadLibraryW(L"kernel32.dll");
            if(kernel32_dll) {
                auto dynamic_GetCurrentConsoleFontEx = reinterpret_cast<decltype(GetCurrentConsoleFontEx) *>(GetProcAddress(kernel32_dll, "GetCurrentConsoleFontEx"));
                auto dynamic_SetCurrentConsoleFontEx = reinterpret_cast<decltype(SetCurrentConsoleFontEx) *>(GetProcAddress(kernel32_dll, "SetCurrentConsoleFontEx"));
                if(dynamic_GetCurrentConsoleFontEx && dynamic_SetCurrentConsoleFontEx) {
                    HANDLE stdout_handle = GetStdHandle(STD_OUTPUT_HANDLE);
                    CONSOLE_FONT_INFOEX console_font_info = { sizeof console_font_info };
                    if(dynamic_GetCurrentConsoleFontEx(stdout_handle, false, &console_font_info)) {
                        console_font_info.nFont = 10;
                        console_font_info.FontFamily = FF_DONTCARE;
                        console_font_info.FontWeight = FW_NORMAL;
                        wmemcpy(console_font_info.FaceName, L"Lucida Console", 15);
                        return !!dynamic_SetCurrentConsoleFontEx(stdout_handle, false, &console_font_info);
                    }
                }
            }
        }
    }
#endif
    return false;
}

}

extern "C" {

int WTF8_set_console_font(void) {
    return WTF8::set_console_font();
}

}
