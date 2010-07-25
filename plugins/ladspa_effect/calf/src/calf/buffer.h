/* Calf DSP Library
 * Buffer abstractions.
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
 * Boston, MA  02110-1301  USA
 */
#ifndef __BUFFER_H
#define __BUFFER_H

namespace dsp {

/// decrease by N if >= N (useful for circular buffers)
template<int N> inline int wrap_around(int a) {
    return (a >= N) ? a - N : a;
}

// provide fast specializations for powers of 2
template<> inline int wrap_around<2>(int a) { return a & 1; }
template<> inline int wrap_around<4>(int a) { return a & 3; }
template<> inline int wrap_around<8>(int a) { return a & 7; }
template<> inline int wrap_around<16>(int a) { return a & 15; }
template<> inline int wrap_around<32>(int a) { return a & 31; }
template<> inline int wrap_around<64>(int a) { return a & 63; }
template<> inline int wrap_around<128>(int a) { return a & 127; }
template<> inline int wrap_around<256>(int a) { return a & 255; }
template<> inline int wrap_around<512>(int a) { return a & 511; }
template<> inline int wrap_around<1024>(int a) { return a & 1023; }
template<> inline int wrap_around<2048>(int a) { return a & 2047; }
template<> inline int wrap_around<4096>(int a) { return a & 4095; }
template<> inline int wrap_around<8192>(int a) { return a & 8191; }
template<> inline int wrap_around<16384>(int a) { return a & 16383; }
template<> inline int wrap_around<32768>(int a) { return a & 32767; }
template<> inline int wrap_around<65536>(int a) { return a & 65535; }

template<class Buf, class T>
void fill(Buf &buf, T value) {
    T* data = buf.data();
    int size = buf.size();
    for (int i=0; i<size; i++)
        *data++ = value;
}

template<class T>
void fill(T *data, int size, T value) {
    for (int i=0; i<size; i++)
        *data++ = value;
}

template<class T, class U>
void copy(T *dest, U *src, int size, T scale = 1, T add = 0) {
    for (int i=0; i<size; i++) 
        *dest++ = (*src++) * scale + add;
}

template<class T>
struct sample_traits {
    enum {
        channels = 1,
        bps = sizeof(T)*8
    };
};

template<class T>
struct sample_traits<stereo_sample<T> > {
    enum {
        channels = 2,
        bps = sizeof(T)*8
    };
};

template<int N, class T = float>
class fixed_size_buffer {
public:
    typedef T data_type;
    enum { buffer_size = N };
    inline int size() { return N; }
};

template<int N, class T = float>
class mem_fixed_size_buffer: public fixed_size_buffer<N, T> {
    T *buf;
public:
    mem_fixed_size_buffer(T ubuf[N]) { buf = ubuf; }
    void set_data(T buf[N]) { this->buf = buf; }
    inline T* data() { return buf; }
    inline const T* data() const { return buf; }
    inline T& operator[](int pos) { return buf[pos]; }
    inline const T& operator[](int pos) const { return buf[pos]; }
};

template<int N, class T = float>
class auto_buffer: public fixed_size_buffer<N, T> {
    T buf[N];
public:    
    T* data() const { return buf; }
    inline T& operator[](int pos) { return buf[pos]; }
    inline const T& operator[](int pos) const { return buf[pos]; }
};

template<class T = float>
class dynamic_buffer {
    T *buf;
    int buf_size;
    bool owns;
public:
    dynamic_buffer() { owns = false; }
    dynamic_buffer(T *_buf, int _buf_size, bool _own)
    : buf(_buf), buf_size(_buf_size), owns(_own) {
    }
    dynamic_buffer(int _size) {
        buf = new T[_size];
        buf_size = _size;
        owns = true;
    }
    inline T* data() { return buf; }
    inline const T* data() const { return buf; }
    inline int size() { return buf_size; }
    void resize(int new_size, bool fill_with_zeros = false) {
        T *new_buf = new T[new_size];
        memcpy(new_buf, buf, std::min(buf_size, new_size));
        if (fill_with_zeros && buf_size < new_size)
            dsp::zero(new_buf + buf_size, new_size - buf_size);
        if (owns)
            delete []buf;
        buf = new_buf;
        buf_size = new_size;
        owns = true;
    }
    inline T& operator[](int pos) { return buf[pos]; }
    inline const T& operator[](int pos) const { return buf[pos]; }
    ~dynamic_buffer() {
        if (owns)
            delete []buf;
    }    
}; 

template<class T, class U>
void copy_buf(T &dest_buf, const U &src_buf, T scale = 1, T add = 0) {
    typedef typename T::data_type data_type;
    data_type *dest = dest_buf.data();
    const data_type *src = src_buf.data();
    int size = src.size();
    for (int i=0; i<size; i++) 
        *dest++ = (*src++) * scale + add;
}

template<class T>
struct buffer_traits {
};

/// this class template defines some basic position operations for fixed_size_buffers
template<int N, class T>
struct buffer_traits<fixed_size_buffer<N, T> > {
    int inc_wrap(int pos) const { 
        return wrap_around<T::size>(pos+1);
    }
    
    int pos_diff(int pos1, int pos2) const { 
        int pos = pos1 - pos2;
        if (pos < 0) pos += T::size;
        return pos;
    }
};

/// this is useless for now (and untested too)
template<class B>
class circular_buffer: public B {
    typedef typename B::data_type data_type;
    typedef class buffer_traits<B> traits;
    B buffer;
    int rpos, wpos;
    circular_buffer() {
        clear();
    }
    void clear() {
        rpos = 0;
        wpos = 0;
    }
    inline void put(data_type data) {
        buffer[wpos] = data;
        wpos = traits::inc_wrap(wpos);
    }
    inline bool empty() {
        return rpos == wpos;
    }
    inline bool full() {
        return rpos == traits::inc_wrap(wpos);
    }
    inline const data_type& get() {
        int oldrpos = rpos;
        rpos = traits::inc_wrap(rpos);
        return buffer[oldrpos];
    }
    inline int get_rbytes() {
        return traits::pos_diff(wpos, rpos);
    }
    inline int get_wbytes() {
        if (full()) return 0;
        return traits::pos_diff(rpos, wpos);
    }
};

/// this is useless for now
template<int N, class T = float>
class mono_auto_buffer: public auto_buffer<N, T> {
};

/// this is useless for now
template<int N, class T = float>
class stereo_auto_buffer: public auto_buffer<N, stereo_sample<T> > {
};

};

#endif
