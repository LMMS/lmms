/* Calf DSP Library
 * Reusable audio effect classes.
 *
 * Copyright (C) 2001-2010 Krzysztof Foltman, Markus Schmidt, Thor Harald Johansen and others
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
#ifndef CALF_AUDIOFX_H
#define CALF_AUDIOFX_H

#include "biquad.h"
#include "delay.h"
#include "fixed_point.h"
#include "inertia.h"
#include "onepole.h"
#include <complex>

namespace calf_plugins {
    struct cairo_iface;
};

namespace dsp {
#if 0
}; to keep editor happy
#endif

/**
 * Audio effect base class. Not really useful until it gets more developed.
 */
class audio_effect
{
public:
    virtual void setup(int sample_rate)=0;
    virtual ~audio_effect() {}
};

class modulation_effect: public audio_effect
{
protected:
    int sample_rate;
    float rate, wet, dry, odsr;
    gain_smoothing gs_wet, gs_dry;
public:
    fixed_point<unsigned int, 20> phase, dphase;
    float get_rate() const {
        return rate;
    }
    void set_rate(float rate) {
        this->rate = rate;
        dphase = rate/sample_rate*4096;
    }
    float get_wet() const {
        return wet;
    }
    void set_wet(float wet) {
        this->wet = wet;
        gs_wet.set_inertia(wet);
    }
    float get_dry() const {
        return dry;
    }
    void set_dry(float dry) {
        this->dry = dry;
        gs_dry.set_inertia(dry);
    }
    void reset_phase(float req_phase)
    {
        phase = req_phase * 4096.0;
    }
    void inc_phase(float req_phase)
    {
        phase += fixed_point<unsigned int, 20>(req_phase * 4096.0);
    }
    void setup(int sample_rate)
    {
        this->sample_rate = sample_rate;
        this->odsr = 1.0 / sample_rate;
        phase = 0;
        set_rate(get_rate());
    }
};

/**
 * A monophonic phaser. If you want stereo, combine two :)
 * Also, gave up on using template args for signal type.
 */
class simple_phaser: public modulation_effect
{
protected:
    float base_frq, mod_depth, fb;
    float state;
    int cnt, stages, max_stages;
    dsp::onepole<float, float> stage1;
    float *x1, *y1;
public:
    simple_phaser(int _max_stages, float *x1vals, float *y1vals);

    float get_base_frq() const {
        return base_frq;
    }
    void set_base_frq(float _base_frq) {
        base_frq = _base_frq;
    }
    int get_stages() const {
        return stages;
    }
    void set_stages(int _stages);

    float get_mod_depth() const {
        return mod_depth;
    }
    void set_mod_depth(float _mod_depth) {
        mod_depth = _mod_depth;
    }

    float get_fb() const {
        return fb;
    }
    void set_fb(float fb) {
        this->fb = fb;
    }

    virtual void setup(int sample_rate) {
        modulation_effect::setup(sample_rate);
        reset();
    }
    void reset();
    void control_step();
    void process(float *buf_out, float *buf_in, int nsamples);
    float freq_gain(float freq, float sr) const;
};

/**
 * Base class for chorus and flanger. Wouldn't be needed if it wasn't
 * for odd behaviour of GCC when deriving templates from template
 * base classes (not seeing fields from base classes!).
 */
class chorus_base: public modulation_effect
{
protected:
    int min_delay_samples, mod_depth_samples;
    float min_delay, mod_depth;
    sine_table<int, 4096, 65536> sine;
public:
    float get_min_delay() const {
        return min_delay;
    }
    void set_min_delay(float min_delay) {
        this->min_delay = min_delay;
        this->min_delay_samples = (int)(min_delay * 65536.0 * sample_rate);
    }
    float get_mod_depth() const {
        return mod_depth;
    }
    void set_mod_depth(float mod_depth) {
        this->mod_depth = mod_depth;
        // 128 because it's then multiplied by (hopefully) a value of 32768..-32767
        this->mod_depth_samples = (int)(mod_depth * 32.0 * sample_rate);
    }
};

/**
 * Single-tap chorus without feedback.
 * Perhaps MaxDelay should be a bit longer!
 */
