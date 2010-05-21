/* Calf DSP Library
 * Open Sound Control primitives
 *
 * Copyright (C) 2007 Krzysztof Foltman
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this program; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, 
 * Boston, MA 02111-1307, USA.
 */

#ifndef __CALF_OSCTL_H
#define __CALF_OSCTL_H

#include <string>
#include <vector>
#include <string.h>
#include <errno.h>
#include <iostream>
#include <stdint.h>

namespace osctl
{
    
enum osc_type
{
    osc_i32 = 'i',
    osc_f32 = 'f',
    osc_string = 's',
    osc_blob = 'b',
    
    // unsupported
    osc_i64 = 'h',
    osc_ts = 't',
    osc_f64 = 'd',
    osc_string_alt = 'S',
    osc_char = 'c',
    osc_rgba = 'r',
    osc_midi = 'm',
    osc_true = 'T',
    osc_false = 'F',
    osc_nil = 'N',
    osc_inf = 'I',
    osc_start_array = '[',
    osc_end_array = ']'
};

extern const char *osc_type_name(osc_type type);

struct osc_exception: public std::exception
{
    virtual const char *what() const throw() { return "OSC parsing error"; }
};

struct osc_read_exception: public std::exception
{
    virtual const char *what() const throw() { return "OSC buffer underflow"; }
};

struct osc_write_exception: public std::exception
{
    virtual const char *what() const throw() { return "OSC buffer overflow"; }
};

struct null_buffer
{
    static bool read(uint8_t *dest, uint32_t bytes)
    {
        return false;
    }
    static bool write(uint8_t *dest, uint32_t bytes)
    {
        return true;
    }
    static void clear()
    {
    }
};

struct raw_buffer
{
    uint8_t *ptr;
    uint32_t pos, count, size;
    
    raw_buffer()
    {
        ptr = NULL;
        pos = count = size = 0;
    }
    raw_buffer(uint8_t *_ptr, uint32_t _count, uint32_t _size)
    {
        set(_ptr, _count, _size);
    }
    inline void set(uint8_t *_ptr, uint32_t _count, uint32_t _size)
    {
        ptr = _ptr;
        pos = 0;
        count = _count;
        size = _size;
    }
    bool read(uint8_t *dest, uint32_t bytes)
    {
        if (pos + bytes > count)
            return false;
        memcpy(dest, ptr + pos, bytes);
        pos += bytes;
        return true;
    }
    bool write(const uint8_t *src, uint32_t bytes)
    {
        if (count + bytes > size)
            return false;
        memcpy(ptr + count, src, bytes);
        count += bytes;
        return true;
    }
    int read_left()
    {
        return count - pos;
    }
    int write_left()
    {
        return size - count;
    }
    inline int write_misalignment()
    {
        return 4 - (count & 3);
    }
    void clear()
    {
        pos = 0;
        count = 0;
    }
    int tell()
    {
        return pos;
    }
    void seek(int _pos)
    {
        pos = _pos;
    }
};

struct string_buffer
{
    std::string data;
    uint32_t pos, size;
    
    string_buffer()
    {
        pos = 0;
        size = 1048576;
    }
    string_buffer(std::string _data, int _size = 1048576)
    {
        data = _data;
        pos = 0;
        size = _size;
    }
    bool read(uint8_t *dest, uint32_t bytes)
    {
        if (pos + bytes > data.length())
            return false;
        memcpy(dest, &data[pos], bytes);
        pos += bytes;
        return true;
    }
    bool write(const uint8_t *src, uint32_t bytes)
    {
        if (data.length() + bytes > size)
            return false;
        uint32_t wpos = data.length();
        data.resize(wpos + bytes);
        memcpy(&data[wpos], src, bytes);
        return true;
    }
    inline int read_left()
    {
        return data.length() - pos;
    }
    inline int write_left()
    {
        return size - data.length();
    }
    inline int write_misalignment()
    {
        return 4 - (data.length() & 3);
    }
    void clear()
    {
        data.clear();
        pos = 0;
    }
    int tell()
    {
        return pos;
    }
    void seek(int _pos)
    {
        pos = _pos;
    }
};

template<class Buffer, class TypeBuffer = null_buffer, bool Throw = true>
struct osc_stream
{
    Buffer &buffer;
    TypeBuffer *type_buffer;
    bool error;
    
