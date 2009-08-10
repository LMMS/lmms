/* Calf DSP Library
 * Audio modules - synthesizers
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
#ifndef __CALF_MODULES_SYNTHS_H
#define __CALF_MODULES_SYNTHS_H

#include <assert.h>
#include "biquad.h"
#include "onepole.h"
#include "audio_fx.h"
#include "inertia.h"
#include "osc.h"
#include "synth.h"
#include "envelope.h"
#include "organ.h"
#include "modmatrix.h"

namespace calf_plugins {

#define MONOSYNTH_WAVE_BITS 12
    
/// Monosynth-in-making. Parameters may change at any point, so don't make songs with it!
/// It lacks inertia for parameters, even for those that really need it.
class monosynth_audio_module: public audio_module<monosynth_metadata>, public line_graph_iface, public mod_matrix
{
public:
    enum { mod_matrix_slots = 10 };
    float *ins[in_count]; 
    float *outs[out_count];
    float *params[param_count];
    uint32_t srate, crate;
    static dsp::waveform_family<MONOSYNTH_WAVE_BITS> *waves;
    dsp::waveform_oscillator<MONOSYNTH_WAVE_BITS> osc1, osc2;
    dsp::triangle_lfo lfo;
    bool running, stopping, gate, force_fadeout;
    int last_key;
    
    float buffer[step_size], buffer2[step_size];
    uint32_t output_pos;
    dsp::onepole<float> phaseshifter;
    dsp::biquad_d1_lerp<float> filter;
    dsp::biquad_d1_lerp<float> filter2;
    /// Waveform number - OSC1
    int wave1;
    /// Waveform number - OSC2
    int wave2;
    /// Last used waveform number - OSC1
    int prev_wave1;
    /// Last used waveform number - OSC2
    int prev_wave2;
    int filter_type, last_filter_type;
    float freq, start_freq, target_freq, cutoff, decay_factor, fgain, fgain_delta, separation;
    float detune, xpose, xfade, ampctl, fltctl, queue_vel;
    float odcr, porta_time, lfo_bend, lfo_clock, last_lfov, modwheel_value;
    /// Last value of phase shift for pulse width emulation for OSC1
    int32_t last_pwshift1;
    /// Last value of phase shift for pulse width emulation for OSC2
    int32_t last_pwshift2;
    int queue_note_on, stop_count, modwheel_value_int;
    int legato;
    dsp::adsr envelope;
    dsp::keystack stack;
    dsp::gain_smoothing master;
    /// Smoothed cutoff value
    dsp::inertia<dsp::exponential_ramp> inertia_cutoff;
    /// Smoothed pitch bend value
    dsp::inertia<dsp::exponential_ramp> inertia_pitchbend;
    /// Smoothed channel pressure value
    dsp::inertia<dsp::linear_ramp> inertia_pressure;
    /// Rows of the modulation matrix
    dsp::modulation_entry mod_matrix_data[mod_matrix_slots];
    /// Currently used velocity
    float velocity;
    /// Last value of oscillator mix ratio
    float last_xfade;
    /// Current calculated mod matrix outputs
    float moddest[moddest_count];
     
    monosynth_audio_module();    
    static void precalculate_waves(progress_report_iface *reporter);
    void set_sample_rate(uint32_t sr);
    void delayed_note_on();
    /// Handle MIDI Note On message (does not immediately trigger a note, as it must start on
    /// boundary of step_size samples).
    void note_on(int note, int vel);
    /// Handle MIDI Note Off message
    void note_off(int note, int vel);
    /// Handle MIDI Channel Pressure
    void channel_pressure(int value);
    /// Handle pitch bend message.
    inline void pitch_bend(int value)
    {
        inertia_pitchbend.set_inertia(pow(2.0, (value * *params[par_pwhlrange]) / (1200.0 * 8192.0)));
    }
    /// Update oscillator frequency based on base frequency, detune amount, pitch bend scaling factor and sample rate.
    inline void set_frequency()
    {
        float detune_scaled = (detune - 1); // * log(freq / 440);
        if (*params[par_scaledetune] > 0)
            detune_scaled *= pow(20.0 / freq, *params[par_scaledetune]);
        float p1 = 1, p2 = 1;
        if (moddest[moddest_o1detune] != 0)
            p1 = pow(2.0, moddest[moddest_o1detune] * (1.0 / 1200.0));
        if (moddest[moddest_o2detune] != 0)
            p2 = pow(2.0, moddest[moddest_o2detune] * (1.0 / 1200.0));
        osc1.set_freq(freq * (1 - detune_scaled) * p1 * inertia_pitchbend.get_last() * lfo_bend, srate);
        osc2.set_freq(freq * (1 + detune_scaled) * p2 * inertia_pitchbend.get_last() * lfo_bend * xpose, srate);
    }
    /// Handle control change messages.
    void control_change(int controller, int value);
    /// Update variables from control ports.
    void params_changed() {
        float sf = 0.001f;
        envelope.set(*params[par_attack] * sf, *params[par_decay] * sf, std::min(0.999f, *params[par_sustain]), *params[par_release] * sf, srate / step_size, *params[par_fade] * sf);
        filter_type = dsp::fastf2i_drm(*params[par_filtertype]);
        decay_factor = odcr * 1000.0 / *params[par_decay];
        separation = pow(2.0, *params[par_cutoffsep] / 1200.0);
        wave1 = dsp::clip(dsp::fastf2i_drm(*params[par_wave1]), 0, (int)wave_count - 1);
        wave2 = dsp::clip(dsp::fastf2i_drm(*params[par_wave2]), 0, (int)wave_count - 1);
        detune = pow(2.0, *params[par_detune] / 1200.0);
        xpose = pow(2.0, *params[par_osc2xpose] / 12.0);
        xfade = *params[par_oscmix];
        legato = dsp::fastf2i_drm(*params[par_legato]);
        master.set_inertia(*params[par_master]);
        set_frequency();
        if (wave1 != prev_wave1 || wave2 != prev_wave2)
            lookup_waveforms();
    }
    void activate();
    void deactivate();
    void post_instantiate()
    {
        precalculate_waves(progress_report);
    }
    /// Set waveform addresses for oscillators
    void lookup_waveforms();
    /// Run oscillators
    void calculate_buffer_oscs(float lfo);
    /// Run two filters in series to produce mono output samples.
    void calculate_buffer_ser();
    /// Run one filter to produce mono output samples.
    void calculate_buffer_single();
    /// Run two filters (one per channel) to produce stereo output samples.
    void calculate_buffer_stereo();
    /// Retrieve filter graph (which is 'live' so it cannot be generated by get_static_graph), or fall back to get_static_graph.
    bool get_graph(int index, int subindex, float *data, int points, cairo_iface *context);
    /// @retval true if the filter 1 is to be used for the left channel and filter 2 for the right channel
    /// @retval false if filters are to be connected in series and sent (mono) to both channels    
    inline bool is_stereo_filter() const
    {
        return filter_type == flt_2lp12 || filter_type == flt_2bp6;
    }
    /// No CV inputs for now
    bool is_cv(int param_no) { return false; }
    /// Practically all the stuff here is noisy
    bool is_noisy(int param_no) { return param_no != par_cutoff; }
    /// Calculate control signals and produce step_size samples of output.
    void calculate_step();
    /// Main processing function
    uint32_t process(uint32_t offset, uint32_t nsamples, uint32_t inputs_mask, uint32_t outputs_mask);
};

struct organ_audio_module: public audio_module<organ_metadata>, public dsp::drawbar_organ, public line_graph_iface
{
public:
    using drawbar_organ::note_on;
    using drawbar_organ::note_off;
    using drawbar_organ::control_change;
    enum { param_count = drawbar_organ::param_count};
    float *ins[in_count]; 
    float *outs[out_count];
    float *params[param_count];
    dsp::organ_parameters par_values;
    uint32_t srate;
    bool panic_flag;
    /// Value for configure variable map_curve
    std::string var_map_curve;

    organ_audio_module()
    : drawbar_organ(&par_values)
    {
        var_map_curve = "2\n0 1\n1 1\n"; // XXXKF hacky bugfix
    }
    
    void post_instantiate()
    {
        dsp::organ_voice_base::precalculate_waves(progress_report);
    }

    void set_sample_rate(uint32_t sr) {
        srate = sr;
    }
    void params_changed() {
        for (int i = 0; i < param_count - var_count; i++)
            ((float *)&par_values)[i] = *params[i];

        unsigned int old_poly = polyphony_limit;
        polyphony_limit = dsp::clip(dsp::fastf2i_drm(*params[par_polyphony]), 1, 32);
        if (polyphony_limit < old_poly)
            trim_voices();
        
        update_params();
    }
    inline void pitch_bend(int amt)
    {
        drawbar_organ::pitch_bend(amt);
    }
    void activate() {
        setup(srate);
        panic_flag = false;
    }
    void deactivate();
    uint32_t process(uint32_t offset, uint32_t nsamples, uint32_t inputs_mask, uint32_t outputs_mask) {
        float *o[2] = { outs[0] + offset, outs[1] + offset };
        if (panic_flag)
        {
            control_change(120, 0); // stop all sounds
            control_change(121, 0); // reset all controllers
            panic_flag = false;
        }
        render_separate(o, nsamples);
        return 3;
    }
    /// No CV inputs for now
    bool is_cv(int param_no) { return false; }
    /// Practically all the stuff here is noisy
    bool is_noisy(int param_no) { return true; }
    void execute(int cmd_no);
    bool get_graph(int index, int subindex, float *data, int points, cairo_iface *context);
    
    char *configure(const char *key, const char *value);
    void send_configures(send_configure_iface *);
    uint32_t message_run(const void *valid_inputs, void *output_ports) { 
        // silence a default printf (which is kind of a warning about unhandled message_run)
        return 0;
    }
};

};

#if ENABLE_EXPERIMENTAL
    
#include "wavetable.h"

#endif

#endif
