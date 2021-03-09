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
#ifndef WTF8_FILEIO_H_INCLUDED_
#define WTF8_FILEIO_H_INCLUDED_

#ifdef __cplusplus
#include <cstdio>
#else
#include <stdio.h>
#endif

#ifdef __cplusplus
namespace WTF8 {
int access(const char *path, int amode);
std::FILE *fopen(const char *path, const char *mode);
std::FILE *freopen(const char *path, const char *mode, std::FILE *fp);
std::FILE *fclose(std::FILE *fp);
int remove(const char *path);
int rename(const char *oldname, const char *newname);
}
#endif

#ifdef __cplusplus
extern "C" {
std::FILE *WTF8_fopen(const char *path, const char *mode);
std::FILE *WTF8_freopen(const char *path, const char *mode, std::FILE* fp);
std::FILE *WTF8_fclose(std::FILE *fp);
}
#else
FILE *WTF8_fopen(const char *path, const char *mode);
FILE *WTF8_freopen(const char *path, const char *mode, FILE *fp);
FILE *WTF8_fclose(FILE *fp);
#endif

#ifdef __cplusplus
extern "C" {
#endif
int WTF8_access(const char *path, int amode);
int WTF8_remove(const char *path);
int WTF8_rename(const char *oldname, const char *newname);
#ifdef __cplusplus
}
#endif

#endif
