/* Calf DSP Library
 * Drawbar organ emulator. 
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
 * Boston, MA 02111-1307, USA.
 */

#ifndef __CALF_ORGAN_H
#define __CALF_ORGAN_H

#include "synth.h"
#include "envelope.h"
#include "metadata.h"

#define ORGAN_KEYTRACK_POINTS 4

namespace dsp
{

struct organ_parameters {
    enum { FilterCount = 2, EnvCount = 3 };
    struct organ_filter_parameters
    {
        float cutoff;
        float resonance;
        float envmod[organ_parameters::EnvCount];
        float keyf;
    };

    struct organ_env_parameters
    {
        float attack, decay, sustain, release, velscale, ampctl;
    };
        
    //////////////////////////////////////////////////////////////////////////
    // these parameters are binary-copied from control ports (order is important!)
    
    float drawbars[9];
    float harmonics[9];
    float waveforms[9];
    float detune[9];
    float phase[9];
    float pan[9];
    float routing[9];
    float foldover;
    float percussion_time;
    float percussion_level;
    float percussion_wave;
    float percussion_harmonic;
    float percussion_vel2amp;
    float percussion_fm_time;
    float percussion_fm_depth;
    float percussion_fm_wave;
    float percussion_fm_harmonic;
    float percussion_vel2fm;
    float percussion_trigger;
    float percussion_stereo;
    float filter_chain;
    float filter1_type;
    float master;

    organ_filter_parameters filters[organ_parameters::FilterCount];
    organ_env_parameters envs[organ_parameters::EnvCount];
    float lfo_rate;
    float lfo_amt;
    float lfo_wet;
    float lfo_phase;
    float lfo_mode;
    
    float global_transpose;
    float global_detune;
    
    float polyphony;
    
    float quad_env;
    
    float pitch_bend_range;
    
    float bass_freq;
    float bass_gain;
    float treble_freq;
    float treble_gain;
    
    float dummy_mapcurve;
    
    //////////////////////////////////////////////////////////////////////////
    // these parameters are calculated
    
    double perc_decay_const, perc_fm_decay_const;
    float multiplier[9];
    int phaseshift[9];
    float cutoff;
    unsigned int foldvalue;
    float pitch_bend;

    float percussion_keytrack[ORGAN_KEYTRACK_POINTS][2];
    
    organ_parameters() : pitch_bend(1.0f) {}

    inline int get_percussion_wave() { return dsp::fastf2i_drm(percussion_wave); }
    inline int get_percussion_fm_wave() { return dsp::fastf2i_drm(percussion_fm_wave); }
};

#define ORGAN_WAVE_BITS 12
#define ORGAN_WAVE_SIZE 4096
#define ORGAN_BIG_WAVE_BITS 17
#define ORGAN_BIG_WAVE_SIZE 131072
/// 2^ORGAN_BIG_WAVE_SHIFT = how many (quasi)periods per sample
#define ORGAN_BIG_WAVE_SHIFT 5

class organ_voice_base: public calf_plugins::organ_enums
{
public:
    typedef waveform_family<ORGAN_WAVE_BITS> small_wave_family;
    typedef waveform_family<ORGAN_BIG_WAVE_BITS> big_wave_family;
public:
    organ_parameters *parameters;
protected:
    static small_wave_family (*waves)[wave_count_small];
    static big_wave_family (*big_waves)[wave_count_big];

    // dsp::sine_table<float, ORGAN_WAVE_SIZE, 1> sine_wave;
    int note;
    dsp::decay amp;
    /// percussion FM carrier amplitude envelope
    dsp::decay pamp;
    /// percussion FM modulator amplitude envelope
    dsp::decay fm_amp;
    dsp::fixed_point<int64_t, 20> pphase, dpphase;
    dsp::fixed_point<int64_t, 20> modphase, moddphase;
    float fm_keytrack;
    int &sample_rate_ref;
    bool &released_ref;
    /// pamp per-sample (linear) step during release stage (calculated on release so that it will take 30ms for it to go from "current value at release point" to 0)
    float rel_age_const;

    organ_voice_base(organ_parameters *_parameters, int &_sample_rate_ref, bool &_released_ref);
    
    inline float wave(float *data, dsp::fixed_point<int, 20> ph) {
        return ph.lerp_table_lookup_float(data);
    }
    inline float big_wave(float *data, dsp::fixed_point<int64_t, 20> &ph) {
        // wrap to fit within the wave
        return ph.lerp_table_lookup_float_mask(data, ORGAN_BIG_WAVE_SIZE - 1);
    }
public:
    static inline small_wave_family &get_wave(int wave) {
        return (*waves)[wave];
    }
    static inline big_wave_family &get_big_wave(int wave) {
        return (*big_waves)[wave];
    }
    static void precalculate_waves(calf_plugins::progress_report_iface *reporter);
    void update_pitch()
    {
        float phase = dsp::midi_note_to_phase(note, 100 * parameters->global_transpose + parameters->global_detune, sample_rate_ref);
        dpphase.set((long int) (phase * parameters->percussion_harmonic * parameters->pitch_bend));
        moddphase.set((long int) (phase * parameters->percussion_fm_harmonic * parameters->pitch_bend));
    }
    // this doesn't really have a voice interface
    void render_percussion_to(float (*buf)[2], int nsamples);
    void perc_note_on(int note, int vel);
    void perc_note_off(int note, int vel);
    void perc_reset()
    {
        pphase = 0;
        modphase = 0;
        dpphase = 0;
        moddphase = 0;
        note = -1;
    }
};

class organ_vibrato
{
protected:
    enum { VibratoSize = 6 };
    float vibrato_x1[VibratoSize][2], vibrato_y1[VibratoSize][2];
    float lfo_phase;
    dsp::onepole<float> vibrato[2];
public:
    void reset();
    void process(organ_parameters *parameters, float (*data)[2], unsigned int len, float sample_rate);
};

class organ_voice: public dsp::voice, public organ_voice_base {
protected:    
    enum { Channels = 2, BlockSize = 64, EnvCount = organ_parameters::EnvCount, FilterCount = organ_parameters::FilterCount };
    union {
        float output_buffer[BlockSize][Channels];
        float aux_buffers[3][BlockSize][Channels];
    };
    dsp::fixed_point<int64_t, 52> phase, dphase;
    dsp::biquad_d1<float> filterL[2], filterR[2];
    adsr envs[EnvCount];
    dsp::inertia<dsp::linear_ramp> expression;
    organ_vibrato vibrato;
    float velocity;
    bool perc_released;
    /// The envelopes have ended and the voice is in final fadeout stage
    bool finishing;
    dsp::inertia<dsp::exponential_ramp> inertia_pitchbend;

public:
    organ_voice()
    : organ_voice_base(NULL, sample_rate, perc_released)
    , expression(dsp::linear_ramp(16))
    , inertia_pitchbend(dsp::exponential_ramp(1))
    {
        inertia_pitchbend.set_now(1);
    }

