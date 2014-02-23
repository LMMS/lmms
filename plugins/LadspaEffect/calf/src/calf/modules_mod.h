/* Calf DSP plugin pack
 * Modulation effect plugins
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
 * Boston, MA 02111-1307, USA.
 */
#ifndef CALF_MODULES_MOD_H
#define CALF_MODULES_MOD_H

#include <assert.h>
#include <limits.h>
#include "biquad.h"
#include "inertia.h"
#include "audio_fx.h"
#include "giface.h"
#include "metadata.h"
#include "multichorus.h"

namespace calf_plugins {

class flanger_audio_module: public audio_module<flanger_metadata>, public frequency_response_line_graph
{
public:
    dsp::simple_flanger<float, 2048> left, right;
    uint32_t srate;
    bool clear_reset;
    float last_r_phase;
    bool is_active;
public:
    flanger_audio_module() {
        is_active = false;
    }
    void set_sample_rate(uint32_t sr);
    void params_changed();
    void params_reset();
    void activate();
    void deactivate();
    uint32_t process(uint32_t offset, uint32_t nsamples, uint32_t inputs_mask, uint32_t outputs_mask) {
        left.process(outs[0] + offset, ins[0] + offset, nsamples);
        right.process(outs[1] + offset, ins[1] + offset, nsamples);
        return outputs_mask; // XXXKF allow some delay after input going blank
    }
    bool get_graph(int index, int subindex, float *data, int points, cairo_iface *context) const;
    float freq_gain(int subindex, float freq, float srate) const;
};

class phaser_audio_module: public audio_module<phaser_metadata>, public frequency_response_line_graph
{
public:
    enum { MaxStages = 12 };
    uint32_t srate;
    bool clear_reset;
    float last_r_phase;
    dsp::simple_phaser left, right;
    float x1vals[2][MaxStages], y1vals[2][MaxStages];
    bool is_active;
public:
    phaser_audio_module();
    void params_changed();
    void params_reset();
    void activate();
    void set_sample_rate(uint32_t sr);
    void deactivate();
    uint32_t process(uint32_t offset, uint32_t nsamples, uint32_t inputs_mask, uint32_t outputs_mask) {
        left.process(outs[0] + offset, ins[0] + offset, nsamples);
        right.process(outs[1] + offset, ins[1] + offset, nsamples);
        return outputs_mask; // XXXKF allow some delay after input going blank
    }
    bool get_graph(int index, int subindex, float *data, int points, cairo_iface *context) const;
    bool get_gridline(int index, int subindex, float &pos, bool &vertical, std::string &legend, cairo_iface *context) const;
    float freq_gain(int subindex, float freq, float srate) const;
};

class rotary_speaker_audio_module: public audio_module<rotary_speaker_metadata>
{
public:
    /// Current phases and phase deltas for bass and treble rotors
    uint32_t phase_l, dphase_l, phase_h, dphase_h;
    dsp::simple_delay<1024, float> delay;
    dsp::biquad_d2<float> crossover1l, crossover1r, crossover2l, crossover2r, damper1l, damper1r;
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
    
    int meter_l, meter_h;
    
    rotary_speaker_audio_module();
    void set_sample_rate(uint32_t sr);
    void setup();
    void activate();
    void deactivate();
    
    void params_changed();
    void set_vibrato();
    /// Convert RPM speed to delta-phase
    uint32_t rpm2dphase(float rpm);
    /// Set delta-phase variables based on current calculated (and interpolated) RPM speed
    void update_speed();
    void update_speed_manual(float delta);
    /// Increase or decrease aspeed towards raspeed, with required negative and positive rate
    bool incr_towards(float &aspeed, float raspeed, float delta_decc, float delta_acc);
    uint32_t process(uint32_t offset, uint32_t nsamples, uint32_t inputs_mask, uint32_t outputs_mask);
    virtual void control_change(int channel, int ctl, int val);
};

/// A multitap stereo chorus thing
class multichorus_audio_module: public audio_module<multichorus_metadata>, public frequency_response_line_graph
{
public:
    uint32_t srate;
    dsp::multichorus<float, dsp::sine_multi_lfo<float, 8>, dsp::filter_sum<dsp::biquad_d2<>, dsp::biquad_d2<> >, 4096> left, right;
    float last_r_phase;
    float cutoff;
    bool is_active;
    
public:    
    multichorus_audio_module();
    void params_changed();
    uint32_t process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask);
    void activate();
    void deactivate();
    void set_sample_rate(uint32_t sr);
    bool get_graph(int index, int subindex, float *data, int points, cairo_iface *context) const;
    float freq_gain(int subindex, float freq, float srate) const;
    bool get_dot(int index, int subindex, float &x, float &y, int &size, cairo_iface *context) const;
    bool get_gridline(int index, int subindex, float &pos, bool &vertical, std::string &legend, cairo_iface *context) const;
};

/// Pulsator by Markus Schmidt
class pulsator_audio_module: public audio_module<pulsator_metadata>, public frequency_response_line_graph  {
private:
    typedef pulsator_audio_module AM;
    uint32_t clip_inL, clip_inR, clip_outL, clip_outR;
    float meter_inL, meter_inR, meter_outL, meter_outR;
    float offset_old;
    int mode_old;
    bool clear_reset;
    dsp::simple_lfo lfoL, lfoR;
public:
    uint32_t srate;
    bool is_active;
    pulsator_audio_module();
    void activate();
    void deactivate();
    void params_changed();
    void set_sample_rate(uint32_t sr);
    void params_reset()
    {
        if (clear_reset) {
            *params[param_reset] = 0.f;
            clear_reset = false;
        }
    }
    uint32_t process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask);
    bool get_graph(int index, int subindex, float *data, int points, cairo_iface *context) const;
    bool get_dot(int index, int subindex, float &x, float &y, int &size, cairo_iface *context) const;
    bool get_gridline(int index, int subindex, float &pos, bool &vertical, std::string &legend, cairo_iface *context) const;
};

};

#endif
