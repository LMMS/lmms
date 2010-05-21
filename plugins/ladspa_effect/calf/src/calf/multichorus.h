/* Calf DSP Library
 * Multitap chorus class.
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
#ifndef __CALF_MULTICHORUS_H
#define __CALF_MULTICHORUS_H

#include "audio_fx.h"

namespace dsp {

typedef fixed_point<unsigned int, 20> chorus_phase;

template<class T, uint32_t Voices>
class sine_multi_lfo
{
protected:
    sine_table<int, 4096, 65535> sine;
    
public:
    /// Current LFO phase
    chorus_phase phase;
    /// LFO phase increment
    chorus_phase dphase;
    /// LFO phase per-voice increment
    chorus_phase vphase;
    /// Current number of voices
    uint32_t voices;
    /// Current scale (output multiplier)
    T scale;
    /// Per-voice offset unit (the value that says how much the voices are offset with respect to each other in non-100% 'overlap' mode), scaled so that full range = 131072
    int32_t voice_offset;
    /// LFO Range scaling for non-100% overlap
    uint32_t voice_depth;
public:
    sine_multi_lfo()
    {
        phase = dphase = vphase = 0.0;
        voice_offset = 0;
        voice_depth = 1U << 31;
        
        set_voices(Voices);
    }
    inline uint32_t get_voices() const
    {
        return voices;
    }
    inline void set_voices(uint32_t value)
    {
        voices = value;
        // use sqrt, because some phases will cancel each other - so 1 / N is usually too low
        scale = sqrt(1.0 / voices);
    }
    inline void set_overlap(float overlap)
    {
        // If we scale the delay amount so that full range of a single LFO is 0..1, all the overlapped LFOs will cover 0..range
        // How it's calculated:
        // 1. First voice is assumed to always cover the range of 0..1
        // 2. Each remaining voice contributes an interval of a width = 1 - overlap, starting from the end of the interval of the previous voice
        // Coverage = non-overlapped part of the LFO range in the 1st voice
        float range = 1.f + (1.f - overlap) * (voices - 1);
        float scaling = 1.f / range;
        voice_offset = (int)(131072 * (1 - overlap) / range);
        voice_depth = (unsigned int)((1U << 30) * 1.0 * scaling);
    }
    /// Get LFO value for given voice, returns a values in range of [-65536, 65535] (or close)
    inline int get_value(uint32_t voice) const {
        // find this voice's phase (= phase + voice * 360 degrees / number of voices)
        chorus_phase voice_phase = phase + vphase * (int)voice;
        // find table offset
        unsigned int ipart = voice_phase.ipart();
        // interpolate (use 14 bits of precision - because the table itself uses 17 bits and the result of multiplication must fit in int32_t)
        // note, the result is still -65535 .. 65535, it's just interpolated
        // it is never reaching -65536 - but that's acceptable
        int intval = voice_phase.lerp_by_fract_int<int, 14, int>(sine.data[ipart], sine.data[ipart+1]);
        // apply the voice offset/depth (rescale from -65535..65535 to appropriate voice's "band")
        return -65535 + voice * voice_offset + ((voice_depth >> (30-13)) * (65536 + intval) >> 13);
    }
    inline void step() {
        phase += dphase;
    }
    inline T get_scale() const {
        return scale;
    }
    void reset() {
        phase = 0.f;
    }
};

/**
 * Multi-tap chorus without feedback.
 * Perhaps MaxDelay should be a bit longer!
 */
template<class T, class MultiLfo, class Postprocessor, int MaxDelay=4096>
class multichorus: public chorus_base
{
protected:
    simple_delay<MaxDelay,T> delay;
public:    
    MultiLfo lfo;
    Postprocessor post;
public:
    multichorus() {
        rate = 0.63f;
        dry = 0.5f;
        wet = 0.5f;
        min_delay = 0.005f;
        mod_depth = 0.0025f;
        setup(44100);
    }
    void reset() {
        delay.reset();
        lfo.reset();
    }
    void set_rate(float rate) {
        chorus_base::set_rate(rate);
        lfo.dphase = dphase;
    }
    virtual void setup(int sample_rate) {
        modulation_effect::setup(sample_rate);
        delay.reset();
        lfo.reset();
        set_min_delay(get_min_delay());
        set_mod_depth(get_mod_depth());
    }
    template<class OutIter, class InIter>
    void process(OutIter buf_out, InIter buf_in, int nsamples) {
        int mds = min_delay_samples + mod_depth_samples * 1024 + 2*65536;
        int mdepth = mod_depth_samples;
        // 1 sample peak-to-peak = mod_depth_samples of 32 (this scaling stuff is tricky and may - but shouldn't - be wrong)
        // with 192 kHz sample rate, 1 ms = 192 samples, and the maximum 20 ms = 3840 samples (so, 4096 will be used)
        // 3840 samples of mod depth = mdepth of 122880 (which multiplied by 65536 doesn't fit in int32_t)
        // so, it will be right-shifted by 2, which gives it a safe range of 30720
        // NB: calculation of mod_depth_samples (and multiply-by-32) is in chorus_base::set_mod_depth
        mdepth = mdepth >> 2;
        T scale = lfo.get_scale();
        for (int i=0; i<nsamples; i++) {
            phase += dphase;
            
            float in = *buf_in++;
            
            delay.put(in);
            unsigned int nvoices = lfo.get_voices();
            T out = 0.f;
            // add up values from all voices, each voice tell its LFO phase and the buffer value is picked at that location
            for (unsigned int v = 0; v < nvoices; v++)
            {
                int lfo_output = lfo.get_value(v);
                // 3 = log2(32 >> 2) + 1 because the LFO value is in range of [-65535, 65535] (17 bits)
                int dv = mds + (mdepth * lfo_output >> (3 + 1));
                int ifv = dv >> 16;
                T fd; // signal from delay's output
                delay.get_interp(fd, ifv, (dv & 0xFFFF)*(1.0/65536.0));
                out += fd;
            }
            // apply the post filter
            out = post.process(out);
            T sdry = in * gs_dry.get();
            T swet = out * gs_wet.get() * scale;
            *buf_out++ = sdry + swet;
            lfo.step();
        }
        post.sanitize();
    }
    float freq_gain(float freq, float sr) const
    {
        typedef std::complex<double> cfloat;
        freq *= 2.0 * M_PI / sr;
        cfloat z = 1.0 / exp(cfloat(0.0, freq)); // z^-1        
        cfloat h = 0.0;
        int mds = min_delay_samples + mod_depth_samples * 1024 + 2*65536;
        int mdepth = mod_depth_samples;
        mdepth = mdepth >> 2;
        T scale = lfo.get_scale();
        unsigned int nvoices = lfo.get_voices();
        for (unsigned int v = 0; v < nvoices; v++)
        {
            int lfo_output = lfo.get_value(v);
            // 3 = log2(32 >> 2) + 1 because the LFO value is in range of [-65535, 65535] (17 bits)
            int dv = mds + (mdepth * lfo_output >> (3 + 1));
            int fldp = dv >> 16;
            cfloat zn = std::pow(z, fldp); // z^-N
            h += zn + (zn * z - zn) * cfloat(dv / 65536.0 - fldp);
        }
        // apply the post filter
        h *= post.h_z(z);
        // mix with dry signal
        float v = std::abs(cfloat(gs_dry.get_last()) + cfloat(scale * gs_wet.get_last()) * h);
        return v;
    }
};

};

#endif


