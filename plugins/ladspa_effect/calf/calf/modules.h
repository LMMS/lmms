/* Calf DSP Library
 * Example audio modules
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
#ifndef __CALF_MODULES_H
#define __CALF_MODULES_H

#include <assert.h>
#include <limits.h>
#include "biquad.h"
#include "inertia.h"
#include "audio_fx.h"
#include "multichorus.h"
#include "giface.h"
#include "metadata.h"
#include "loudness.h"
#include "primitives.h"

namespace calf_plugins {

using namespace dsp;

struct ladspa_plugin_info;
    
#if 0
class amp_audio_module: public null_audio_module
{
public:
    enum { in_count = 2, out_count = 2, param_count = 1, support_midi = false, require_midi = false, rt_capable = true };
    float *ins[2]; 
    float *outs[2];
    float *params[1];
    uint32_t srate;
    static parameter_properties param_props[];
    uint32_t process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask) {
        if (!inputs_mask)
            return 0;
        float gain = *params[0];
        numsamples += offset;
        for (uint32_t i = offset; i < numsamples; i++) {
            outs[0][i] = ins[0][i] * gain;
            outs[1][i] = ins[1][i] * gain;
        }
        return inputs_mask;
    }
};
#endif

class frequency_response_line_graph: public line_graph_iface 
{
public:
    bool get_gridline(int index, int subindex, float &pos, bool &vertical, std::string &legend, cairo_iface *context);
    virtual int get_changed_offsets(int generation, int &subindex_graph, int &subindex_dot, int &subindex_gridline);
};

class flanger_audio_module: public audio_module<flanger_metadata>, public frequency_response_line_graph
{
public:
    dsp::simple_flanger<float, 2048> left, right;
    float *ins[in_count]; 
    float *outs[out_count];
    float *params[param_count];
    uint32_t srate;
    bool clear_reset;
    float last_r_phase;
    bool is_active;
public:
    flanger_audio_module() {
        is_active = false;
    }
    void set_sample_rate(uint32_t sr);
    void params_changed() {
        float dry = *params[par_dryamount];
        float wet = *params[par_amount];
        float rate = *params[par_rate]; // 0.01*pow(1000.0f,*params[par_rate]);
        float min_delay = *params[par_delay] / 1000.0;
        float mod_depth = *params[par_depth] / 1000.0;
        float fb = *params[par_fb];
        left.set_dry(dry); right.set_dry(dry);
        left.set_wet(wet); right.set_wet(wet);
        left.set_rate(rate); right.set_rate(rate);
        left.set_min_delay(min_delay); right.set_min_delay(min_delay);
        left.set_mod_depth(mod_depth); right.set_mod_depth(mod_depth);
        left.set_fb(fb); right.set_fb(fb);
        float r_phase = *params[par_stereo] * (1.f / 360.f);
        clear_reset = false;
        if (*params[par_reset] >= 0.5) {
            clear_reset = true;
            left.reset_phase(0.f);
            right.reset_phase(r_phase);
        } else {
            if (fabs(r_phase - last_r_phase) > 0.0001f) {
                right.phase = left.phase;
                right.inc_phase(r_phase);
                last_r_phase = r_phase;
            }
        }
    }
    void params_reset()
    {
        if (clear_reset) {
            *params[par_reset] = 0.f;
            clear_reset = false;
        }
    }
    void activate();
    void deactivate();
    uint32_t process(uint32_t offset, uint32_t nsamples, uint32_t inputs_mask, uint32_t outputs_mask) {
        left.process(outs[0] + offset, ins[0] + offset, nsamples);
        right.process(outs[1] + offset, ins[1] + offset, nsamples);
        return outputs_mask; // XXXKF allow some delay after input going blank
    }
    bool get_graph(int index, int subindex, float *data, int points, cairo_iface *context);
    float freq_gain(int subindex, float freq, float srate);
};

class phaser_audio_module: public audio_module<phaser_metadata>, public frequency_response_line_graph
{
public:
    float *ins[in_count]; 
    float *outs[out_count];
    float *params[param_count];
    uint32_t srate;
    bool clear_reset;
    float last_r_phase;
    dsp::simple_phaser<12> left, right;
    bool is_active;
public:
    phaser_audio_module() {
        is_active = false;
    }
    void params_changed() {
        float dry = *params[par_dryamount];
        float wet = *params[par_amount];
        float rate = *params[par_rate]; // 0.01*pow(1000.0f,*params[par_rate]);
        float base_frq = *params[par_freq];
        float mod_depth = *params[par_depth];
        float fb = *params[par_fb];
        int stages = (int)*params[par_stages];
        left.set_dry(dry); right.set_dry(dry);
        left.set_wet(wet); right.set_wet(wet);
        left.set_rate(rate); right.set_rate(rate);
        left.set_base_frq(base_frq); right.set_base_frq(base_frq);
        left.set_mod_depth(mod_depth); right.set_mod_depth(mod_depth);
        left.set_fb(fb); right.set_fb(fb);
        left.set_stages(stages); right.set_stages(stages);
        float r_phase = *params[par_stereo] * (1.f / 360.f);
        clear_reset = false;
        if (*params[par_reset] >= 0.5) {
            clear_reset = true;
            left.reset_phase(0.f);
            right.reset_phase(r_phase);
        } else {
            if (fabs(r_phase - last_r_phase) > 0.0001f) {
                right.phase = left.phase;
                right.inc_phase(r_phase);
                last_r_phase = r_phase;
            }
        }
    }
    void params_reset()
    {
        if (clear_reset) {
            *params[par_reset] = 0.f;
            clear_reset = false;
        }
    }
    void activate();
    void set_sample_rate(uint32_t sr);
    void deactivate();
    uint32_t process(uint32_t offset, uint32_t nsamples, uint32_t inputs_mask, uint32_t outputs_mask) {
        left.process(outs[0] + offset, ins[0] + offset, nsamples);
        right.process(outs[1] + offset, ins[1] + offset, nsamples);
        return outputs_mask; // XXXKF allow some delay after input going blank
    }
    bool get_graph(int index, int subindex, float *data, int points, cairo_iface *context);
    bool get_gridline(int index, int subindex, float &pos, bool &vertical, std::string &legend, cairo_iface *context);
    float freq_gain(int subindex, float freq, float srate);
};

class reverb_audio_module: public audio_module<reverb_metadata>
{
public:    
    dsp::reverb<float> reverb;
    dsp::simple_delay<16384, stereo_sample<float> > pre_delay;
    dsp::onepole<float> left_lo, right_lo, left_hi, right_hi;
    uint32_t srate;
    gain_smoothing amount, dryamount;
    int predelay_amt;
    float *ins[in_count]; 
    float *outs[out_count];
    float *params[param_count];
    
    void params_changed() {
        //reverb.set_time(0.5*pow(8.0f, *params[par_decay]));
        //reverb.set_cutoff(2000*pow(10.0f, *params[par_hfdamp]));
        reverb.set_type_and_diffusion(fastf2i_drm(*params[par_roomsize]), *params[par_diffusion]);
        reverb.set_time(*params[par_decay]);
        reverb.set_cutoff(*params[par_hfdamp]);
        amount.set_inertia(*params[par_amount]);
        dryamount.set_inertia(*params[par_dry]);
        left_lo.set_lp(dsp::clip(*params[par_treblecut], 20.f, (float)(srate * 0.49f)), srate);
        left_hi.set_hp(dsp::clip(*params[par_basscut], 20.f, (float)(srate * 0.49f)), srate);
        right_lo.copy_coeffs(left_lo);
        right_hi.copy_coeffs(left_hi);
        predelay_amt = (int) (srate * (*params[par_predelay]) * (1.0f / 1000.0f) + 1);
    }
    uint32_t process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask) {
        numsamples += offset;
        
        for (uint32_t i = offset; i < numsamples; i++) {
            float dry = dryamount.get();
            float wet = amount.get();
            stereo_sample<float> s(ins[0][i], ins[1][i]);
            stereo_sample<float> s2 = pre_delay.process(s, predelay_amt);
            
            float rl = s2.left, rr = s2.right;
            rl = left_lo.process(left_hi.process(rl));
            rr = right_lo.process(right_hi.process(rr));
            reverb.process(rl, rr);
            outs[0][i] = dry*s.left + wet*rl;
            outs[1][i] = dry*s.right + wet*rr;
        }
        reverb.extra_sanitize();
        left_lo.sanitize();
        left_hi.sanitize();
        right_lo.sanitize();
        right_hi.sanitize();
        return outputs_mask;
    }
    void activate();
    void set_sample_rate(uint32_t sr);
    void deactivate();
};

class vintage_delay_audio_module: public audio_module<vintage_delay_metadata>
{
public:    
    // 1MB of delay memory per channel... uh, RAM is cheap
    enum { MAX_DELAY = 262144, ADDR_MASK = MAX_DELAY - 1 };
    float *ins[in_count]; 
    float *outs[out_count];
    float *params[param_count];
    float buffers[2][MAX_DELAY];
    int bufptr, deltime_l, deltime_r, mixmode, medium, old_medium;
    /// number of table entries written (value is only important when it is less than MAX_DELAY, which means that the buffer hasn't been totally filled yet)
    int age;
    
    gain_smoothing amt_left, amt_right, fb_left, fb_right;
    float dry;
    
    dsp::biquad_d2<float> biquad_left[2], biquad_right[2];
    
    uint32_t srate;
    
    vintage_delay_audio_module()
    {
        old_medium = -1;
        for (int i = 0; i < MAX_DELAY; i++) {
            buffers[0][i] = 0.f;
            buffers[1][i] = 0.f;
        }
    }
    
    void params_changed()
    {
        float unit = 60.0 * srate / (*params[par_bpm] * *params[par_divide]);
        deltime_l = dsp::fastf2i_drm(unit * *params[par_time_l]);
        deltime_r = dsp::fastf2i_drm(unit * *params[par_time_r]);
        amt_left.set_inertia(*params[par_amount]); amt_right.set_inertia(*params[par_amount]);
        float fb = *params[par_feedback];
        dry = *params[par_dryamount];
        mixmode = dsp::fastf2i_drm(*params[par_mixmode]);
        medium = dsp::fastf2i_drm(*params[par_medium]);
        if (mixmode == 0)
        {
            fb_left.set_inertia(fb);
            fb_right.set_inertia(pow(fb, *params[par_time_r] / *params[par_time_l]));
        } else {
            fb_left.set_inertia(fb);
            fb_right.set_inertia(fb);
        }
        if (medium != old_medium)
            calc_filters();
    }
    void activate() {
        bufptr = 0;
        age = 0;
    }
    void deactivate() {
    }
    void set_sample_rate(uint32_t sr) {
        srate = sr;
        old_medium = -1;
        amt_left.set_sample_rate(sr); amt_right.set_sample_rate(sr);
        fb_left.set_sample_rate(sr); fb_right.set_sample_rate(sr);
    }
    void calc_filters()
    {
        // parameters are heavily influenced by gordonjcp and his tape delay unit
        // although, don't blame him if it sounds bad - I've messed with them too :)
        biquad_left[0].set_lp_rbj(6000, 0.707, srate);
        biquad_left[1].set_bp_rbj(4500, 0.250, srate);
        biquad_right[0].copy_coeffs(biquad_left[0]);
        biquad_right[1].copy_coeffs(biquad_left[1]);
    }
    uint32_t process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask) {
        uint32_t ostate = 3; // XXXKF optimize!
        uint32_t end = offset + numsamples;
        int v = mixmode ? 1 : 0;
        int orig_bufptr = bufptr;
        for(uint32_t i = offset; i < end; i++)
        {
            float out_left, out_right, del_left, del_right;
            // if the buffer hasn't been cleared yet (after activation), pretend we've read zeros

            if (deltime_l >= age) {
                del_left = ins[0][i];
                out_left = dry * del_left;
                amt_left.step();
                fb_left.step();
            }
            else
            {
                float in_left = buffers[v][(bufptr - deltime_l) & ADDR_MASK];
                dsp::sanitize(in_left);
                out_left = dry * ins[0][i] + in_left * amt_left.get();
                del_left = ins[0][i] + in_left * fb_left.get();
            }
            if (deltime_r >= age) {
                del_right = ins[1][i];
                out_right = dry * del_right;
                amt_right.step();
                fb_right.step();
            }
            else
            {
                float in_right = buffers[1 - v][(bufptr - deltime_r) & ADDR_MASK];
                dsp::sanitize(in_right);
                out_right = dry * ins[1][i] + in_right * amt_right.get();
                del_right = ins[1][i] + in_right * fb_right.get();
            }
            
            age++;
            outs[0][i] = out_left; outs[1][i] = out_right; buffers[0][bufptr] = del_left; buffers[1][bufptr] = del_right;
            bufptr = (bufptr + 1) & (MAX_DELAY - 1);
        }
        if (age >= MAX_DELAY)
            age = MAX_DELAY;
        if (medium > 0) {
            bufptr = orig_bufptr;
            if (medium == 2)
            {
                for(uint32_t i = offset; i < end; i++)
                {
                    buffers[0][bufptr] = biquad_left[0].process_lp(biquad_left[1].process(buffers[0][bufptr]));
                    buffers[1][bufptr] = biquad_right[0].process_lp(biquad_right[1].process(buffers[1][bufptr]));
                    bufptr = (bufptr + 1) & (MAX_DELAY - 1);
                }
                biquad_left[0].sanitize();biquad_right[0].sanitize();
            } else {
                for(uint32_t i = offset; i < end; i++)
                {
                    buffers[0][bufptr] = biquad_left[1].process(buffers[0][bufptr]);
                    buffers[1][bufptr] = biquad_right[1].process(buffers[1][bufptr]);
                    bufptr = (bufptr + 1) & (MAX_DELAY - 1);
                }
            }
            biquad_left[1].sanitize();biquad_right[1].sanitize();
            
        }
        return ostate;
    }
};

class rotary_speaker_audio_module: public audio_module<rotary_speaker_metadata>
{
public:
    float *ins[in_count]; 
    float *outs[out_count];
    float *params[param_count];
    /// Current phases and phase deltas for bass and treble rotors
    uint32_t phase_l, dphase_l, phase_h, dphase_h;
    dsp::simple_delay<1024, float> delay;
    dsp::biquad_d2<float> crossover1l, crossover1r, crossover2l, crossover2r;
    dsp::simple_delay<8, float> phaseshift;
    uint32_t srate;
    int vibrato_mode;
    /// Current CC1 (Modulation) value, normalized to [0, 1]
    float mwhl_value;
    /// Current CC64 (Hold) value, normalized to [0, 1]
    float hold_value;
    /// Current rotation speed for bass rotor - automatic mode
    float aspeed_l;
    /// Current rotation speed for treble rotor - automatic mode
    float aspeed_h;
    /// Desired speed (0=slow, 1=fast) - automatic mode
    float dspeed;
    /// Current rotation speed for bass rotor - manual mode
    float maspeed_l;
    /// Current rotation speed for treble rotor - manual mode
    float maspeed_h;

    rotary_speaker_audio_module();
    void set_sample_rate(uint32_t sr);
    void setup();
    void activate();
    void deactivate();
    
    void params_changed() {
        set_vibrato();
    }
    void set_vibrato()
    {
        vibrato_mode = fastf2i_drm(*params[par_speed]);
        // manual vibrato - do not recalculate speeds as they're not used anyway
        if (vibrato_mode == 5) 
            return;
        if (!vibrato_mode)
            dspeed = -1;
        else {
            float speed = vibrato_mode - 1;
            if (vibrato_mode == 3)
                speed = hold_value;
            if (vibrato_mode == 4)
                speed = mwhl_value;
            dspeed = (speed < 0.5f) ? 0 : 1;
        }
        update_speed();
    }
    /// Convert RPM speed to delta-phase
    inline uint32_t rpm2dphase(float rpm)
    {
        return (uint32_t)((rpm / (60.0 * srate)) * (1 << 30)) << 2;
    }
    /// Set delta-phase variables based on current calculated (and interpolated) RPM speed
    void update_speed()
    {
        float speed_h = aspeed_h >= 0 ? (48 + (400-48) * aspeed_h) : (48 * (1 + aspeed_h));
        float speed_l = aspeed_l >= 0 ? 40 + (342-40) * aspeed_l : (40 * (1 + aspeed_l));
        dphase_h = rpm2dphase(speed_h);
        dphase_l = rpm2dphase(speed_l);
    }
    void update_speed_manual(float delta)
    {
        float ts = *params[par_treblespeed];
        float bs = *params[par_bassspeed];
        incr_towards(maspeed_h, ts, delta * 200, delta * 200);
        incr_towards(maspeed_l, bs, delta * 200, delta * 200);
        dphase_h = rpm2dphase(maspeed_h);
        dphase_l = rpm2dphase(maspeed_l);
    }
    /// map a ramp [int] to a sinusoid-like function [0, 65536]
    static inline int pseudo_sine_scl(int counter)
    {
        // premature optimization is a root of all evil; it can be done with integers only - but later :)
        double v = counter * (1.0 / (65536.0 * 32768.0));
        return (int) (32768 + 32768 * (v - v*v*v) * (1.0 / 0.3849));
    }
    /// Increase or decrease aspeed towards raspeed, with required negative and positive rate
    inline bool incr_towards(float &aspeed, float raspeed, float delta_decc, float delta_acc)
    {
        if (aspeed < raspeed) {
            aspeed = std::min(raspeed, aspeed + delta_acc);
            return true;
        }
        else if (aspeed > raspeed) 
        {
            aspeed = std::max(raspeed, aspeed - delta_decc);
            return true;
        }        
        return false;
    }
    uint32_t process(uint32_t offset, uint32_t nsamples, uint32_t inputs_mask, uint32_t outputs_mask)
    {
        int shift = (int)(300000 * (*params[par_shift])), pdelta = (int)(300000 * (*params[par_spacing]));
        int md = (int)(100 * (*params[par_moddepth]));
        float mix = 0.5 * (1.0 - *params[par_micdistance]);
        float mix2 = *params[par_reflection];
        float mix3 = mix2 * mix2;
        for (unsigned int i = 0; i < nsamples; i++) {
            float in_l = ins[0][i + offset], in_r = ins[1][i + offset];
            float in_mono = 0.5f * (in_l + in_r);
            
            int xl = pseudo_sine_scl(phase_l), yl = pseudo_sine_scl(phase_l + 0x40000000);
            int xh = pseudo_sine_scl(phase_h), yh = pseudo_sine_scl(phase_h + 0x40000000);
            // printf("%d %d %d\n", shift, pdelta, shift + pdelta + 20 * xl);
            
            // float out_hi_l = in_mono - delay.get_interp_1616(shift + md * xh) + delay.get_interp_1616(shift + md * 65536 + pdelta - md * yh) - delay.get_interp_1616(shift + md * 65536 + pdelta + pdelta - md * xh);
            // float out_hi_r = in_mono + delay.get_interp_1616(shift + md * 65536 - md * yh) - delay.get_interp_1616(shift + pdelta + md * xh) + delay.get_interp_1616(shift + pdelta + pdelta + md * yh);
            float out_hi_l = in_mono + delay.get_interp_1616(shift + md * xh) - mix2 * delay.get_interp_1616(shift + md * 65536 + pdelta - md * yh) + mix3 * delay.get_interp_1616(shift + md * 65536 + pdelta + pdelta - md * xh);
            float out_hi_r = in_mono + delay.get_interp_1616(shift + md * 65536 - md * yh) - mix2 * delay.get_interp_1616(shift + pdelta + md * xh) + mix3 * delay.get_interp_1616(shift + pdelta + pdelta + md * yh);

            float out_lo_l = in_mono + delay.get_interp_1616(shift + md * xl); // + delay.get_interp_1616(shift + md * 65536 + pdelta - md * yl);
            float out_lo_r = in_mono + delay.get_interp_1616(shift + md * yl); // - delay.get_interp_1616(shift + pdelta + md * yl);
            
            out_hi_l = crossover2l.process(out_hi_l); // sanitize(out_hi_l);
            out_hi_r = crossover2r.process(out_hi_r); // sanitize(out_hi_r);
            out_lo_l = crossover1l.process(out_lo_l); // sanitize(out_lo_l);
            out_lo_r = crossover1r.process(out_lo_r); // sanitize(out_lo_r);
            
            float out_l = out_hi_l + out_lo_l;
            float out_r = out_hi_r + out_lo_r;
            
            float mic_l = out_l + mix * (out_r - out_l);
            float mic_r = out_r + mix * (out_l - out_r);
            
            outs[0][i + offset] = mic_l * 0.5f;
            outs[1][i + offset] = mic_r * 0.5f;
            delay.put(in_mono);
            phase_l += dphase_l;
            phase_h += dphase_h;
        }
        crossover1l.sanitize();
        crossover1r.sanitize();
        crossover2l.sanitize();
        crossover2r.sanitize();
        float delta = nsamples * 1.0 / srate;
        if (vibrato_mode == 5)
            update_speed_manual(delta);
        else
        {
            bool u1 = incr_towards(aspeed_l, dspeed, delta * 0.2, delta * 0.14);
            bool u2 = incr_towards(aspeed_h, dspeed, delta, delta * 0.5);
            if (u1 || u2)
                set_vibrato();
        }
        return outputs_mask;
    }
    virtual void control_change(int ctl, int val);
};

/// Compose two filters in series
template<class F1, class F2>
class filter_compose {
public:
    typedef std::complex<float> cfloat;
    F1 f1;
    F2 f2;
public:
    float process(float value) {
        return f2.process(f1.process(value));
    }
    
    cfloat h_z(const cfloat &z) {
        return f1.h_z(z) * f2.h_z(z);
    }
    
    /// Return the filter's gain at frequency freq
    /// @param freq   Frequency to look up
    /// @param sr     Filter sample rate (used to convert frequency to angular frequency)
    float freq_gain(float freq, float sr)
    {
        typedef std::complex<double> cfloat;
        freq *= 2.0 * M_PI / sr;
        cfloat z = 1.0 / exp(cfloat(0.0, freq));
        
        return std::abs(h_z(z));
    }
    
    void sanitize() {
        f1.sanitize();
        f2.sanitize();
    }
};

/// Compose two filters in parallel
template<class F1, class F2>
class filter_sum {
public:
    typedef std::complex<double> cfloat;
    F1 f1;
    F2 f2;
public:
    float process(float value) {
        return f2.process(value) + f1.process(value);
    }
    
    inline cfloat h_z(const cfloat &z) {
        return f1.h_z(z) + f2.h_z(z);
    }
    
    /// Return the filter's gain at frequency freq
    /// @param freq   Frequency to look up
    /// @param sr     Filter sample rate (used to convert frequency to angular frequency)
    float freq_gain(float freq, float sr)
    {
        typedef std::complex<double> cfloat;
        freq *= 2.0 * M_PI / sr;
        cfloat z = 1.0 / exp(cfloat(0.0, freq));
        
        return std::abs(h_z(z));
    }
    
    void sanitize() {
        f1.sanitize();
        f2.sanitize();
    }
};

template<typename FilterClass, typename Metadata>
class filter_module_with_inertia: public FilterClass
{
public:
    typedef filter_module_with_inertia inertia_filter_module;
    
    float *ins[Metadata::in_count]; 
    float *outs[Metadata::out_count];
    float *params[Metadata::param_count];

    inertia<exponential_ramp> inertia_cutoff, inertia_resonance, inertia_gain;
    once_per_n timer;
    bool is_active;    
    volatile int last_generation, last_calculated_generation;
    
    filter_module_with_inertia()
    : inertia_cutoff(exponential_ramp(128), 20)
    , inertia_resonance(exponential_ramp(128), 20)
    , inertia_gain(exponential_ramp(128), 1.0)
    , timer(128)
    {
        is_active = false;
    }
    
    void calculate_filter()
    {
        float freq = inertia_cutoff.get_last();
        // printf("freq=%g inr.cnt=%d timer.left=%d\n", freq, inertia_cutoff.count, timer.left);
        // XXXKF this is resonance of a single stage, obviously for three stages, resonant gain will be different
        float q    = inertia_resonance.get_last();
        int   mode = dsp::fastf2i_drm(*params[Metadata::par_mode]);
        // printf("freq = %f q = %f mode = %d\n", freq, q, mode);
        
        int inertia = dsp::fastf2i_drm(*params[Metadata::par_inertia]);
        if (inertia != inertia_cutoff.ramp.length()) {
            inertia_cutoff.ramp.set_length(inertia);
            inertia_resonance.ramp.set_length(inertia);
            inertia_gain.ramp.set_length(inertia);
        }
        
        FilterClass::calculate_filter(freq, q, mode, inertia_gain.get_last());
    }
    
    virtual void params_changed()
    {
        calculate_filter();
    }
    
    void on_timer()
    {
        int gen = last_generation;
        inertia_cutoff.step();
        inertia_resonance.step();
        inertia_gain.step();
        calculate_filter();
        last_calculated_generation = gen;
    }
    
    void activate()
    {
        params_changed();
        FilterClass::filter_activate();
        timer = once_per_n(FilterClass::srate / 1000);
        timer.start();
        is_active = true;
    }
    
    void set_sample_rate(uint32_t sr)
    {
        FilterClass::srate = sr;
    }

    
    void deactivate()
    {
        is_active = false;
    }

    uint32_t process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask) {
//        printf("sr=%d cutoff=%f res=%f mode=%f\n", FilterClass::srate, *params[Metadata::par_cutoff], *params[Metadata::par_resonance], *params[Metadata::par_mode]);
        uint32_t ostate = 0;
        numsamples += offset;
        while(offset < numsamples) {
            uint32_t numnow = numsamples - offset;
            // if inertia's inactive, we can calculate the whole buffer at once
            if (inertia_cutoff.active() || inertia_resonance.active() || inertia_gain.active())
                numnow = timer.get(numnow);
            
            if (outputs_mask & 1) {
                ostate |= FilterClass::process_channel(0, ins[0] + offset, outs[0] + offset, numnow, inputs_mask & 1);
            }
            if (outputs_mask & 2) {
                ostate |= FilterClass::process_channel(1, ins[1] + offset, outs[1] + offset, numnow, inputs_mask & 2);
            }
            
            if (timer.elapsed()) {
                on_timer();
            }
            offset += numnow;
        }
        return ostate;
    }
};

/// biquad filter module
class filter_audio_module: 
    public audio_module<filter_metadata>, 
    public filter_module_with_inertia<biquad_filter_module, filter_metadata>, 
    public frequency_response_line_graph
{
    float old_cutoff, old_resonance, old_mode;
public:    
    filter_audio_module()
    {
        last_generation = 0;
    }
    void params_changed()
    { 
        inertia_cutoff.set_inertia(*params[par_cutoff]);
        inertia_resonance.set_inertia(*params[par_resonance]);
        inertia_filter_module::params_changed(); 
    }
        
    void activate()
    {
        inertia_filter_module::activate();
    }
    
    void set_sample_rate(uint32_t sr)
    {
        inertia_filter_module::set_sample_rate(sr);
    }

    
    void deactivate()
    {
        inertia_filter_module::deactivate();
    }
    
    bool get_graph(int index, int subindex, float *data, int points, cairo_iface *context);
    int get_changed_offsets(int generation, int &subindex_graph, int &subindex_dot, int &subindex_gridline);
};

/// A multitap stereo chorus thing - processing
class multichorus_audio_module: public audio_module<multichorus_metadata>, public frequency_response_line_graph
{
public:
    float *ins[in_count]; 
    float *outs[out_count];
    float *params[param_count];
    uint32_t srate;
    dsp::multichorus<float, sine_multi_lfo<float, 8>, filter_sum<dsp::biquad_d2<>, dsp::biquad_d2<> >, 4096> left, right;
    float last_r_phase;
    float cutoff;
    bool is_active;
    
public:    
    multichorus_audio_module()
    {
        is_active = false;
        last_r_phase = -1;
    }
    
    void params_changed()
    {
        // delicious copy-pasta from flanger module - it'd be better to keep it common or something
        float dry = *params[par_dryamount];
        float wet = *params[par_amount];
        float rate = *params[par_rate];
        float min_delay = *params[par_delay] / 1000.0;
        float mod_depth = *params[par_depth] / 1000.0;
        float overlap = *params[par_overlap];
        left.set_dry(dry); right.set_dry(dry);
        left.set_wet(wet); right.set_wet(wet);
        left.set_rate(rate); right.set_rate(rate);
        left.set_min_delay(min_delay); right.set_min_delay(min_delay);
        left.set_mod_depth(mod_depth); right.set_mod_depth(mod_depth);
        int voices = (int)*params[par_voices];
        left.lfo.set_voices(voices); right.lfo.set_voices(voices);
        left.lfo.set_overlap(overlap);right.lfo.set_overlap(overlap);
        float vphase = *params[par_vphase] * (1.f / 360.f);
        left.lfo.vphase = right.lfo.vphase = vphase * (4096 / std::max(voices - 1, 1));
        float r_phase = *params[par_stereo] * (1.f / 360.f);
        if (fabs(r_phase - last_r_phase) > 0.0001f) {
            right.lfo.phase = left.lfo.phase;
            right.lfo.phase += chorus_phase(r_phase * 4096);
            last_r_phase = r_phase;
        }
        left.post.f1.set_bp_rbj(*params[par_freq], *params[par_q], srate);
        left.post.f2.set_bp_rbj(*params[par_freq2], *params[par_q], srate);
        right.post.f1.copy_coeffs(left.post.f1);
        right.post.f2.copy_coeffs(left.post.f2);
    }
    uint32_t process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask) {
        left.process(outs[0] + offset, ins[0] + offset, numsamples);
        right.process(outs[1] + offset, ins[1] + offset, numsamples);
        return outputs_mask; // XXXKF allow some delay after input going blank
    }
    void activate();
    void deactivate();
    void set_sample_rate(uint32_t sr);
    bool get_graph(int index, int subindex, float *data, int points, cairo_iface *context);
    float freq_gain(int subindex, float freq, float srate);
    bool get_dot(int index, int subindex, float &x, float &y, int &size, cairo_iface *context);
    bool get_gridline(int index, int subindex, float &pos, bool &vertical, std::string &legend, cairo_iface *context);
};

class compressor_audio_module: public audio_module<compressor_metadata>, public line_graph_iface {
private:
    float linSlope, peak, detected, kneeSqrt, kneeStart, linKneeStart, kneeStop, threshold, ratio, knee, makeup, compressedKneeStop, adjKneeStart;
    float old_threshold, old_ratio, old_knee, old_makeup, old_bypass;
    int last_generation;
    uint32_t clip;
    aweighter awL, awR;
    biquad_d2<float> bpL, bpR;
public:
    float *ins[in_count];
    float *outs[out_count];
    float *params[param_count];
    uint32_t srate;
    bool is_active;
    compressor_audio_module();
    void activate();
    void deactivate();
    uint32_t process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask);
    
    inline float output_level(float slope) {
        return slope * output_gain(slope, false) * makeup;
    }
    
    inline float output_gain(float linSlope, bool rms) {
         if(linSlope > (rms ? adjKneeStart : linKneeStart)) {
            float slope = log(linSlope);
            if(rms) slope *= 0.5f;

            float gain = 0.f;
            float delta = 0.f;
            if(IS_FAKE_INFINITY(ratio)) {
                gain = threshold;
                delta = 0.f;
            } else {
                gain = (slope - threshold) / ratio + threshold;
                delta = 1.f / ratio;
            }
            
            if(knee > 1.f && slope < kneeStop) {
                gain = hermite_interpolation(slope, kneeStart, kneeStop, kneeStart, compressedKneeStop, 1.f, delta);
            }
            
            return exp(gain - slope);
        }

        return 1.f;
    }

    void set_sample_rate(uint32_t sr);
    
    virtual bool get_graph(int index, int subindex, float *data, int points, cairo_iface *context);
    virtual bool get_dot(int index, int subindex, float &x, float &y, int &size, cairo_iface *context);
    virtual bool get_gridline(int index, int subindex, float &pos, bool &vertical, std::string &legend, cairo_iface *context);

    virtual int  get_changed_offsets(int generation, int &subindex_graph, int &subindex_dot, int &subindex_gridline)
    {
	subindex_graph = 0;
	subindex_dot = 0;
	subindex_gridline = generation ? INT_MAX : 0;

        if (fabs(threshold-old_threshold) + fabs(ratio - old_ratio) + fabs(knee - old_knee) + fabs( makeup - old_makeup) + fabs( *params[param_bypass] - old_bypass) > 0.01f)
        {
	    old_threshold = threshold;
	    old_ratio = ratio;
	    old_knee = knee;
	    old_makeup = makeup;
            old_bypass = *params[param_bypass];
            last_generation++;
        }

        if (generation == last_generation)
            subindex_graph = 2;
        return last_generation;
    }
};

/// Filterclavier --- MIDI controlled filter by Hans Baier
class filterclavier_audio_module: 
        public audio_module<filterclavier_metadata>, 
        public filter_module_with_inertia<biquad_filter_module, filterclavier_metadata>, 
        public frequency_response_line_graph
{        
    const float min_gain;
    const float max_gain;
    
    int last_note;
    int last_velocity;
        
public:    
    filterclavier_audio_module() 
        : 
            min_gain(1.0),
            max_gain(32.0),
            last_note(-1),
            last_velocity(-1) {}
    
    void params_changed()
    { 
        inertia_filter_module::inertia_cutoff.set_inertia(
            note_to_hz(last_note + *params[par_transpose], *params[par_detune]));
        
        float min_resonance = param_props[par_max_resonance].min;
         inertia_filter_module::inertia_resonance.set_inertia( 
                 (float(last_velocity) / 127.0)
                 // 0.001: see below
                 * (*params[par_max_resonance] - min_resonance + 0.001)
                 + min_resonance);
             
        adjust_gain_according_to_filter_mode(last_velocity);
        
        inertia_filter_module::calculate_filter(); 
    }
        
    void activate()
    {
        inertia_filter_module::activate();
    }
    
    void set_sample_rate(uint32_t sr)
    {
        inertia_filter_module::set_sample_rate(sr);
    }

    
    void deactivate()
    {
        inertia_filter_module::deactivate();
    }
  
    /// MIDI control
    virtual void note_on(int note, int vel)
    {
        last_note     = note;
        last_velocity = vel;
        inertia_filter_module::inertia_cutoff.set_inertia(
                note_to_hz(note + *params[par_transpose], *params[par_detune]));

        float min_resonance = param_props[par_max_resonance].min;
        inertia_filter_module::inertia_resonance.set_inertia( 
                (float(vel) / 127.0) 
                // 0.001: if the difference is equal to zero (which happens
                // when the max_resonance knom is at minimum position
                // then the filter gain doesnt seem to snap to zero on most note offs
                * (*params[par_max_resonance] - min_resonance + 0.001) 
                + min_resonance);
        
        adjust_gain_according_to_filter_mode(vel);
        
        inertia_filter_module::calculate_filter();
    }
    
    virtual void note_off(int note, int vel)
    {
        if (note == last_note) {
            inertia_filter_module::inertia_resonance.set_inertia(param_props[par_max_resonance].min);
            inertia_filter_module::inertia_gain.set_inertia(min_gain);
            inertia_filter_module::calculate_filter();
            last_velocity = 0;
        }
    }

    bool get_graph(int index, int subindex, float *data, int points, cairo_iface *context);
    
private:
    void adjust_gain_according_to_filter_mode(int velocity) {
        int   mode = dsp::fastf2i_drm(*params[par_mode]);
        
        // for bandpasses: boost gain for velocities > 0
        if ( (mode_6db_bp <= mode) && (mode <= mode_18db_bp) ) {
            // gain for velocity 0:   1.0
            // gain for velocity 127: 32.0
            float mode_max_gain = max_gain;
            // max_gain is right for mode_6db_bp
            if (mode == mode_12db_bp)
                mode_max_gain /= 6.0;
            if (mode == mode_18db_bp)
                mode_max_gain /= 10.5;
            
            inertia_filter_module::inertia_gain.set_now(
                    (float(velocity) / 127.0) * (mode_max_gain - min_gain) + min_gain);
        } else {
            inertia_filter_module::inertia_gain.set_now(min_gain);
        }
    }
};

extern std::string get_builtin_modules_rdf();

};

#include "modules_synths.h"

#endif
