/* Calf DSP Library
 * A-weighting filter for 
 * Copyright (C) 2001-2007 Krzysztof Foltman
 *
 * Most of code in this file is based on freely
 * available other work of other people (filter equations).
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
#ifndef __CALF_LOUDNESS_H
#define __CALF_LOUDNESS_H

#include "biquad.h"

namespace dsp {
    
class aweighter {
public:
    biquad_d2<float> bq1, bq2, bq3;
    
    /// Produce one output sample from one input sample
    float process(float sample)
    {
        return bq1.process(bq2.process(bq3.process(sample)));
    }
    
    /// Set sample rate (updates filter coefficients)
    void set(float sr)
    {
        // analog coeffs taken from: http://www.diracdelta.co.uk/science/source/a/w/aweighting/source.html
        // first we need to adjust them by doing some obscene sort of reverse pre-warping (a broken one, too!)
        float f1 = biquad_coeffs<float>::unwarpf(20.6f, sr);
        float f2 = biquad_coeffs<float>::unwarpf(107.7f, sr);
        float f3 = biquad_coeffs<float>::unwarpf(738.f, sr);
        float f4 = biquad_coeffs<float>::unwarpf(12200.f, sr);
        // then map s domain to z domain using bilinear transform
        // note: f1 and f4 are double poles
        bq1.set_bilinear(0, 0, 1, f1*f1, 2 * f1, 1);
        bq2.set_bilinear(1, 0, 0, f2*f3, f2 + f3, 1);
        bq3.set_bilinear(0, 0, 1, f4*f4, 2 * f4, 1);
        // the coeffs above give non-normalized value, so it should be normalized to produce 0dB at 1 kHz
        // find actual gain
        float gain1kHz = freq_gain(1000.0, sr);
        // divide one filter's x[n-m] coefficients by that value
        float gc = 1.0 / gain1kHz;
        bq1.a0 *= gc;
        bq1.a1 *= gc;
        bq1.a2 *= gc;
    }
    
    /// Reset to zero if at risk of denormals
    void sanitize()
    {
        bq1.sanitize();
        bq2.sanitize();
        bq3.sanitize();
    }
    
    /// Reset state to zero
    void reset()
    {
        bq1.reset();
        bq2.reset();
        bq3.reset();
    }
    
    /// Gain and a given frequency
    float freq_gain(float freq, float sr)
    {
        return bq1.freq_gain(freq, sr) * bq2.freq_gain(freq, sr) * bq3.freq_gain(freq, sr);
    }
    
};

};

#endif