    void reset() {
        inertia_pitchbend.ramp.set_length(sample_rate / (BlockSize * 30)); // 1/30s    
        vibrato.reset();
        phase = 0;
        for (int i = 0; i < FilterCount; i++)
        {
            filterL[i].reset();
            filterR[i].reset();
        }
    }

    void note_on(int note, int vel) {
        stolen = false;
        finishing = false;
        perc_released = false;
        released = false;
        reset();
        this->note = note;
        const float sf = 0.001f;
        for (int i = 0; i < EnvCount; i++)
        {
            organ_parameters::organ_env_parameters &p = parameters->envs[i];
            envs[i].set(sf * p.attack, sf * p.decay, p.sustain, sf * p.release, sample_rate / BlockSize);
            envs[i].note_on();
        }
        update_pitch();
        velocity = vel * 1.0 / 127.0;
        amp.set(1.0f);
        perc_note_on(note, vel);
    }

    void note_off(int /* vel */) {
        // reset age to 0 (because decay will turn from exponential to linear, necessary because of error cumulation prevention)
        perc_released = true;
        if (pamp.get_active())
        {
            pamp.reinit();
        }
        rel_age_const = pamp.get() * ((1.0/44100.0)/0.03);
        for (int i = 0; i < EnvCount; i++)
            envs[i].note_off();
    }

    virtual float get_priority() { return stolen ? 20000 : (perc_released ? 1 : (sostenuto ? 200 : 100)); }
    
    virtual void steal() {
        perc_released = true;
        finishing = true;
        stolen = true;
    }

    void render_block();
    
    virtual int get_current_note() {
        return note;
    }
    virtual bool get_active() {
        // printf("note %d getactive %d use_percussion %d pamp active %d\n", note, amp.get_active(), use_percussion(), pamp.get_active());
        return (note != -1) && (amp.get_active() || (use_percussion() && pamp.get_active()));
    }
    void update_pitch();
    inline bool use_percussion()
    {
        return dsp::fastf2i_drm(parameters->percussion_trigger) == perctrig_polyphonic && parameters->percussion_level > 0;
    }
};

/// Not a true voice, just something with similar-ish interface.
class percussion_voice: public organ_voice_base {
public:
    int sample_rate;
    bool released;

    percussion_voice(organ_parameters *_parameters)
    : organ_voice_base(_parameters, sample_rate, released)
    , released(false)
    {
    }
    
    bool get_active() {
        return (note != -1) && pamp.get_active();
    }
    bool get_noticable() {
        return (note != -1) && (pamp.get() > 0.2 * parameters->percussion_level);
    }
    void setup(int sr) {
        sample_rate = sr;
    }
};

struct drawbar_organ: public dsp::basic_synth, public calf_plugins::organ_enums {
    organ_parameters *parameters;
    percussion_voice percussion;
    organ_vibrato global_vibrato;
    two_band_eq eq_l, eq_r;
    
     drawbar_organ(organ_parameters *_parameters)
    : parameters(_parameters)
    , percussion(_parameters) {
    }
    void render_separate(float *output[], int nsamples);
    dsp::voice *alloc_voice() {
        block_voice<organ_voice> *v = new block_voice<organ_voice>();
        v->parameters = parameters;
        return v;
    }
    virtual void percussion_note_on(int note, int vel) {
        percussion.perc_note_on(note, vel);
    }
    virtual void params_changed() = 0;
    virtual void setup(int sr) {
        basic_synth::setup(sr);
        percussion.setup(sr);
        parameters->cutoff = 0;
        params_changed();
        global_vibrato.reset();
    }
    void update_params();
    void control_change(int controller, int value)
    {
#if 0
        if (controller == 11)
        {
            parameters->cutoff = value / 64.0 - 1;
        }
#endif
        dsp::basic_synth::control_change(controller, value);
    }
    void pitch_bend(int amt);
    virtual bool check_percussion() { 
        switch(dsp::fastf2i_drm(parameters->percussion_trigger))
        {        
            case organ_voice_base::perctrig_first:
                return active_voices.empty();
            case organ_voice_base::perctrig_each: 
            default:
                return true;
            case organ_voice_base::perctrig_eachplus:
                return !percussion.get_noticable();
            case organ_voice_base::perctrig_polyphonic:
                return false;
        }
    }
};

};

#endif
