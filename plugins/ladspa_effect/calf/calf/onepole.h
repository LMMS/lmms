/* Calf DSP Library
 * Basic one-pole one-zero filters based on bilinear transform.
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
#ifndef __CALF_ONEPOLE_H
#define __CALF_ONEPOLE_H

#include "primitives.h"

namespace dsp {

/**
 * one-pole filter, for floating point values
 * coefficient calculation is based on bilinear transform, and the code itself is based on my very old OneSignal lib
 * lp and hp are *somewhat* tested, allpass is not tested at all
 * don't use this for integers because it won't work
 */
template<class T = float, class Coeff = float>
class onepole
{
public:
    typedef std::complex<double> cfloat;

    T x1, y1;
    Coeff a0, a1, b1;

    onepole()
    {
        reset();
    }
    
    /// Set coefficients for a lowpass filter
    void set_lp(float fc, float sr)
    {
        //   x   x
        //  x+1 x-1
        Coeff x = tan (M_PI * fc / (2 * sr));
        Coeff q = 1/(1+x);
	a0 = a1 = x*q;
	b1 = (x-1)*q;
    }
    
    /// Set coefficients for an allpass filter
    void set_ap(float fc, float sr)
    {
        // x-1  x+1
        // x+1  x-1
	Coeff x = tan (M_PI * fc / (2 * sr));
	Coeff q = 1/(1+x);
	b1 = a0 = (x-1)*q;
        a1 = 1;
    }
    
    /// Set coefficients for an allpass filter, using omega instead of fc and sr
    /// omega = (PI / 2) * fc / sr
    void set_ap_w(float w)
    {
        // x-1  x+1
        // x+1  x-1
	Coeff x = tan (w);
	Coeff q = 1/(1+x);
	b1 = a0 = (x-1)*q;
        a1 = 1;
    }
    
    /// Set coefficients for a highpass filter
    void set_hp(float fc, float sr)
    {
        //   x   -x
        //  x+1  x-1
	Coeff x = tan (M_PI * fc / (2 * sr));
	Coeff q = 1/(1+x);
	a0 = q;
        a1 = -a0;
	b1 = (x-1)*q;
    }
    
    /// Process one sample
    inline T process(T in)
    {
        T out = in * a0 + x1 * a1 - y1 * b1;
        x1 = in;
        y1 = out;
        return out;
    }
    
    /// Process one sample, assuming it's a lowpass filter (optimized special case)
    inline T process_lp(T in)
    {
        T out = (in + x1) * a0 - y1 * b1;
        x1 = in;
        y1 = out;
        return out;
    }

    /// Process one sample, assuming it's a highpass filter (optimized special case)
    inline T process_hp(T in)
    {
        T out = (in - x1) * a0 - y1 * b1;
        x1 = in;
        y1 = out;
        return out;
    }
    
    /// Process one sample, assuming it's an allpass filter (optimized special case)
    inline T process_ap(T in)
    {
        T out = (in - y1) * a0 + x1;
        x1 = in;
        y1 = out;
        return out;
    }
    
    /// Process one sample using external state variables
    inline T process_ap(T in, float &x1, float &y1)
    {
        T out = (in - y1) * a0 + x1;
        x1 = in;
        y1 = out;
        return out;
    }
    
    /// Process one sample using external state variables, including filter coeff
    inline T process_ap(T in, float &x1, float &y1, float a0)
    {
        T out = (in - y1) * a0 + x1;
        x1 = in;
        y1 = out;
        return out;
    }
    
    inline bool empty() const {
        return y1 == 0;
    }
    
    inline void sanitize() 
    {
        dsp::sanitize(x1);
        dsp::sanitize(y1);
    }
    
    inline void reset()
    {
        dsp::zero(x1);
        dsp::zero(y1);
    }
    
    template<class U>
    inline void copy_coeffs(const onepole<U> &src)
    {
        a0 = src.a0;
        a1 = src.a1;
        b1 = src.b1;
    }
    
    /// Return the filter's gain at frequency freq
    /// @param freq   Frequency to look up
    /// @param sr     Filter sample rate (used to convert frequency to angular frequency)
    float freq_gain(float freq, float sr) const
    {
        freq *= 2.0 * M_PI / sr;
        cfloat z = 1.0 / exp(cfloat(0.0, freq));
        
        return std::abs(h_z(z));
    }
    
    /// Return H(z) the filter's gain at frequency freq
    /// @param z   Z variable (e^jw)
    cfloat h_z(const cfloat &z) const
    {
        return (cfloat(a0) + double(a1) * z) / (cfloat(1.0) + double(b1) * z);
    }
};

};

#endif
