/* Calf DSP Library
 * DSP primitives.
 *
 * Copyright (C) 2001-2007 Krzysztof Foltman
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
#ifndef __CALF_PRIMITIVES_H
#define __CALF_PRIMITIVES_H

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <cmath>
#include <cstdlib>
#include <map>

namespace dsp {

/// Set a float to zero
inline void zero(float &v) {
    v = 0;
};

/// Set a double to zero
inline void zero(double &v) {
    v = 0;
};

/// Set 64-bit unsigned integer value to zero
inline void zero(uint64_t &v) { v = 0; };
/// Set 32-bit unsigned integer value to zero
inline void zero(uint32_t &v) { v = 0; };
/// Set 16-bit unsigned integer value to zero
inline void zero(uint16_t &v) { v = 0; };
/// Set 8-bit unsigned integer value to zero
inline void zero(uint8_t &v) { v = 0; };
/// Set 64-bit signed integer value to zero
inline void zero(int64_t &v) { v = 0; };
/// Set 32-bit signed integer value to zero
inline void zero(int32_t &v) { v = 0; };
/// Set 16-bit signed integer value to zero
inline void zero(int16_t &v) { v = 0; };
/// Set 8-bit signed integer value to zero
inline void zero(int8_t &v) { v = 0; };

/// Set array (buffer or anything similar) to vector of zeroes
template<class T>
void zero(T *data, unsigned int size) {
    T value;
    dsp::zero(value);
    for (unsigned int i=0; i<size; i++)
        *data++ = value;
}

template<class T = float>struct stereo_sample {
    T left;
    T right;
    /// default constructor - preserves T's semantics (ie. no implicit initialization to 0)
    inline stereo_sample() {
    }
    inline stereo_sample(T _left, T _right) {
        left = _left;
        right = _right;
    }
    inline stereo_sample(T _both) {
        left = right = _both;
    }
    template<typename U>
    inline stereo_sample(const stereo_sample<U> &value) {
        left = value.left;
        right = value.right;
    }
    inline stereo_sample& operator=(const T &value) {
        left = right = value;
        return *this;
    }
    template<typename U>
    inline stereo_sample& operator=(const stereo_sample<U> &value) {
        left = value.left;
        right = value.right;
        return *this;
    }
/*
    inline operator T() const {
        return (left+right)/2;
    }
*/
    inline stereo_sample& operator*=(const T &multiplier) {
        left *= multiplier;
        right *= multiplier;
        return *this;
    }
    inline stereo_sample& operator+=(const stereo_sample<T> &value) {
        left += value.left;
        right += value.right;
        return *this;
    }
    inline stereo_sample& operator-=(const stereo_sample<T> &value) {
        left -= value.left;
        right -= value.right;
        return *this;
    }
    template<typename U> inline stereo_sample<U> operator*(const U &value) const {
        return stereo_sample<U>(left*value, right*value);
    }
    /*inline stereo_sample<float> operator*(float value) const {
        return stereo_sample<float>(left*value, right*value);
    }
    inline stereo_sample<double> operator*(double value) const {
        return stereo_sample<double>(left*value, right*value);
    }*/
    inline stereo_sample<T> operator+(const stereo_sample<T> &value) {
        return stereo_sample(left+value.left, right+value.right);
    }
    inline stereo_sample<T> operator-(const stereo_sample<T> &value) {
        return stereo_sample(left-value.left, right-value.right);
    }
    inline stereo_sample<T> operator+(const T &value) {
        return stereo_sample(left+value, right+value);
    }
    inline stereo_sample<T> operator-(const T &value) {
        return stereo_sample(left-value, right-value);
    }
    inline stereo_sample<float> operator+(float value) {
        return stereo_sample<float>(left+value, right+value);
    }
    inline stereo_sample<float> operator-(float value) {
        return stereo_sample<float>(left-value, right-value);
    }
    inline stereo_sample<double> operator+(double value) {
        return stereo_sample<double>(left+value, right+value);
    }
    inline stereo_sample<double> operator-(double value) {
        return stereo_sample<double>(left-value, right-value);
    }
};

/// Multiply constant by stereo_value
template<class T>
inline stereo_sample<T> operator*(const T &value, const stereo_sample<T> &value2) {
    return stereo_sample<T>(value2.left*value, value2.right*value);
}

/// Add constant to stereo_value
template<class T>
inline stereo_sample<T> operator+(const T &value, const stereo_sample<T> &value2) {
    return stereo_sample<T>(value2.left+value, value2.right+value);
}

/// Subtract stereo_value from constant (yields stereo_value of course)
template<class T>
inline stereo_sample<T> operator-(const T &value, const stereo_sample<T> &value2) {
    return stereo_sample<T>(value-value2.left, value-value2.right);
}

