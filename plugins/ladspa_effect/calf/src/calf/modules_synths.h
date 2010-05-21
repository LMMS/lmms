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

#include "biquad.h"
#include "onepole.h"
#include "audio_fx.h"
#include "inertia.h"
#include "osc.h"
#include "synth.h"
#include "envelope.h"
#include "modmatrix.h"
#include "metadata.h"

namespace calf_plugins {

#define MONOSYNTH_WAVE_BITS 12
    
/// Monosynth-in-making. Parameters may change at any point, so don't make songs with it!
/// It lacks inertia for parameters, even for those that really need it.
class monosynth_audio_module: public audio_module<monosynth_metadata>, public line_graph_iface, public mod_matrix
{
public:
    enum { mod_matrix_slots = 10 };
    uint32_t srate, crate;
    static dsp::waveform_family<MONOSYNTH_WAVE_BITS> *waves;
    dsp::waveform_oscillator<MONOSYNTH_WAVE_BITS> osc1, osc2;
    dsp::triangle_lfo lfo1, lfo2;
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
    float freq, start_freq, target_freq, cutoff, fgain, fgain_delta, separation;
    float detune, xpose, xfade, ampctl, fltctl, queue_vel;
    float odcr, porta_time, lfo_bend, lfo1_clock, lfo2_clock, modwheel_value;
    /// Delay counter for LFOs
    float lfo_clock;
    /// Last value of phase shift for pulse width emulation for OSC1
    int32_t last_pwshift1;
    /// Last value of phase shift for pulse width emulation for OSC2
    int32_t last_pwshift2;
    /// Last value of stretch for osc sync emulation for OSC1
    int32_t last_stretch1;
    int queue_note_on, stop_count, modwheel_value_int;
    int legato;
    dsp::adsr envelope1, envelope2;
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
    void set_frequency();
    /// Handle control change messages.
    void control_change(int controller, int value);
    /// Update variables from control ports.
    void params_changed();
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
    bool get_graph(int index, int subindex, float *data, int points, cairo_iface *context) const;
    /// @retval true if the filter 1 is to be used for the left channel and filter 2 for the right channel
    /// @retval false if filters are to be connected in series and sent (mono) to both channels    
    inline bool is_stereo_filter() const
    {
        return filter_type == flt_2lp12 || filter_type == flt_2bp6;
    }
    /// No CV inputs for now
    bool is_cv(int param_no) const { return false; }
    /// Practically all the stuff here is noisy
    bool is_noisy(int param_no) const { return param_no != par_cutoff; }
    /// Calculate control signals and produce step_size samples of output.
    void calculate_step();
    /// Main processing function
    uint32_t process(uint32_t offset, uint32_t nsamples, uint32_t inputs_mask, uint32_t outputs_mask);
};

};

#if ENABLE_EXPERIMENTAL
    
#include "wavetable.h"

#endif

#endif
