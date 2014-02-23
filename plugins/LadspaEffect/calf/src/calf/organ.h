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

#include "audio_fx.h"
#include "envelope.h"
#include "metadata.h"
#include "osc.h"
#include "synth.h"

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
    float lfo_type;
    
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
    void update_pitch();
    // this doesn't really have a voice interface
    void render_percussion_to(float (*buf)[2], int nsamples);
    void perc_note_on(int note, int vel);
    void perc_note_off(int note, int vel);
    void perc_reset();
};

/// A simple (and bad) simulation of scanner vibrato based on a series of modulated allpass filters
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

/// A more sophisticated simulation of scanner vibrato. Simulates a line box
/// and an interpolating scanner. The line box is a series of 18 2nd order
/// lowpass filters with cutoff frequency ~4kHz, with loss compensation.
/// The interpolating scanner uses linear interpolation to "slide" between
/// selected outputs of the line box.
///
/// @note
/// This is a true CPU hog, and it should be optimised some day.
/// @note
/// The line box is mono. 36 lowpass filters might be an overkill.
/// @note 
/// See also: http://www.jhaible.de/interpolating_scanner_and_scanvib/jh_interpolating_scanner_and_scanvib.html
/// (though it's a very loose adaptation of that version)
class scanner_vibrato
{
protected:
    enum { ScannerSize = 18 };
    float lfo_phase;
    dsp::biquad_d2<float> scanner[ScannerSize];
    organ_vibrato legacy;
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
    scanner_vibrato vibrato;
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

    void reset();
    void note_on(int note, int vel);
    void note_off(int /* vel */);
    virtual float get_priority() { return stolen ? 20000 : (perc_released ? 1 : (sostenuto ? 200 : 100)); }
    virtual void steal();
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
    scanner_vibrato global_vibrato;
    two_band_eq eq_l, eq_r;
    
     drawbar_organ(organ_parameters *_parameters)
    : parameters(_parameters)
    , percussion(_parameters) {
    }
    void render_separate(float *output[], int nsamples);
    dsp::voice *alloc_voice();
    virtual void percussion_note_on(int note, int vel);
    virtual void params_changed() = 0;
    virtual void setup(int sr);
    void update_params();
    void control_change(int controller, int value)
    {
        dsp::basic_synth::control_change(controller, value);
    }
    void pitch_bend(int amt);
    virtual bool check_percussion();
};

};

namespace calf_plugins {

struct organ_audio_module: public audio_module<organ_metadata>, public dsp::drawbar_organ, public line_graph_iface
{
public:
    using drawbar_organ::note_on;
    using drawbar_organ::note_off;
    using drawbar_organ::control_change;
    enum { param_count = drawbar_organ::param_count};
    dsp::organ_parameters par_values;
    uint32_t srate;
    bool panic_flag;
    /// Value for configure variable map_curve
    std::string var_map_curve;

    organ_audio_module();
    
    void post_instantiate();

    void set_sample_rate(uint32_t sr) {
        srate = sr;
    }
    void params_changed();

    void activate();
    void deactivate();
    uint32_t process(uint32_t offset, uint32_t nsamples, uint32_t inputs_mask, uint32_t outputs_mask);
    /// No CV inputs for now
    bool is_cv(int param_no) { return false; }
    /// Practically all the stuff here is noisy
    bool is_noisy(int param_no) { return true; }
    void execute(int cmd_no);
    bool get_graph(int index, int subindex, float *data, int points, cairo_iface *context) const;
    char *configure(const char *key, const char *value);
    void send_configures(send_configure_iface *);
    uint32_t message_run(const void *valid_inputs, void *output_ports);
public:
    // overrides
    virtual void note_on(int /*channel*/, int note, int velocity) { dsp::drawbar_organ::note_on(note, velocity); }
    virtual void note_off(int /*channel*/, int note, int velocity) { dsp::drawbar_organ::note_off(note, velocity); }
    virtual void control_change(int /*channel*/, int controller, int value) { dsp::drawbar_organ::control_change(controller, value); }
    virtual void pitch_bend(int /*channel*/, int value) { dsp::drawbar_organ::pitch_bend(value); }
};

};

#endif