template<class T, int MaxDelay=512>
class simple_chorus: public chorus_base
{
protected:
    simple_delay<MaxDelay,T> delay;
public:
    simple_chorus() {
        rate = 0.63f;
        dry = 0.5f;
        wet = 0.5f;
        min_delay = 0.005f;
        mod_depth = 0.0025f;
        setup(44100);
    }
    void reset() {
        delay.reset();
    }
    virtual void setup(int sample_rate) {
        modulation_effect::setup(sample_rate);
        delay.reset();
        set_min_delay(get_min_delay());
        set_mod_depth(get_mod_depth());
    }
    template<class OutIter, class InIter>
    void process(OutIter buf_out, InIter buf_in, int nsamples) {
        int mds = min_delay_samples + mod_depth_samples * 1024 + 2*65536;
        int mdepth = mod_depth_samples;
        for (int i=0; i<nsamples; i++) {
            phase += dphase;
            unsigned int ipart = phase.ipart();

            float in = *buf_in++;
            int lfo = phase.lerp_by_fract_int<int, 14, int>(sine.data[ipart], sine.data[ipart+1]);
            int v = mds + (mdepth * lfo >> 6);
            // if (!(i & 7)) printf("%d\n", v);
            int ifv = v >> 16;
            delay.put(in);
            T fd; // signal from delay's output
            delay.get_interp(fd, ifv, (v & 0xFFFF)*(1.0/65536.0));
            T sdry = in * gs_dry.get();
            T swet = fd * gs_wet.get();
            *buf_out++ = sdry + swet;
        }
    }
};

/**
 * Single-tap flanger (chorus plus feedback).
 */
template<class T, int MaxDelay=1024>
class simple_flanger: public chorus_base
{
protected:
    simple_delay<MaxDelay,T> delay;
    float fb;
    int last_delay_pos, last_actual_delay_pos;
    int ramp_pos, ramp_delay_pos;
public:
    simple_flanger()
    : fb(0) {}
    void reset() {
        delay.reset();
        last_delay_pos = last_actual_delay_pos = ramp_delay_pos = 0;
        ramp_pos = 1024;
    }
    virtual void setup(int sample_rate) {
        this->sample_rate = sample_rate;
        this->odsr = 1.0 / sample_rate;
        delay.reset();
        phase = 0;
        set_rate(get_rate());
        set_min_delay(get_min_delay());
    }
    float get_fb() const {
        return fb;
    }
    void set_fb(float fb) {
        this->fb = fb;
    }
    template<class OutIter, class InIter>
    void process(OutIter buf_out, InIter buf_in, int nsamples) {
        if (!nsamples)
            return;
        int mds = this->min_delay_samples + this->mod_depth_samples * 1024 + 2 * 65536;
        int mdepth = this->mod_depth_samples;
        int delay_pos;
        unsigned int ipart = this->phase.ipart();
        int lfo = phase.lerp_by_fract_int<int, 14, int>(this->sine.data[ipart], this->sine.data[ipart+1]);
        delay_pos = mds + (mdepth * lfo >> 6);

        if (delay_pos != last_delay_pos || ramp_pos < 1024)
        {
            if (delay_pos != last_delay_pos) {
                // we need to ramp from what the delay tap length actually was,
                // not from old (ramp_delay_pos) or desired (delay_pos) tap length
                ramp_delay_pos = last_actual_delay_pos;
                ramp_pos = 0;
            }

            int64_t dp = 0;
            for (int i=0; i<nsamples; i++) {
                float in = *buf_in++;
                T fd; // signal from delay's output
                dp = (((int64_t)ramp_delay_pos) * (1024 - ramp_pos) + ((int64_t)delay_pos) * ramp_pos) >> 10;
                ramp_pos++;
                if (ramp_pos > 1024) ramp_pos = 1024;
                this->delay.get_interp(fd, dp >> 16, (dp & 0xFFFF)*(1.0/65536.0));
                sanitize(fd);
                T sdry = in * this->dry;
                T swet = fd * this->wet;
                *buf_out++ = sdry + swet;
                this->delay.put(in+fb*fd);

                this->phase += this->dphase;
                ipart = this->phase.ipart();
                lfo = phase.lerp_by_fract_int<int, 14, int>(this->sine.data[ipart], this->sine.data[ipart+1]);
                delay_pos = mds + (mdepth * lfo >> 6);
            }
            last_actual_delay_pos = dp;
        }
        else {
            for (int i=0; i<nsamples; i++) {
                float in = *buf_in++;
                T fd; // signal from delay's output
                this->delay.get_interp(fd, delay_pos >> 16, (delay_pos & 0xFFFF)*(1.0/65536.0));
                sanitize(fd);
                T sdry = in * this->gs_dry.get();
                T swet = fd * this->gs_wet.get();
                *buf_out++ = sdry + swet;
                this->delay.put(in+fb*fd);

                this->phase += this->dphase;
                ipart = this->phase.ipart();
                lfo = phase.lerp_by_fract_int<int, 14, int>(this->sine.data[ipart], this->sine.data[ipart+1]);
                delay_pos = mds + (mdepth * lfo >> 6);
            }
            last_actual_delay_pos = delay_pos;
        }
        last_delay_pos = delay_pos;
    }
    float freq_gain(float freq, float sr) const
    {
        typedef std::complex<double> cfloat;
        freq *= 2.0 * M_PI / sr;
        cfloat z = 1.0 / exp(cfloat(0.0, freq)); // z^-1

        float ldp = last_delay_pos / 65536.0;
        float fldp = floor(ldp);
        cfloat zn = std::pow(z, fldp); // z^-N
        cfloat zn1 = zn * z; // z^-(N+1)
        // simulate a lerped comb filter - H(z) = 1 / (1 + fb * (lerp(z^-N, z^-(N+1), fracpos))), N = int(pos), fracpos = pos - int(pos)
        cfloat delayed = zn + (zn1 - zn) * cfloat(ldp - fldp);
        cfloat h = cfloat(delayed) / (cfloat(1.0) - cfloat(fb) * delayed);
        // mix with dry signal
        float v = std::abs(cfloat(gs_dry.get_last()) + cfloat(gs_wet.get_last()) * h);
        return v;
    }
};

