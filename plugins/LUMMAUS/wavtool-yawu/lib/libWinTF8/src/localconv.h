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
#ifndef WTF8_LOCALCONV_H_INCLUDED_
#define WTF8_LOCALCONV_H_INCLUDED_

#ifdef __cplusplus
#include <cstddef>
#include <string>
#else
#include <stddef.h>
#endif

#ifdef __cplusplus
namespace WTF8 {

/**
 * Convert a UTF-8 string to local charest (the so-called Windows ANSI)
 *
 * On platforms other than Windows, this is identical to `WTF8::utf8_validify`
 *
 * Throws:
 *   WTF8::unicode_conversion_error (when `strict` is set)
 */
std::string utf8_to_local(const std::string &utf8str, bool strict = false);

/**
 * Convert local charest string (the so-called Windows ANSI) to UTF-8
 *
 * On platforms other than Windows, this is identical to `WTF8::utf8_validify`
 *
 * Throws:
 *   WTF8::unicode_conversion_error (when `strict` is set)
 */
std::string local_to_utf8(const std::string &localstr, bool strict = false);

/**
 * Convert a UTF-8 filename to a DOS short filename
 *
 * When interacting with third-party libraries which does not support Unicode,
 * filenames that already exist may be converted to DOS short filename
 *
 * On platforms other than Windows, this is identical to strict `WTF8::utf8_validify`
 *
 * Throws:
 *   WTF8::unicode_conversion_error
 */
std::string utf8_to_dos_filename(const std::string &utf8_filename);

}

#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Convert a UTF-8 string to local charest (the so-called Windows ANSI)
 *
 * On platforms other than Windows, this is identical to `WTF8::utf8_validify`
 *
 * Results:
 *   The converted string is stored in `localstr`,
 *   at most `bufsize` bytes, including trailing '\0' may be written
 *   The return value is the size of the full converted string
 *
 * Errors:
 *   Return WTF8_UNICODE_CONVERT_ERROR when `strict` is non-zero and an error was found
 */
size_t WTF8_utf8_to_local(char *localstr, const char *utf8str, int strict, size_t bufsize);

/**
 * Convert local charest string (the so-called Windows ANSI) to UTF-8
 *
 * On platforms other than Windows, this is identical to `WTF8::utf8_validify`
 *
 * Results:
 *   The converted string is stored in `utf8str`,
 *   at most `bufsize` bytes, including trailing '\0' may be written
 *   The return value is the size of the full converted string
 *
 * Errors:
 *   Return WTF8_UNICODE_CONVERT_ERROR when `strict` is non-zero and an error was found
 */
size_t WTF8_local_to_utf8(char *utf8str, const char *localstr, int strict, size_t bufsize);

/**
 * Convert a UTF-8 filename to a DOS short filename
 *
 * On platforms other than Windows, this is identical to strict `WTF8::utf8_validify`
 *
 * Results:
 *   The converted string is stored in `dos_filename`,
 *   at most `bufsize` bytes, including trailing '\0' may be written
 *   The return value is the size of the full converted string
 *
 * Errors:
 *   Return WTF8_UNICODE_CONVERT_ERROR
 */
size_t WTF8_utf8_to_dos_filename(char *dos_filename, const char *utf8_filename, size_t bufsize);

#ifdef __cplusplus
}
#endif

#endif
