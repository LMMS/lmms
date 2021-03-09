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

#include <iostream>
#include <istream>
#include <ostream>
#include "utils.h"
#include "u8str.h"
#include "utfconv.h"
#include "termio.h"

#ifdef _WIN32
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <ios>
#include <memory>
#include <string>
#include <streambuf>
#include <vector>
#include <windows.h>
#endif

namespace WTF8 {

#ifdef _WIN32

class ConsoleInputBuffer : public std::streambuf {
public:
    ConsoleInputBuffer(int fd) :
        WTF8_handle(GetStdHandle(fd)) {
        DWORD dummy;
        WTF8_is_console = !!GetConsoleMode(WTF8_handle, &dummy);
        if(WTF8_is_console)
            WTF8_wbuffer.resize(BUFSIZ);
        else
            WTF8_buffer.reserve(BUFSIZ);
        setg(nullptr, nullptr, nullptr);
    }
protected:
    std::streambuf *setbuf(char *buffer, std::streamsize size) {
        unused_arg(buffer); /* only use the size parameter */
        if(size == 0)
            size = BUFSIZ;
        if(WTF8_is_console) {
            WTF8_buffer.clear();
            WTF8_wbuffer.resize(size_t(size));
            WTF8_wbuffer.shrink_to_fit();
        } else {
            WTF8_buffer.resize(0);
            WTF8_buffer.reserve(size_t(size));
        }
        WTF8_buffer_size = size_t(size);
        return this;
    }
    int underflow() {
        if(gptr() == egptr()) {
            if(read() == -1)
                return EOF;
            if(gptr() == egptr())
                return EOF;
        }
        return std::char_traits<char>::to_int_type(*gptr());
    }
private:
    ssize_t read() {
        bool has_last_putback = eback() != gptr();
        char last_putback = has_last_putback ? gptr()[-1] : '\xff';
        if(WTF8_is_console) {
            DWORD wchars_read = 0;
            if(last_surrogate_pair == L'\0') {
                if(!ReadConsoleW(WTF8_handle, WTF8_wbuffer.data(), DWORD(WTF8_buffer_size), &wchars_read, nullptr))
                    return -1;
            } else {
                WTF8_wbuffer[0] = last_surrogate_pair;
                if(!ReadConsoleW(WTF8_handle, WTF8_wbuffer.data()+1, DWORD(WTF8_buffer_size-1), &wchars_read, nullptr))
                    return -1;
                ++wchars_read;
            }
            if(size_t(wchars_read) == WTF8_buffer_size && uint16_t(WTF8_wbuffer[wchars_read-1] & 0xfc00) == 0xd800)
                last_surrogate_pair = WTF8_wbuffer[--wchars_read];
            else
                last_surrogate_pair = L'\0';
            u8string WTF8_sbuffer;
            if(has_last_putback) {
                WTF8_sbuffer = u8string(1, last_putback);
                WTF8_sbuffer += u8string::from_wide(std::wstring(WTF8_wbuffer.data(), wchars_read));
            } else
                WTF8_sbuffer = u8string::from_wide(std::wstring(WTF8_wbuffer.data(), wchars_read));
            WTF8_buffer.assign(WTF8_sbuffer.cbegin(), WTF8_sbuffer.cend());
        } else {
            DWORD bytes_read = 0;
            WTF8_buffer.clear();
            WTF8_buffer.resize(WTF8_buffer_size);
            if(has_last_putback) {
                WTF8_buffer[0] = last_putback;
                if(!ReadFile(WTF8_handle, WTF8_buffer.data()+1, DWORD(WTF8_buffer.size()-1), &bytes_read, nullptr))
                    return -1;
                WTF8_buffer.resize(bytes_read+1);
            } else {
                if(!ReadFile(WTF8_handle, WTF8_buffer.data(), DWORD(WTF8_buffer.size()), &bytes_read, nullptr))
                    return -1;
                WTF8_buffer.resize(bytes_read);
            }
        }
        setg(WTF8_buffer.data(), WTF8_buffer.data(), WTF8_buffer.data()+WTF8_buffer.size());
        if(has_last_putback) {
            gbump(1);
            return WTF8_buffer.size()-1;
        } else
            return WTF8_buffer.size();
    }
    HANDLE WTF8_handle;
    bool WTF8_is_console;
    std::vector<char> WTF8_buffer;
    std::vector<wchar_t> WTF8_wbuffer;
    wchar_t last_surrogate_pair = L'\0';
    bool has_last_putback = false;
    char last_putback;
    size_t WTF8_buffer_size = BUFSIZ;
};

class ConsoleOutputBuffer : public std::streambuf {
public:
    ConsoleOutputBuffer(int fd) :
        WTF8_handle(GetStdHandle(fd)) {
        DWORD dummy;
        WTF8_is_console = !!GetConsoleMode(WTF8_handle, &dummy);
        setp(WTF8_buffer, WTF8_buffer+WTF8_buffer_size);
    }
protected:
    std::streambuf *setbuf(char *buffer, std::streamsize size) {
        sync();
        if(size == 0) {
            WTF8_init_buffer.clear();
            WTF8_init_buffer.resize(BUFSIZ);
            WTF8_init_buffer.shrink_to_fit();
            buffer = WTF8_init_buffer.data();
            size = BUFSIZ;
        } else if(!WTF8_init_buffer.empty()) {
            WTF8_init_buffer.resize(0);
            WTF8_init_buffer.shrink_to_fit();
        }
        WTF8_buffer = buffer;
        WTF8_buffer_size = size_t(size);
        setp(WTF8_buffer, WTF8_buffer+WTF8_buffer_size);
        return this;
    }
    int sync() {
        ptrdiff_t size = pptr()-pbase();
        assert(size >= 0);
        if(size > 0) {
            ssize_t written = write(pbase(), size_t(size));
            if(written <= 0)
                return -1;
            else if(written < size) {
                memmove(pbase(), pbase()+written, size-written);
                setp(pbase(), epptr());
                pbump(int(size-written));
            } else
                setp(pbase(), epptr());
            return 0;
        } else if(size == 0)
            return 0;
        else
            return -1;
    }
    int overflow(int c) {
        if(sync() != 0)
            return EOF;
        if(c != EOF) {
            if(pptr() == epptr())
                return EOF;
            *pptr() = char(c);
            pbump(1);
        }
        return c;
    }
private:
    ssize_t write(const char *buf, size_t size) {
        if(WTF8_is_console) {
            const char *last_char = buf+size;
            while(last_char != buf)
                if(uint8_t(*--last_char & 0xc0) != 0x80)
                    break;
            if(last_char == buf)
                last_char = buf+size;
            std::wstring wbuf = u8string(buf, size_t(last_char-buf)).to_wide();
            if(last_char != buf)
                try {
                    wbuf.append(u8string(last_char, size_t(size-(last_char-buf))).to_wide(true));
                } catch(unicode_conversion_error) {
                    size = size_t(last_char-buf);
                }
            DWORD wchars_written = 0;
            if(!WriteConsoleW(WTF8_handle, wbuf.data(), DWORD(wbuf.size()), &wchars_written, nullptr))
                return -1;
            else if(wchars_written == wbuf.size())
                return size;
            else
                /* UTF conversion error may happen, the accurate number of chars is unknown */
                return (std::min)(u8string::from_wide(wbuf.substr(0, wchars_written)).size(), size);
        } else {
            DWORD bytes_written = 0;
            if(!WriteFile(WTF8_handle, buf, DWORD(size), &bytes_written, nullptr))
                return -1;
            else
                return bytes_written;
        }
    }
    HANDLE WTF8_handle;
    bool WTF8_is_console;
    std::vector<char> WTF8_init_buffer = std::vector<char>(BUFSIZ);
    char *WTF8_buffer = WTF8_init_buffer.data();
    size_t WTF8_buffer_size = BUFSIZ;
};

class ConsoleInput : public std::istream {
public:
    ConsoleInput(std::streambuf *rdbuf, std::ostream *tied_stream) :
        std::istream(rdbuf) {
        if(tied_stream)
            tie(tied_stream);
    }
};

class ConsoleOutput : public std::ostream {
public:
    ConsoleOutput(std::streambuf *rdbuf, std::ostream *tied_stream = nullptr) :
        std::ostream(rdbuf) {
        if(tied_stream)
            tie(tied_stream);
    }
    virtual ~ConsoleOutput() {
        *this << std::flush;
    }
};

static ConsoleInputBuffer cin_buf(STD_INPUT_HANDLE);
static ConsoleOutputBuffer cout_buf(STD_OUTPUT_HANDLE);
static ConsoleOutputBuffer cerr_buf(STD_ERROR_HANDLE);
static ConsoleOutputBuffer clog_buf(STD_ERROR_HANDLE);

static ConsoleOutput cout_inst(&cout_buf);
static ConsoleInput cin_inst(&cin_buf, &cout_inst);
static ConsoleOutput cerr_inst(&cerr_buf, &cout_inst);
static ConsoleOutput clog_inst(&clog_buf, &cout_inst);

std::istream &cin = cin_inst;
std::ostream &cout = cout_inst;
std::ostream &cerr = cerr_inst;
std::ostream &clog = clog_inst;

#else

std::istream &cin = std::cin;
std::ostream &cout = std::cout;
std::ostream &cerr = std::cerr;
std::ostream &clog = std::clog;

#endif

}

extern "C" {

int WTF8_fgetc(std::FILE *stream) {
    return WTF8::fgetc(stream);
}

char *WTF8_fgets(char *s, int size, std::FILE *stream) {
    return WTF8::fgets(s, size, stream);
}

int WTF8_ungetc(int c, std::FILE *stream) {
    return WTF8::ungetc(c, stream);
}

int WTF8_fputc(int c, std::FILE *stream) {
    return WTF8::fputc(c, stream);
}

int WTF8_fputs(const char *s, std::FILE *stream) {
    return WTF8::fputs(s, stream);
}

int WTF8_feof(std::FILE *stream) {
    return WTF8::feof(stream);
}

int WTF8_puts(const char *s) {
    return WTF8::puts(s);
}

}