/// Shift value right by 'bits' bits (multiply by 2^-bits)
template<typename T>
inline stereo_sample<T> shr(stereo_sample<T> v, int bits = 1) {
    v.left = shr(v.left, bits);
    v.right = shr(v.right, bits);
    return v;
}

/// Set a stereo_sample<T> value to zero
template<typename T> 
inline void zero(stereo_sample<T> &v) {
    dsp::zero(v.left);
    dsp::zero(v.right);
}

/// 'Small value' for integer and other types
template<typename T>
inline T small_value() {
    return 0;
}

/// 'Small value' for floats (2^-24) - used for primitive underrun prevention. The value is pretty much arbitrary (allowing for 24-bit signals normalized to 1.0).
template<>
inline float small_value<float>() {
    return (1.0/16777216.0); // allows for 2^-24, should be enough for 24-bit DACs at least :)
}

/// 'Small value' for doubles (2^-24) - used for primitive underrun prevention. The value is pretty much arbitrary.
template<>
inline double small_value<double>() {
    return (1.0/16777216.0);
}

/// Convert a single value to single value = do nothing :) (but it's a generic with specialisation for stereo_sample)
template<typename T> 
inline float mono(T v) {
    return v;
}

/// Convert a stereo_sample to single value by averaging two channels
template<typename T> 
inline T mono(stereo_sample<T> v) {
    return shr(v.left+v.right);
}

