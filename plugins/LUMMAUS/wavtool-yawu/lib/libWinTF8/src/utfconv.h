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
#ifndef WTF8_UTFCONV_H_INCLUDED_
#define WTF8_UTFCONV_H_INCLUDED_

#ifdef __cplusplus
#include <cstddef>
#include <stdexcept>
#include <string>
#else
#include <stddef.h>
#endif

#ifdef __cplusplus
namespace WTF8 {

/**
 * This file contains Unicode conversion functions
 *
 * Microsoft VC++ Runtime behaves differently on different versions,
 * we will just reimplement them ourselves.
 */

/**
 * The exception that throws when `strict` is set and an error was found during Unicode conversion
 */
class unicode_conversion_error : public std::runtime_error {
public:
    unicode_conversion_error(const char *what) : std::runtime_error(what) {}
    unicode_conversion_error() : std::runtime_error("Can not convert string to Unicode") {}
};

/**
 * Covnert a UTF-8 string to `std::wstring`
 *
 * Throws:
 *   WTF8::unicode_conversion_error (when `strict` is set)
 */
std::wstring utf8_to_wide(const std::string &utf8str, bool strict = false);

/**
 * Covnert an `std::wstring` to a UTF-8 string
 *
 * Throws:
 *   WTF8::unicode_conversion_error (when `strict` is set)
 */
std::string wide_to_utf8(const std::wstring &widestr, bool strict = false);

/**
 * Verify if a string is valid UTF-8
 *
 * Return:
 *   A valid UTF-8 string that has replaced corrupted bytes with U+FFFD
 *
 * Throws:
 *   WTF8::unicode_conversion_error (when `strict` is set)
 */
std::string utf8_validify(const std::string &utf8str, bool strict = false);

/**
 * Count how many codepoints a UTF-8 string contains
 *
 * Throws:
 *   WTF8::unicode_conversion_error (when `strict` is set)
 */
size_t utf8_count_codepoints(const std::string &utf8str, bool strict = false);

};
#endif

#ifdef __cplusplus
extern "C" {
#endif

static const size_t WTF8_UNICODE_CONVERT_ERROR = ~(size_t) 0 /* -1 */;

/**
 * Covnert a UTF-8 string to a wide string
 *
 * Result:
 *   The converted string is stored in `widestr`,
 *   at most `bufsize` wide chars, including trailing L'\0' may be written
 *   The return value is the size of the full converted string, in wide chars
 *
 * Errors:
 *   Return WTF8_UNICODE_CONVERT_ERROR when `strict` is non-zero and an error was found
 */
size_t WTF8_utf8_to_wide(wchar_t *widestr, const char *utf8str, int strict, size_t bufsize);

/**
 * Covnert a wide string to UTF-8
 *
 * Result:
 *   The converted string is stored in `utf8str`,
 *   at most `bufsize` bytes, including trailing '\0' may be written
 *   The return value is the size of the full converted string
 *
 * Errors:
 *   Return WTF8_UNICODE_CONVERT_ERROR when `strict` is non-zero and an error was found
 */
size_t WTF8_wide_to_utf8(char *utf8str, const wchar_t *widestr, int strict, size_t bufsize);

/**
 * Verify if a string is valid UTF-8
 *
 * Return:
 *   A valid UTF-8 string that has replaced corrupted bytes with U+FFFD is stored in `validstr`,
 *   at most `bufsize` bytes, including trailing '\0' may be written
 *   The return value is the size of the full converted string
 *
 * Errors:
 *   Return WTF8_UNICODE_CONVERT_ERROR when `strict` is non-zero and an error was found
 */
size_t WTF8_utf8_validify(char *validstr, const char *utf8str, int strict, size_t bufsize);

/**
 * Count how many codepoints a UTF-8 string contains
 *
 * Errors:
 *   Return WTF8_UNICODE_CONVERT_ERROR when `strict` is non-zero and an error was found
 */
size_t WTF8_utf8_count_codepoints(const char *utf8str, int strict);

#ifdef __cplusplus
}
#endif

#endif
