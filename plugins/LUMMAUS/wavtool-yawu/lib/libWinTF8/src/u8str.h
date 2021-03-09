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
#ifndef WTF8_U8STR_H_INCLUDED_
#define WTF8_U8STR_H_INCLUDED_

#ifdef __cplusplus
#include <cstddef>
#include <functional>
#include <initializer_list>
#include <string>
#include <utility>

namespace WTF8 {

/**
 * A string compatible to `std::string`, that stores UTF-8 string
 * By marking a variable with the type `WTF8::u8string`,
 * it could be made clear that the string that this variable stores is UTF-8
 */
class u8string : public std::string {
public:
    explicit u8string() : std::string() {
    }
    explicit u8string(const std::string &s) :
        std::string(s) {
    }
    explicit u8string(std::string &&s) :
        std::string(std::move(s)) {
    }
    u8string(const u8string &s) :
        std::string(s) {
    }
    u8string(u8string &&s) :
        std::string(std::move(s)) {
    }
    u8string &operator=(const u8string &s) {
        *static_cast<std::string *>(this) = s;
        return *this;
    }
    u8string &operator=(u8string &&s) {
        *static_cast<std::string *>(this) = std::move(s);
        return *this;
    }

    /* Conversion methods */
    explicit u8string(const std::wstring &s, bool strict = false);
    explicit u8string(const wchar_t *s, bool strict = false);
    static u8string from_wide(const std::wstring &s, bool strict = false) {
        return u8string(s, strict);
    }
    static u8string from_wide(const wchar_t *s, bool strict = false) {
        return u8string(s, strict);
    }
    std::wstring to_wide(bool strict = false) const;
    explicit operator std::wstring() const {
        return to_wide();
    }
    u8string validify(bool strict = false) const;
    size_t count_codepoints(bool strict = false) const;

    /* Inheriting all the constructors, since MSVC does not support C++ 11 inherited constructors */
    u8string(size_t count, char ch) :
        std::string(count, ch) {
    }
    u8string(const u8string &other, size_t pos, size_t count = npos) :
        std::string(other, pos, count) {
    }
    u8string(const char *s, size_t count) :
        std::string(s, count) {
    }
    u8string(const char *s) :
        std::string(s) {
    }
    template<class InputIterator>
    u8string(InputIterator first, InputIterator last) :
        std::string(first, last) {
    }
    u8string(std::initializer_list<char> init) :
        std::string(init) {
    }
};

}

namespace std {

template<>
struct hash<WTF8::u8string> {
    size_t operator()(const WTF8::u8string &s) const {
        return hash<string>()(s);
    }
};

}

#endif

#endif