/// Clip a value to [min, max]
template<typename T> 
inline T clip(T value, T min, T max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

/// Clip a double to [-1.0, +1.0]
inline double clip11(double value) {
    double a = fabs(value);
    if (a<=1) return value;
    return (value<0) ? -1.0 : 1.0;
}

/// Clip a float to [-1.0f, +1.0f]
inline float clip11(float value) {
    float a = fabsf(value);
    if (a<=1) return value;
    return (value<0) ? -1.0f : 1.0f;
}

/// Clip a double to [0.0, +1.0]
inline double clip01(double value) {
    double a = fabs(value-0.5);
    if (a<=0.5) return value;
    return (a<0) ? -0.0 : 1.0;
}

/// Clip a float to [0.0f, +1.0f]
inline float clip01(float value) {
    float a = fabsf(value-0.5f);
    if (a<=0.5f) return value;
    return (value < 0) ? -0.0f : 1.0f;
}

// Linear interpolation (mix-way between v1 and v2).
template<typename T, typename U>
inline T lerp(T v1, T v2, U mix) {
    return v1+(v2-v1)*mix;
}

// Linear interpolation for stereo values (mix-way between v1 and v2).
template<typename T>
inline stereo_sample<T> lerp(stereo_sample<T> &v1, stereo_sample<T> &v2, float mix) {
    return stereo_sample<T>(v1.left+(v2.left-v1.left)*mix, v1.right+(v2.right-v1.right)*mix);
}

/**
 * decay-only envelope (linear or exponential); deactivates itself when it goes below a set point (epsilon)
 */
class decay
{
    double value, initial;
    unsigned int age, mask;
    bool active;
public:
    decay() {
        active = false;
        mask = 127;
        initial = value = 0.0;
    }
    inline bool get_active() {
        return active;
    }
    inline double get() {
        return active ? value : 0.0;
    }
    inline void set(double v) {
        initial = value = v;
        active = true;
        age = 0;
    }
    /// reinitialise envelope (must be called if shape changes from linear to exponential or vice versa in the middle of envelope)
    inline void reinit()
    {
        initial = value;
        age = 1;
    }
    inline void add(double v) {
        if (active)
            value += v;
        else
            value = v;
        initial = value;
        age = 0;
        active = true;
    }
    static inline double calc_exp_constant(double times, double cycles)
    {
        if (cycles < 1.0)
            cycles = 1.0;
        return pow(times, 1.0 / cycles);
    }
    inline void age_exp(double constant, double epsilon) {
        if (active) {
            if (!(age & mask))
                value = initial * pow(constant, (double)age);
            else
                value *= constant;
            if (value < epsilon)
                active = false;
            age++;
        }
    }
    inline void age_lin(double constant, double epsilon) {
        if (active) {
            if (!(age & mask))
                value = initial - constant * age;
            else
                value -= constant;
            if (value < epsilon)
                active = false;
            age++;
        }
    }
    inline void deactivate() {
        active = false;
        value = 0;
    }
};

class scheduler;

class task {
public:
    virtual void execute(scheduler *s)=0;
    virtual void dispose() { delete this; }
    virtual ~task() {}
};

/// this scheduler is based on std::multimap, so it isn't very fast, I guess
/// maybe some day it should be rewritten to use heapsort or something
/// work in progress, don't use!
class scheduler {
    std::multimap<unsigned int, task *> timeline;
    unsigned int time, next_task;
    bool eob;
    class end_buf_task: public task {
    public:
        scheduler *p;
        end_buf_task(scheduler *_p) : p(_p) {}
        virtual void execute(scheduler *s) { p->eob = true; }
        virtual void dispose() { }
    } eobt;
public:

    scheduler()
    : time(0)
    , next_task((unsigned)-1)
    , eob(true)
    , eobt (this)
    {
        time = 0;
        next_task = (unsigned)-1;
        eob = false;
    }
    inline bool is_next_tick() {
        if (time < next_task)
            return true;
        do_tasks();
    }
    inline void next_tick() {
        time++;
    }
    void set(int pos, task *t) {
        timeline.insert(std::pair<unsigned int, task *>(time+pos, t));
        next_task = timeline.begin()->first;
    }
    void do_tasks() {
        std::multimap<unsigned int, task *>::iterator i = timeline.begin();
        while(i != timeline.end() && i->first == time) {
            i->second->execute(this);
            i->second->dispose();
            timeline.erase(i);
        }
    }
    bool is_eob() {
        return eob;
    }
    void set_buffer_size(int count) {
        set(count, &eobt);
    }
};

/**
 * Force "small enough" float value to zero
 */
inline void sanitize(float &value)
{
    if (std::abs(value) < small_value<float>())
        value = 0.f;
}

/**
 * Force already-denormal float value to zero
 */
inline void sanitize_denormal(float& value)
{
    if (((*(unsigned int *) &value) & 0x7f800000) == 0) {
        value = 0;
    }
}
	
/**
 * Force "small enough" double value to zero
 */
inline void sanitize(double &value)
{
    if (std::abs(value) < small_value<double>())
        value = 0.f;
}

/**
 * Force "small enough" stereo value to zero
 */
template<class T>
inline void sanitize(stereo_sample<T> &value)
{
    sanitize(value.left);
    sanitize(value.right);
}

inline float fract16(unsigned int value)
{
    return (value & 0xFFFF) * (1.0 / 65536.0);
}

/**
 * typical precalculated sine table
 */
template<class T, int N, int Multiplier>
class sine_table
{
public:
    static bool initialized;
    static T data[N+1];
    sine_table() {
        if (initialized)
            return;
        initialized = true;
        for (int i=0; i<N+1; i++)
            data[i] = (T)(Multiplier*sin(i*2*M_PI*(1.0/N)));
    }
};

template<class T, int N, int Multiplier>
bool sine_table<T,N,Multiplier>::initialized = false;

template<class T, int N, int Multiplier>
T sine_table<T,N,Multiplier>::data[N+1];

/// fast float to int conversion using default rounding mode
inline int fastf2i_drm(float f)
{
#ifdef __X86__
    volatile int v;
    __asm ( "flds %1; fistpl %0" : "=m"(v) : "m"(f));
    return v;
#else
    return (int)nearbyintf(f);
#endif
}

/// Convert MIDI note to frequency in Hz.
inline float note_to_hz(double note, double detune_cents = 0.0)
{
    return 440 * pow(2.0, (note - 69 + detune_cents/100.0) / 12.0);
}

/// Hermite interpolation between two points and slopes in normalized range (written after Wikipedia article)
/// @arg t normalized x coordinate (0-1 over the interval in question)
/// @arg p0 first point
/// @arg p1 second point
/// @arg m0 first slope (multiply by interval width when using over non-1-wide interval)
/// @arg m1 second slope (multiply by interval width when using over non-1-wide interval)
inline float normalized_hermite(float t, float p0, float p1, float m0, float m1)
{
    float t2 = t*t;
    float t3 = t2*t;
    return (2*t3 - 3*t2 + 1) * p0 + (t3 - 2*t2 + t) * m0 + (-2*t3 + 3*t2) * p1 + (t3-t2) * m1;
}

/// Hermite interpolation between two points and slopes 
/// @arg x point within interval (x0 <= x <= x1)
/// @arg x0 interval start
/// @arg x1 interval end
/// @arg p0 value at x0
/// @arg p1 value at x1
/// @arg m0 slope (steepness, tangent) at x0
/// @arg m1 slope at x1
inline float hermite_interpolation(float x, float x0, float x1, float p0, float p1, float m0, float m1)
{
    float width = x1 - x0;
    float t = (x - x0) / width;
    m0 *= width;
    m1 *= width;
    float t2 = t*t;
    float t3 = t2*t;
    
    float ct0 = p0;
    float ct1 = m0;
    float ct2 = -3 * p0 - 2 * m0 + 3 * p1 - m1;
    float ct3 = 2 * p0 + m0  - 2 * p1 + m1;
    
    return ct3 * t3 + ct2 * t2 + ct1 * t + ct0;
    //return (2*t3 - 3*t2 + 1) * p0 + (t3 - 2*t2 + t) * m0 + (-2*t3 + 3*t2) * p1 + (t3-t2) * m1;
}

/// convert amplitude value to dB
inline float amp2dB(float amp)
{
    return 6.0 * log(amp) / log(2);
}

};

#endif
