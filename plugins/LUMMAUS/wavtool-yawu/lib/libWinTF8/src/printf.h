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
#pragma once
#ifndef WTF8_PRINTF_H_INCLUDED_
#define WTF8_PRINTF_H_INCLUDED_

#ifdef __cplusplus
#include <cstdarg>
#include <cstddef>
#include <string>
#else
#include <stdarg.h>
#include <stddef.h>
#endif

#ifdef __cplusplus
namespace WTF8 {
int printf(const char *format, ...);
int vprintf(const char *format, va_list ap);
int fprintf(std::FILE *stream, const char *format, ...);
int vfprintf(std::FILE *stream, const char *format, va_list ap);
}
#endif

#ifdef __cplusplus
extern "C" {
int WTF8_printf(const char *format, ...);
int WTF8_vprintf(const char *format, va_list ap);
int WTF8_fprintf(std::FILE *stream, const char *format, ...);
int WTF8_vfprintf(std::FILE *stream, const char *format, va_list ap);
}
#else
int WTF8_printf(const char *format, ...);
int WTF8_vprintf(const char *format, va_list ap);
int WTF8_fprintf(FILE *stream, const char *format, ...);
int WTF8_vfprintf(FILE *stream, const char *format, va_list ap);
#endif

#endif
