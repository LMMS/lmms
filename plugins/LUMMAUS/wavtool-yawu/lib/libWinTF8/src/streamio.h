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
#ifndef WTF8_STREAMIO_H_INCLUDED_
#define WTF8_STREAMIO_H_INCLUDED_

#ifdef __cplusplus
#include <ios>
#include <fstream>
#include <utility>
#include "u8str.h"

namespace WTF8 {

/**
 * An `std::ifstream` implementation that accepts UTF-8 strings as filename
 *
 * Bugs:
 *   Since `std::ifstream::open(const std::wstring &filename)` is not implementated by MinGW,
 *   alternative measures (refer to `Boost::NoWide`) must be taken when building on such platform
 */
template<typename CharT, typename Traits = std::char_traits<CharT> >
class basic_ifstream : public std::basic_ifstream<CharT, Traits> {
public:
    basic_ifstream() :
        std::basic_ifstream<CharT, Traits>() {
    }
    basic_ifstream(basic_ifstream &&other) :
        std::basic_ifstream<CharT, Traits>(std::move(other)) {
    }
    basic_ifstream(const basic_ifstream &) = delete;
#ifdef _WIN32
    explicit basic_ifstream(const char *filename, std::ios_base::openmode mode = std::ios_base::in) :
        std::basic_ifstream<CharT, Traits>(u8string(filename).to_wide(true), mode) {
    }
    explicit basic_ifstream(const std::string &filename, std::ios_base::openmode mode = std::ios_base::in) :
        std::basic_ifstream<CharT, Traits>(u8string(filename).to_wide(true), mode) {
    }
    explicit basic_ifstream(const u8string &filename, std::ios_base::openmode mode = std::ios_base::in) :
        std::basic_ifstream<CharT, Traits>(filename.to_wide(true), mode) {
    }
    void open(const char *filename, std::ios_base::openmode mode = std::ios_base::in) {
        std::basic_ifstream<CharT, Traits>::open(u8string(filename).to_wide(true), mode);
    }
    void open(const std::string &filename, std::ios_base::openmode mode = std::ios_base::in) {
        std::basic_ifstream<CharT, Traits>::open(u8string(filename).to_wide(true), mode);
    }
    void open(const u8string &filename, std::ios_base::openmode mode = std::ios_base::in) {
        std::basic_ifstream<CharT, Traits>::open(filename.to_wide(true), mode);
    }
#else
    using std::basic_ifstream<CharT, Traits>::basic_ifstream;
    explicit basic_ifstream(const u8string &filename, std::ios_base::openmode mode = std::ios_base::in) :
        std::basic_ifstream<CharT, Traits>(static_cast<std::string>(filename), mode) {
    }
    void open(const u8string &filename, std::ios_base::openmode mode = std::ios_base::in) {
        std::basic_ifstream<CharT, Traits>::open(static_cast<std::string>(filename), mode);
    }
#endif
};

/**
 * An `std::ofstream` implementation that accepts UTF-8 strings as filename
 *
 * Bugs:
 *   Since `std::ofstream::open(const std::wstring &filename)` is not implementated by MinGW,
 *   alternative measures (refer to `Boost::NoWide`) must be taken when building on such platform
 */
template<typename CharT, typename Traits = std::char_traits<CharT> >
class basic_ofstream : public std::basic_ofstream<CharT, Traits> {
public:
    basic_ofstream() :
        std::basic_ofstream<CharT, Traits>() {
    }
    basic_ofstream(basic_ofstream &&other) :
        std::basic_ofstream<CharT, Traits>(std::move(other)) {
    }
    basic_ofstream(const basic_ofstream &) = delete;
#ifdef _WIN32
    explicit basic_ofstream(const char *filename, std::ios_base::openmode mode = std::ios_base::out) :
        std::basic_ofstream<CharT, Traits>(u8string(filename).to_wide(true), mode) {
    }
    explicit basic_ofstream(const std::string &filename, std::ios_base::openmode mode = std::ios_base::out) :
        std::basic_ofstream<CharT, Traits>(u8string(filename).to_wide(true), mode) {
    }
    explicit basic_ofstream(const u8string &filename, std::ios_base::openmode mode = std::ios_base::out) :
        std::basic_ofstream<CharT, Traits>(filename.to_wide(true), mode) {
    }
    void open(const char *filename, std::ios_base::openmode mode = std::ios_base::out) {
        std::basic_ofstream<CharT, Traits>::open(u8string(filename).to_wide(true), mode);
    }
    void open(const std::string &filename, std::ios_base::openmode mode = std::ios_base::in | std::ios_base::out) {
        std::basic_ofstream<CharT, Traits>::open(u8string(filename).to_wide(true), mode);
    }
    void open(const u8string &filename, std::ios_base::openmode mode = std::ios_base::in | std::ios_base::out) {
        std::basic_ofstream<CharT, Traits>::open(filename.to_wide(true), mode);
    }
#else
    using std::basic_ofstream<CharT, Traits>::basic_ofstream;
    explicit basic_ofstream(const u8string &filename, std::ios_base::openmode mode = std::ios_base::out) :
        std::basic_ofstream<CharT, Traits>(static_cast<std::string>(filename), mode) {
    }
    void open(const u8string &filename, std::ios_base::openmode mode = std::ios_base::out) {
        std::basic_ofstream<CharT, Traits>::open(static_cast<std::string>(filename), mode);
    }
#endif
};

/**
 * An `std::fstream` implementation that accepts UTF-8 strings as filename
 *
 * Bugs:
 *   Since `std::fstream::open(const std::wstring &filename)` is not implementated by MinGW,
 *   alternative measures (refer to `Boost::NoWide`) must be taken when building on such platform
 */
template<typename CharT, typename Traits = std::char_traits<CharT> >
class basic_fstream : public std::basic_fstream<CharT, Traits> {
public:
    basic_fstream() :
        std::basic_fstream<CharT, Traits>() {
    }
    basic_fstream(basic_fstream &&other) :
        std::basic_fstream<CharT, Traits>(std::move(other)) {
    }
    basic_fstream(const basic_fstream &) = delete;
#ifdef _WIN32
    explicit basic_fstream(const char *filename, std::ios_base::openmode mode = std::ios_base::in | std::ios_base::out) :
        std::basic_fstream<CharT, Traits>(u8string(filename).to_wide(true), mode) {
    }
    explicit basic_fstream(const std::string &filename, std::ios_base::openmode mode = std::ios_base::in | std::ios_base::out) :
        std::basic_fstream<CharT, Traits>(u8string(filename).to_wide(true), mode) {
    }
    explicit basic_fstream(const u8string &filename, std::ios_base::openmode mode = std::ios_base::in | std::ios_base::out) :
        std::basic_fstream<CharT, Traits>(filename.to_wide(true), mode) {
    }
    void open(const char *filename, std::ios_base::openmode mode = std::ios_base::in | std::ios_base::out) {
        std::basic_fstream<CharT, Traits>::open(u8string(filename).to_wide(true), mode);
    }
    void open(const std::string &filename, std::ios_base::openmode mode = std::ios_base::in | std::ios_base::out) {
        std::basic_fstream<CharT, Traits>::open(u8string(filename).to_wide(true), mode);
    }
    void open(const u8string &filename, std::ios_base::openmode mode = std::ios_base::in | std::ios_base::out) {
        std::basic_fstream<CharT, Traits>::open(filename.to_wide(true), mode);
    }
#else
    using std::basic_fstream<CharT, Traits>::basic_fstream;
    explicit basic_fstream(const u8string &filename, std::ios_base::openmode mode = std::ios_base::in | std::ios_base::out) :
        std::basic_fstream<CharT, Traits>(static_cast<std::string>(filename), mode) {
    }
    void open(const u8string &filename, std::ios_base::openmode mode = std::ios_base::in | std::ios_base::out) {
        std::basic_fstream<CharT, Traits>::open(static_cast<std::string>(filename), mode);
    }
#endif
};

/**
 * An `std::filebuf` implementation that accepts UTF-8 strings as filename
 */
template<typename CharT, typename Traits = std::char_traits<CharT> >
class basic_filebuf : public std::basic_filebuf<CharT, Traits> {
public:
    basic_filebuf() :
        std::basic_filebuf<CharT, Traits>() {
    }
    basic_filebuf(const basic_filebuf &) = delete;
    basic_filebuf(basic_filebuf &&other) :
        std::basic_filebuf<CharT, Traits>(std::move(other)) {
    }
    basic_filebuf &operator=(basic_filebuf &&other) {
        *static_cast<std::basic_filebuf<CharT, Traits>>(this) = std::move(other);
        return *this;
    }
    basic_filebuf &operator=(const basic_filebuf &) = delete;

#ifdef _WIN32
    basic_filebuf *open(const char *filename, std::ios_base::openmode mode) {
        return std::basic_filebuf<CharT, Traits>::open(u8string(filename).to_wide(true), mode) ? this : nullptr;
    }
    basic_filebuf *open(const u8string &filename, std::ios_base::openmode mode) {
        return std::basic_filebuf<CharT, Traits>::open(filename.to_wide(true), mode) ? this : nullptr;
    }
#else
    basic_filebuf *open(const u8string &filename, std::ios_base::openmode mode) {
        return std::basic_filebuf<CharT, Traits>::open(static_cast<std::string>(filename), mode) ? this : nullptr;
    }
#endif
};

typedef basic_ifstream<char>    ifstream;
typedef basic_ofstream<char>    ofstream;
typedef basic_fstream<char>     fstream;
typedef basic_ifstream<wchar_t> wifstream;
typedef basic_ofstream<wchar_t> wofstream;
typedef basic_fstream<wchar_t>  wfstream;
typedef basic_filebuf<char>     filebuf;
typedef basic_filebuf<wchar_t>  wfilebuf;

}
#endif

#endif
