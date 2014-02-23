/* Calf DSP Library 
 * Peak metering facilities.
 *
 * Copyright (C) 2007 Krzysztof Foltman
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */
#ifndef __CALF_VUMETER_H
#define __CALF_VUMETER_H

#include <math.h>

namespace dsp {

/// Peak meter class
struct vumeter
{
    /// Measured signal level
    float level;
    /// Falloff of signal level (b1 coefficient of a 1-pole filter)
    float falloff;
    /// Clip indicator (set to 1 when |value| >= 1, fading otherwise)
    float clip;
    /// Falloff of clip indicator (b1 coefficient of a 1-pole filter); set to 1 if no falloff is required (manual reset of clip indicator)
    float clip_falloff;
    
    vumeter()
    {
        falloff = 0.999f;
        clip_falloff = 0.999f;
        reset();
    }
    
    void reset()
    {
        level = 0;
        clip = 0;
    }
    
    /// Set falloff so that the meter falls 20dB in time_20dB seconds, assuming sample rate of sample_rate
    /// @arg time_20dB time for the meter to move by 20dB (default 300ms if <= 0)
    void set_falloff(double time_20dB, double sample_rate)
    {
        if (time_20dB <= 0)
            time_20dB = 0.3;
        // 20dB = 10x +/- --> 0.1 = pow(falloff, sample_rate * time_20dB) = exp(sample_rate * ln(falloff))
        // ln(0.1) = sample_rate * ln(falloff)
        falloff = pow(0.1, 1 / (sample_rate * time_20dB));
        clip_falloff = falloff;
    }
    /// Copy falloff from another object
    void copy_falloff(const vumeter &src)
    {
        falloff = src.falloff;
        clip_falloff = src.clip_falloff;
    }
    
    /// Update peak meter based on input signal
    inline void update(const float *src, unsigned int len)
    {
        update_stereo(src, NULL, len);
    }
    /// Update peak meter based on louder of two input signals
    inline void update_stereo(const float *src1, const float *src2, unsigned int len)
    {
        // "Age" the old level by falloff^length
        level *= pow(falloff, len);
        // Same for clip level (using different fade constant)
        clip *= pow(clip_falloff, len);
        dsp::sanitize(level);
        dsp::sanitize(clip);
        // Process input samples - to get peak value, take a max of all values in the input signal and "aged" old peak
        // Clip is set to 1 if any sample is out-of-range, if no clip occurs, the "aged" value is assumed
        if (src1)
            run_sample_loop(src1, len);
        if (src2)
            run_sample_loop(src2, len);
    }
    inline void run_sample_loop(const float *src, unsigned int len)
    {
        float tmp = level;
        for (unsigned int i = 0; i < len; i++) {
            float sig = fabs(src[i]);
            tmp = std::max(tmp, sig);
            if (sig >= 1.f)
                clip = 1.f;
        }
        level = tmp;
    }
    /// Update clip meter as if update was called with all-zero input signal
    inline void update_zeros(unsigned int len)
    {
        level *= pow((double)falloff, (double)len);
        clip *= pow((double)clip_falloff, (double)len);
        dsp::sanitize(level);
        dsp::sanitize(clip);
    }
};

struct dual_vumeter
{
    vumeter left, right;
    
    inline void update_stereo(const float *src1, const float *src2, unsigned int len)
    {
        left.update_stereo(src1, NULL, len);
        right.update_stereo(NULL, src2, len);
    }
    inline void update_zeros(unsigned int len)
    {
        left.update_zeros(len);
        right.update_zeros(len);
    }
    inline void reset()
    {
        left.reset();
        right.reset();
    }
    inline void set_falloff(double time_20dB, double sample_rate)
    {
        left.set_falloff(time_20dB, sample_rate);
        right.copy_falloff(left);
    }
    inline void copy_falloff(const dual_vumeter &src)
    {
        left.copy_falloff(src.left);
        right.copy_falloff(src.right);
    }

};

};

#endif
