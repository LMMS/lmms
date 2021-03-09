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
#ifndef WTF8_TERMIO_H_INCLUDED_
#define WTF8_TERMIO_H_INCLUDED_

#ifdef __cplusplus
#include <cstdio>
#include <istream>
#include <ostream>
#else
#include <stdio.h>
#endif

#ifdef __cplusplus
namespace WTF8 {

extern std::istream &cin;
extern std::ostream &cout;
extern std::ostream &cerr;
extern std::ostream &clog;

#ifdef _WIN32
static inline int fgetc(std::FILE *stream) {
    if(stream == stdin)
        return cin.get();
    else
        return std::fgetc(stream);
}

static inline char *fgets(char *s, int size, std::FILE *stream) {
    if(stream == stdin) {
        if(size == 0)
            return s;
        char *cur = s;
        while(--size != 0) {
            if(!cin.get(*cur)) {
                *cur = '\0';
                return cur == s ? nullptr : s;
            } else if(*cur == '\n') {
                cur[1] = '\0';
                return s;
            } else
                ++cur;
        }
        *cur = '\0';
        return s;
    } else
        return std::fgets(s, size, stream);
}

static inline int ungetc(int c, std::FILE *stream) {
    if(stream == stdin)
        return cin.putback(char(c)) ? c : EOF;
    else
        return std::ungetc(c, stream);
}

static inline int fputc(int c, std::FILE *stream) {
    if(stream == stdout)
        return cout.put(char(c)) ? c : EOF;
    else if(stream == stderr)
        return cerr.put(char(c)) ? c : EOF;
    else
        return std::fputc(c, stream);
}

static inline int fputs(const char *s, std::FILE *stream) {
    if(stream == stdout)
        return cout << s ? 0 : EOF;
    else if(stream == stderr)
        return cout << s ? 0 : EOF;
    else
        return std::fputs(s, stream);
}

static inline int feof(std::FILE *stream) {
    if(stream == stdin)
        return cin.eof();
    else
        return std::feof(stream);
}

static inline int getchar() {
    return WTF8::fgetc(stdin);
}

static inline int putchar(int c) {
    return WTF8::fputc(c, stdout);
}

static inline int puts(const char *s) {
    if(!(cout << s))
        return EOF;
    return std::endl(cout) ? 0 : EOF;
}
#else
using std::fgetc;
using std::fgets;
using std::ungetc;
using std::fputc;
using std::fputs;
using std::feof;
using std::getchar;
using std::putchar;
using std::puts;
#endif

};
#endif

#ifdef __cplusplus
extern "C" {
int WTF8_fgetc(std::FILE *stream);
char *WTF8_fgets(char *s, int size, std::FILE *stream);
int WTF8_ungetc(int c, std::FILE *stream);
int WTF8_fputc(int c, std::FILE *stream);
int WTF8_fputs(const char *s, std::FILE *stream);
int WTF8_feof(std::FILE *stream); 
int WTF8_puts(const char *s); 
}
#else
int WTF8_fgetc(FILE *stream);
char *WTF8_fgets(char *s, int size, FILE *stream);
int WTF8_ungetc(int c, FILE *stream);
int WTF8_fputc(int c, FILE *stream);
int WTF8_fputs(const char *s, FILE *stream);
int WTF8_feof(FILE *stream); 
int WTF8_puts(const char *s); 
#endif

#ifdef _WIN32
static inline int WTF8_getchar(void) {
    return getchar();
}

static inline int WTF8_putchar(int c) {
    return putchar(c);
}
#else
static inline int WTF8_getchar(void) {
    return WTF8_fgetc(stdin);
}

static inline int WTF8_putchar(int c) {
    return WTF8_fputc(c, stdout);
}
#endif

#endif