/**
 * A classic allpass loop reverb with modulated allpass filter.
 * Just started implementing it, so there is no control over many
 * parameters.
 */
class reverb: public audio_effect
{
    simple_delay<2048, float> apL1, apL2, apL3, apL4, apL5, apL6;
    simple_delay<2048, float> apR1, apR2, apR3, apR4, apR5, apR6;
    fixed_point<unsigned int, 25> phase, dphase;
    sine_table<int, 128, 10000> sine;
    onepole<float> lp_left, lp_right;
    float old_left, old_right;
    int type;
    float time, fb, cutoff, diffusion;
    int tl[6], tr[6];
    float ldec[6], rdec[6];

    int sr;
public:
    reverb()
    {
        phase = 0.0;
        time = 1.0;
        cutoff = 9000;
        type = 2;
        diffusion = 1.f;
        setup(44100);
    }
    virtual void setup(int sample_rate) {
        sr = sample_rate;
        set_time(time);
        set_cutoff(cutoff);
        phase = 0.0;
        dphase = 0.5*128/sr;
        update_times();
    }
    void update_times();
    float get_time() const {
        return time;
    }
    void set_time(float time) {
        this->time = time;
        // fb = pow(1.0f/4096.0f, (float)(1700/(time*sr)));
        fb = 1.0 - 0.3 / (time * sr / 44100.0);
    }
    float get_type() const {
        return type;
    }
    void set_type(int type) {
        this->type = type;
        update_times();
    }
    float get_diffusion() const {
        return diffusion;
    }
    void set_diffusion(float diffusion) {
        this->diffusion = diffusion;
        update_times();
    }
    void set_type_and_diffusion(int type, float diffusion) {
        this->type = type;
        this->diffusion = diffusion;
        update_times();
    }
    float get_fb() const
    {
        return this->fb;
    }
    void set_fb(float fb)
    {
        this->fb = fb;
    }
    float get_cutoff() const {
        return cutoff;
    }
    void set_cutoff(float cutoff) {
        this->cutoff = cutoff;
        lp_left.set_lp(cutoff,sr);
        lp_right.set_lp(cutoff,sr);
    }
    void reset();
    void process(float &left, float &right);
    void extra_sanitize()
    {
        lp_left.sanitize();
        lp_right.sanitize();
    }
};

class filter_module_iface
{
public:
    virtual void  calculate_filter(float freq, float q, int mode, float gain = 1.0) = 0;
    virtual void  filter_activate() = 0;
    virtual void  sanitize() = 0;
    virtual int   process_channel(uint16_t channel_no, const float *in, float *out, uint32_t numsamples, int inmask) = 0;
    virtual float freq_gain(int subindex, float freq, float srate) const = 0;

    virtual ~filter_module_iface() {}
};


class biquad_filter_module: public filter_module_iface
{
private:
    dsp::biquad_d1<float> left[3], right[3];
    int order;

public:
    uint32_t srate;

    enum { mode_12db_lp = 0, mode_24db_lp = 1, mode_36db_lp = 2,
           mode_12db_hp = 3, mode_24db_hp = 4, mode_36db_hp = 5,
           mode_6db_bp  = 6, mode_12db_bp = 7, mode_18db_bp = 8,
           mode_6db_br  = 9, mode_12db_br = 10, mode_18db_br = 11,
           mode_count
    };

public:
    biquad_filter_module()
    : order(0) {}
    /// Calculate filter coefficients based on parameters - cutoff/center frequency, q, filter type, output gain
    void calculate_filter(float freq, float q, int mode, float gain = 1.0);
    /// Reset filter state
    void filter_activate();
    /// Remove denormals
    void sanitize();
    /// Process a single channel (float buffer) of data
    int process_channel(uint16_t channel_no, const float *in, float *out, uint32_t numsamples, int inmask);
    /// Determine gain (|H(z)|) for a given frequency
    float freq_gain(int subindex, float freq, float srate) const;
};