    osc_stream(Buffer &_buffer) : buffer(_buffer), type_buffer(NULL), error(false) {}
    osc_stream(Buffer &_buffer, TypeBuffer &_type_buffer) : buffer(_buffer), type_buffer(&_type_buffer), error(false) {}
    inline void pad()
    {
        uint32_t zero = 0;
        write(&zero, buffer.write_misalignment());
    }
    inline void read(void *dest, uint32_t bytes)
    {
        if (!buffer.read((uint8_t *)dest, bytes))
        {
#if 0
            if (Throw)
                throw osc_read_exception();
            else
#endif
            {
                error = true;
                memset(dest, 0, bytes);
            }
        }
    }
    inline void write(const void *src, uint32_t bytes)
    {
        if (!buffer.write((const uint8_t *)src, bytes))
        {
#if 0
            if (Throw)
                throw osc_write_exception();
            else
#endif
                error = true;
        }
    }
    inline void clear()
    {
        buffer.clear();
        if (type_buffer)
            type_buffer->clear();
    }
    inline void write_type(char ch)
    {
        if (type_buffer)
            type_buffer->write((uint8_t *)&ch, 1);
    }
};

typedef osc_stream<string_buffer> osc_strstream;
typedef osc_stream<string_buffer, string_buffer> osc_typed_strstream;

struct osc_inline_strstream: public string_buffer, public osc_strstream
{
    osc_inline_strstream()
    : string_buffer(), osc_strstream(static_cast<string_buffer &>(*this))
    {
    }
};

struct osc_str_typed_buffer_pair
{
    string_buffer buf_data, buf_types;
};

struct osc_inline_typed_strstream: public osc_str_typed_buffer_pair, public osc_typed_strstream
{
    osc_inline_typed_strstream()
    : osc_str_typed_buffer_pair(), osc_typed_strstream(buf_data, buf_types)
    {
    }
};

template<class Buffer, class TypeBuffer>
inline osc_stream<Buffer, TypeBuffer> &
operator <<(osc_stream<Buffer, TypeBuffer> &s, uint32_t val)
{
#if 0
    val = htonl(val);
    s.write(&val, 4);
    s.write_type(osc_i32);
#endif
    return s;
}

template<class Buffer, class TypeBuffer>
inline osc_stream<Buffer, TypeBuffer> &
operator >>(osc_stream<Buffer, TypeBuffer> &s, uint32_t &val)
{
#if 0
    s.read(&val, 4);
    val = htonl(val);
#endif
    return s;
}

template<class Buffer, class TypeBuffer>
inline osc_stream<Buffer, TypeBuffer> &
operator >>(osc_stream<Buffer, TypeBuffer> &s, int32_t &val)
{
#if 0
    s.read(&val, 4);
    val = htonl(val);
#endif
    return s;
}

template<class Buffer, class TypeBuffer>
inline osc_stream<Buffer, TypeBuffer> &
operator <<(osc_stream<Buffer, TypeBuffer> &s, float val)
{
    union { float v; uint32_t i; } val2;
    val2.v = val;
    val2.i = htonl(val2.i);
    s.write(&val2.i, 4);
    s.write_type(osc_f32);
    return s;
}

template<class Buffer, class TypeBuffer>
inline osc_stream<Buffer, TypeBuffer> &
operator >>(osc_stream<Buffer, TypeBuffer> &s, float &val)
{
    union { float v; uint32_t i; } val2;
    s.read(&val2.i, 4);
    val2.i = htonl(val2.i);
    val = val2.v;
    return s;
}

template<class Buffer, class TypeBuffer>
inline osc_stream<Buffer, TypeBuffer> &
operator <<(osc_stream<Buffer, TypeBuffer> &s, const std::string &str)
{
    s.write(&str[0], str.length());
    s.pad();
    s.write_type(osc_string);
    return s;
}

template<class Buffer, class TypeBuffer>
inline osc_stream<Buffer, TypeBuffer> &
operator >>(osc_stream<Buffer, TypeBuffer> &s, std::string &str)
{
    // inefficient...
    char five[5];
    five[4] = '\0';
    str.resize(0);
    while(1)
    {
        s.read(five, 4);
        if (five[0] == '\0')
            break;
        str += five;
        if (!five[1] || !five[2] || !five[3])
            break;
    }
    return s;
}

template<class Buffer, class TypeBuffer, class DestBuffer>
inline osc_stream<Buffer, TypeBuffer> &
read_buffer_from_osc_stream(osc_stream<Buffer, TypeBuffer> &s, DestBuffer &buf)
{
#if 0
    uint32_t nlen = 0;
    s.read(&nlen, 4);
    uint32_t len = htonl(nlen);
    // write length in network order
    for (uint32_t i = 0; i < len; i += 1024)
    {
        uint8_t tmp[1024];
        uint32_t part = std::min((uint32_t)1024, len - i);
        s.read(tmp, part);
        buf.write(tmp, part);
    }
    // pad
    s.read(&nlen, 4 - (len & 3));
#endif
    return s;
}

template<class Buffer, class TypeBuffer, class SrcBuffer>
inline osc_stream<Buffer, TypeBuffer> &
write_buffer_to_osc_stream(osc_stream<Buffer, TypeBuffer> &s, SrcBuffer &buf)
{
#if 0
    uint32_t len = buf.read_left();
    uint32_t nlen = ntohl(len);
    s.write(&nlen, 4);
    // write length in network order
    for (uint32_t i = 0; i < len; i += 1024)
    {
        uint8_t tmp[1024];
        uint32_t part = std::min((uint32_t)1024, len - i);
        buf.read(tmp, part);
        s.write(tmp, part);
    }
    s.pad();
    s.write_type(osc_blob);
#endif
    return s;
}

template<class Buffer, class TypeBuffer>
inline osc_stream<Buffer, TypeBuffer> &
operator >>(osc_stream<Buffer, TypeBuffer> &s, raw_buffer &str)
{
    return read_buffer_from_osc_stream(s, str);
}

template<class Buffer, class TypeBuffer>
inline osc_stream<Buffer, TypeBuffer> &
operator >>(osc_stream<Buffer, TypeBuffer> &s, string_buffer &str)
{
    return read_buffer_from_osc_stream(s, str);
}

template<class Buffer, class TypeBuffer>
inline osc_stream<Buffer, TypeBuffer> &
operator <<(osc_stream<Buffer, TypeBuffer> &s, raw_buffer &str)
{
    return write_buffer_to_osc_stream(s, str);
}

template<class Buffer, class TypeBuffer>
inline osc_stream<Buffer, TypeBuffer> &
operator <<(osc_stream<Buffer, TypeBuffer> &s, string_buffer &str)
{
    return write_buffer_to_osc_stream(s, str);
}

// XXXKF: I don't support reading binary blobs yet

struct osc_net_bad_address: public std::exception
{
    std::string addr, error_msg;
    osc_net_bad_address(const char *_addr)
    {
        addr = _addr;
        error_msg = "Incorrect OSC URI: " + addr;
    }
    virtual const char *what() const throw() { return error_msg.c_str(); }
    virtual ~osc_net_bad_address() throw () {}
};

struct osc_net_exception: public std::exception
{
    int net_errno;
    std::string command, error_msg;
    osc_net_exception(const char *cmd, int _errno = errno)
    {
        command = cmd;
        net_errno = _errno;
        error_msg = "OSC error in "+command+": "+strerror(_errno);
    }
    virtual const char *what() const throw() { return error_msg.c_str(); }
    virtual ~osc_net_exception() throw () {}
};
    
struct osc_net_dns_exception: public std::exception
{
#if 0
    int net_errno;
    std::string command, error_msg;
    osc_net_dns_exception(const char *cmd, int _errno = h_errno)
    {
        command = cmd;
        net_errno = _errno;
        error_msg = "OSC error in "+command+": "+hstrerror(_errno);
    }
    virtual const char *what() const throw() { return error_msg.c_str(); }
    virtual ~osc_net_dns_exception() throw () {}
#endif
};
    
template<class OscStream>
struct osc_message_sink
{
    virtual void receive_osc_message(std::string address, std::string type_tag, OscStream &buffer)=0;
    virtual ~osc_message_sink() {}
};

template<class OscStream, class DumpStream>
struct osc_message_dump: public osc_message_sink<OscStream>
{
    DumpStream &stream;
    osc_message_dump(DumpStream &_stream) : stream(_stream) {}
        
    virtual void receive_osc_message(std::string address, std::string type_tag, OscStream &buffer)
    {
        int pos = buffer.buffer.tell();
        stream << "address: " << address << ", type tag: " << type_tag << std::endl;
        for (unsigned int i = 0; i < type_tag.size(); i++)
        {
            stream << "Argument " << i << " is ";
            switch(type_tag[i])
            {
                case 'i': 
                {
                    uint32_t val;
                    buffer >> val;
                    stream << val;
                    break;
                }
                case 'f': 
                {
                    float val;
                    buffer >> val;
                    stream << val;
                    break;
                }
                case 's': 
                {
                    std::string val;
                    buffer >> val;
                    stream << val;
                    break;
                }
                case 'b': 
                {
                    osctl::string_buffer val;
                    buffer >> val;
                    stream << "blob (" << val.data.length() << " bytes)";
                    break;
                }
                default:
                {
                    stream << "unknown - cannot parse more arguments" << std::endl;
                    i = type_tag.size();
                    break;
                }
            }
            stream << std::endl;
        }
        stream << std::flush;
        buffer.buffer.seek(pos);
    }
};

};

#endif
