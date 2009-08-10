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
#ifndef __CALF_FIXED_POINT_H
#define __CALF_FIXED_POINT_H

namespace dsp {

inline uint32_t shr(uint32_t v, int bits = 1) { return v>>bits; };
inline int32_t shr(int32_t v, int bits = 1) { return v>>bits; };
inline uint64_t shr(uint64_t v, int bits = 1) { return v>>bits; };
inline int64_t shr(int64_t v, int bits = 1) { return v>>bits; };
inline float shr(float v, int bits = 1) { return v*(1.0/(1<<bits)); };
inline double shr(double v, int bits = 1) { return v*(1.0/(1<<bits)); };
template<class T, int FracBits>
inline T shr(T v, int bits = 1) {
    v.set(v >> bits);
    return v;
}

template<class T, int FracBits> class fixed_point {
    T value;
    enum { IntBits = (sizeof(T)/8) - FracBits };

public:
    /// default constructor, does not initialize the value, just like - say - float doesn't
    inline fixed_point() {
    }

    /// copy constructor from any other fixed_point value
    template<class U, int FracBits2> inline fixed_point(const fixed_point<U, FracBits2> &v) {
        if (FracBits == FracBits2) value = v.get();
        else if (FracBits > FracBits2) value = v.get() << abs(FracBits - FracBits2);
        else value = v.get() >> abs(FracBits - FracBits2);
    }

    /* this would be way too confusing, it wouldn't be obvious if it expects a whole fixed point or an integer part
    explicit inline fixed_point(T v) {
        this->value = v;
    }
    */
    explicit inline fixed_point(double v) {
        value = (T)(v*one());
    }
    
    /// Makes an instance from a representation value (ie. same type of value as is used for internal storage and get/set)
    static inline fixed_point from_base(const T &v) 
    {
        fixed_point result;
        result.value = v;
        return result;
    }
    
    inline static T one() {
        return (T)(1) << FracBits;
    }

    inline void set(T value) {
        this->value = value;
    }
      
    inline T get() const {
        return value;
    }
      
    inline operator double() const {
        return value * (1.0/one());
    }
    
    inline fixed_point &operator=(double v) {
        value = (T)(v*one());
        return *this;
    }
    
    template<class U, int FracBits2> static inline T rebase(const fixed_point<U, FracBits2> &v) {
        if (FracBits == FracBits2) 
            return v.get();
        if (FracBits > FracBits2) 
            return v.get() << abs(FracBits - FracBits2);
        return v.get() >> abs(FracBits2 - FracBits);
    }
    
    template<class U, int FracBits2> inline fixed_point &operator=(const fixed_point<U, FracBits2> &v) {
        value = rebase<U, FracBits2>(v);
        return *this;
    }

    template<class U, int FracBits2> inline fixed_point &operator+=(const fixed_point<U, FracBits2> &v) {
        value += rebase<U, FracBits2>(v);
        return *this;
    }

    template<class U, int FracBits2> inline fixed_point &operator-=(const fixed_point<U, FracBits2> &v) {
        value -= rebase<U, FracBits2>(v);
        return *this;
    }

    template<class U, int FracBits2> inline fixed_point operator+(const fixed_point<U, FracBits2> &v) const {
        fixed_point fpv;
        fpv.set(value + rebase<U, FracBits2>(v));
        return fpv;
    }

    template<class U, int FracBits2> inline fixed_point operator-(const fixed_point<U, FracBits2> &v) const {
        fixed_point fpv;
        fpv.set(value - rebase<U, FracBits2>(v));
        return fpv;
    }

    /// multiply two fixed point values, using long long int to store the temporary multiplication result
    template<class U, int FracBits2> inline fixed_point operator*(const fixed_point<U, FracBits2> &v) const {
        fixed_point tmp;
        tmp.set(((int64_t)value) * v.get() >> FracBits2);
        return tmp;
    }

    /// multiply two fixed point values, using BigType (usually 64-bit int) to store the temporary multiplication result
    template<class U, int FracBits2, class BigType> inline fixed_point& operator*=(const fixed_point<U, FracBits2> &v) {
        value = (T)(((BigType)value) * v.get() >> FracBits2);
        return *this;
    }

    inline fixed_point operator+(int v) const {
        fixed_point tmp;
        tmp.set(value + (v << FracBits));
        return tmp;
    }

    inline fixed_point operator-(int v) const {
        fixed_point tmp;
        tmp.set(value - (v << FracBits));
        return tmp;
    }

    inline fixed_point operator*(int v) const {
        fixed_point tmp;
        tmp.value = value*v;
        return tmp;
    }

    inline fixed_point& operator+=(int v) {
        value += (v << FracBits);
        return *this;
    }

    inline fixed_point& operator-=(int v) {
        value -= (v << FracBits);
        return *this;
    }

    inline fixed_point& operator*=(int v) {
        value *= v;
        return *this;
    }

    /// return integer part
    inline T ipart() const {
        return value >> FracBits;
    }

    /// return integer part as unsigned int
    inline unsigned int uipart() const {
        return ((unsigned)value) >> FracBits;
    }

    /// return integer part as unsigned int
    inline unsigned int ui64part() const {
        return ((uint64_t)value) >> FracBits;
    }

    /// return fractional part as 0..(2^FracBits-1)
    inline T fpart() const {
        return value & ((1 << FracBits)-1);
    }

    /// return fractional part as 0..(2^Bits-1)
    template<int Bits>
    inline T fpart() const {
        int fbits = value & ((1 << FracBits)-1);
        if (Bits == FracBits) return fbits;
        int shift = abs(Bits-FracBits);
        return (Bits < FracBits) ? (fbits >> shift) : (fbits << shift);
    }

    /// return fractional part as 0..1
    inline double fpart_as_double() const {
        return (value & ((1 << FracBits)-1)) * (1.0 / (1 << FracBits));
    }

    /// use fractional part (either whole or given number of most significant bits) for interpolating between two values
    /// note that it uses integer arithmetic only, and isn't suitable for floating point or fixed point U!
    /// @param UseBits can be used when there's a risk of exceeding range of U because max(fpart)*max(v1 or v2) > range of U
    template<class U, int UseBits, class MulType> 
    inline U lerp_by_fract_int(U v1, U v2) const {
        int fp = fpart<UseBits>();
        assert ( fp >=0 && fp <= (1<<UseBits));
        // printf("diff = 
        return v1 + shr(((MulType)(v2-v1) * fp), UseBits);
    }

    template<class U, int UseBits> 
    inline U lerp_table_lookup_int(U data[(1<<IntBits)+1]) const {
        unsigned int pos = uipart();
        return lerp_by_fract_int<U, UseBits>(data[pos], data[pos+1]);
    }

    /// Untested... I've started it to get a sin/cos readout for rotaryorgan, but decided to use table-less solution instead
    /// Do not assume it works, because it most probably doesn't
    template<class U, int UseBits> 
    inline U lerp_table_lookup_int_shift(U data[(1<<IntBits)+1], unsigned int shift) {
        unsigned int pos = (uipart() + shift) & ((1 << IntBits) - 1);
        return lerp_by_fract_int<U, UseBits>(data[pos], data[pos+1]);
    }

    template<class U> 
    inline U lerp_table_lookup_float(U data[(1<<IntBits)+1]) const {
        unsigned int pos = uipart();
        return data[pos] + (data[pos+1]-data[pos]) * fpart_as_double();
    }

    template<class U> 
    inline U lerp_table_lookup_float_mask(U data[(1<<IntBits)+1], unsigned int mask) const {
        unsigned int pos = ui64part() & mask;
        // printf("full = %lld pos = %d + %f\n", value, pos, fpart_as_double());
        return data[pos] + (data[pos+1]-data[pos]) * fpart_as_double();
    }

    template<class U, int UseBits, class MulType> 
    inline U lerp_ptr_lookup_int(U *data) const {
        unsigned int pos = ui64part();
        return lerp_by_fract_int<U, UseBits, MulType>(data[pos], data[pos+1]);
    }

    template<class U> 
    inline U lerp_ptr_lookup_float(U *data) const {
        unsigned int pos = ui64part();
        return data[pos] + (data[pos+1]-data[pos]) * fpart_as_double();
    }
};

template<class T, int FractBits>
inline fixed_point<T, FractBits> operator*(int v, fixed_point<T, FractBits> v2) {
    v2 *= v;
    return v2;
}

/// wave position (unsigned 64-bit int including 24-bit fractional part)
typedef fixed_point<unsigned long long int, 24> wpos;

};

#endif
