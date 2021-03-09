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
#ifndef WTF8_UTILS_H_INCLUDED_
#define WTF8_UTILS_H_INCLUDED_

#include <cstddef>
#include <string>
#include <cstring>

#ifdef _MSC_VER
#define ssize_t ptrdiff_t
#endif

namespace WTF8 {

/**
 * Mark unused arguments to avoid compiler warnings
 *
 * Usage:
 *   int func(int a) {
 *       unused_arg(a);
 *       return 42;
 *   }
 */
template <typename T>
static inline void unused_arg(const T &arg) {
    static_cast<void>(arg);
}

/**
 * Clamp value in range [a, b]
 */
template <typename T>
static inline T clamp(T value, T a, T b) {
    return a < b ?
        value < a ? a : b < value ? b : value :
        value < b ? b : a < value ? a : value;
}

/**
 * Copy the content of a C++ string to a new C string
 *
 * Cleaning:
 *   The memory must be freed with `WTF8::delete_c_str`
 */
template <typename charT>
static inline charT *new_c_str(const std::basic_string<charT> &s) {
    charT *result = new charT[s.size()+1];
    std::memcpy(result, s.c_str(), s.size()+1);
    return result;
}

/**
 * Copy the content of a C string to a new C string
 * The behavior is similar to `std::strdup`
 *
 * Cleaning:
 *   The memory must be freed with `WTF8::delete_c_str`
 */
template <typename charT>
static inline charT *new_c_str(const charT *s) {
    if(s) {
        size_t length = std::strlen(s);
        charT *result = new charT[length+1];
        std::memcpy(result, s, length+1);
        return result;
    } else
        return nullptr;
}

/**
 * Free the memory that was allocated with `WTF8::new_c_str`
 *
 * Result:
 *   nullptr
 */
template <typename charT>
static inline charT *delete_c_str(charT *s) {
    if(s)
        delete[] s;
    return nullptr;
}

}

#endif