class two_band_eq
{
private:
    dsp::onepole<float> lowcut, highcut;
    float low_gain, high_gain;

public:
    void reset()
    {
        lowcut.reset();
        highcut.reset();
    }

    inline float process(float v)
    {
        v = dsp::lerp(lowcut.process_hp(v), v, low_gain);
        v = dsp::lerp(highcut.process_lp(v), v, high_gain);
        return v;
    }

    inline void copy_coeffs(const two_band_eq &src)
    {
        lowcut.copy_coeffs(src.lowcut);
        highcut.copy_coeffs(src.highcut);
        low_gain = src.low_gain;
        high_gain = src.high_gain;
    }

    void sanitize()
    {
        lowcut.sanitize();
        highcut.sanitize();
    }

    void set(float _low_freq, float _low_gain, float _high_freq, float _high_gain, float sr)
    {
        lowcut.set_hp(_low_freq, sr);
        highcut.set_lp(_high_freq, sr);
        low_gain = _low_gain;
        high_gain = _high_gain;
    }
};

/// Tom Szilagyi's distortion code, used with permission
/// KF: I'm not 100% sure how this is supposed to work, but it does.
/// I'm planning to rewrite it using more modular approach when I have more time.
class tap_distortion {
private:
    float blend_old, drive_old;
    float meter;
    float rdrive, rbdr, kpa, kpb, kna, knb, ap, an, imr, kc, srct, sq, pwrq;
    float prev_med, prev_out;
public:
    uint32_t srate;
    bool is_active;
    tap_distortion();
    void activate();
    void deactivate();
    void set_params(float blend, float drive);
    void set_sample_rate(uint32_t sr);
    float process(float in);
    float get_distortion_level();
    static inline float M(float x)
    {
        return (fabs(x) > 0.000000001f) ? x : 0.0f;
    }

    static inline float D(float x)
    {
        x = fabs(x);
        return (x > 0.000000001f) ? sqrtf(x) : 0.0f;
    }
};

/// LFO module by Markus
/// This module provides simple LFO's (sine=0, triangle=1, square=2, saw_up=3, saw_down=4)
/// get_value() returns a value between -1 and 1
class simple_lfo {
private:
    float phase, freq, offset, amount;
    int mode;
    uint32_t srate;
    bool is_active;
public:
    simple_lfo();
    void set_params(float f, int m, float o, uint32_t sr, float amount = 1.f);
    float get_value();
    void advance(uint32_t count);
    void set_phase(float ph);
    void activate();
    void deactivate();
    float get_value_from_phase(float ph, float off) const;
    bool get_graph(float *data, int points, calf_plugins::cairo_iface *context) const;
    bool get_dot(float &x, float &y, int &size, calf_plugins::cairo_iface *context) const;
};


/// Lookahead Limiter by Markus Schmidt and Christian Holschuh
class lookahead_limiter {
private:
public:
    float limit, attack, release, weight;
    uint32_t srate;
    float att; // a coefficient the output is multiplied with
    float att_max; // a memory for the highest attenuation - used for display
    int pos; // where we are actually in our sample buffer
    int buffer_size;
    int overall_buffer_size;
    bool is_active;
    bool debug;
    bool auto_release;
    bool asc_active;
    float *buffer;
    int channels;
    float delta;
    float _delta;
    float peak;
    unsigned int over_s;
    float over_c;
    bool use_multi;
    unsigned int id;
    bool _sanitize;
    int nextiter;
    int nextlen;
    int * nextpos;
    float * nextdelta;
    int asc_c;
    float asc;
    int asc_pos;
    bool asc_changed;
    float asc_coeff;
    static inline void denormal(volatile float *f) {
	    *f += 1e-18;
	    *f -= 1e-18;
    }
    void reset();
    void reset_asc();
    bool get_asc();
    lookahead_limiter();
	~lookahead_limiter();
    void set_multi(bool set);
    void process(float &left, float &right, float *multi_buffer);
    void set_sample_rate(uint32_t sr);
    void set_params(float l, float a, float r, float weight = 1.f, bool ar = false, float arc = 1.f, bool d = false);
    float get_attenuation();
    void activate();
    void deactivate();
};

#if 0
{ to keep editor happy
#endif
}

#endif
