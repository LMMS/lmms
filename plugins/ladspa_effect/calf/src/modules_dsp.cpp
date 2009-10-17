/* Calf DSP Library
 * Example audio modules - DSP code
 *
 * Copyright (C) 2001-2008 Krzysztof Foltman
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
#include <assert.h>
#include <limits.h>
#include <memory.h>
#if USE_JACK
#include <jack/jack.h>
#endif
#include <calf/giface.h>
#include <calf/modules.h>
#include <calf/modules_dev.h>

using namespace dsp;
using namespace calf_plugins;

/// convert amplitude value to normalized grid-ish value (0dB = 0.5, 30dB = 1.0, -30 dB = 0.0, -60dB = -0.5, -90dB = -1.0)
static inline float dB_grid(float amp)
{
    return log(amp) * (1.0 / log(256.0)) + 0.4;
}

template<class Fx>
static bool get_graph(Fx &fx, int subindex, float *data, int points)
{
    for (int i = 0; i < points; i++)
    {
        typedef std::complex<double> cfloat;
        double freq = 20.0 * pow (20000.0 / 20.0, i * 1.0 / points);
        data[i] = dB_grid(fx.freq_gain(subindex, freq, fx.srate));
    }
    return true;
}

/// convert normalized grid-ish value back to amplitude value
static inline float dB_grid_inv(float pos)
{
    return pow(256.0, pos - 0.4);
}

static void set_channel_color(cairo_iface *context, int channel)
{
    if (channel & 1)
        context->set_source_rgba(0.35, 0.4, 0.2, 1);
    else
        context->set_source_rgba(0.35, 0.4, 0.2, 0.5);
    context->set_line_width(1.5);
}

static bool get_freq_gridline(int subindex, float &pos, bool &vertical, std::string &legend, cairo_iface *context, bool use_frequencies = true)
{
    if (subindex < 0 )
	return false;
    if (use_frequencies)
    {
        if (subindex < 28)
        {
            vertical = true;
            if (subindex == 9) legend = "100 Hz";
            if (subindex == 18) legend = "1 kHz";
            if (subindex == 27) legend = "10 kHz";
            float freq = 100;
            if (subindex < 9)
                freq = 10 * (subindex + 1);
            else if (subindex < 18)
                freq = 100 * (subindex - 9 + 1);
            else if (subindex < 27)
                freq = 1000 * (subindex - 18 + 1);
            else
                freq = 10000 * (subindex - 27 + 1);
            pos = log(freq / 20.0) / log(1000);
            if (!legend.empty())
                context->set_source_rgba(0, 0, 0, 0.2);
            else
                context->set_source_rgba(0, 0, 0, 0.1);
            return true;
        }
        subindex -= 28;
    }
    if (subindex >= 32)
        return false;
    float gain = 16.0 / (1 << subindex);
    pos = dB_grid(gain);
    if (pos < -1)
        return false;
    if (subindex != 4)
        context->set_source_rgba(0, 0, 0, subindex & 1 ? 0.1 : 0.2);
    if (!(subindex & 1))
    {
        std::stringstream ss;
        ss << (24 - 6 * subindex) << " dB";
        legend = ss.str();
    }
    vertical = false;
    return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool frequency_response_line_graph::get_gridline(int index, int subindex, float &pos, bool &vertical, std::string &legend, cairo_iface *context)
{ 
    return get_freq_gridline(subindex, pos, vertical, legend, context);
}

int frequency_response_line_graph::get_changed_offsets(int index, int generation, int &subindex_graph, int &subindex_dot, int &subindex_gridline)
{
    subindex_graph = 0;
    subindex_dot = 0;
    subindex_gridline = generation ? INT_MAX : 0;
    return 1;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void flanger_audio_module::activate() {
    left.reset();
    right.reset();
    last_r_phase = *params[par_stereo] * (1.f / 360.f);
    left.reset_phase(0.f);
    right.reset_phase(last_r_phase);
    is_active = true;
}

void flanger_audio_module::set_sample_rate(uint32_t sr) {
    srate = sr;
    left.setup(sr);
    right.setup(sr);
}

void flanger_audio_module::deactivate() {
    is_active = false;
}

bool flanger_audio_module::get_graph(int index, int subindex, float *data, int points, cairo_iface *context)
{
    if (!is_active)
        return false;
    if (index == par_delay && subindex < 2) 
    {
        set_channel_color(context, subindex);
        return ::get_graph(*this, subindex, data, points);
    }
    return false;
}

float flanger_audio_module::freq_gain(int subindex, float freq, float srate)
{
    return (subindex ? right : left).freq_gain(freq, srate);                
}

///////////////////////////////////////////////////////////////////////////////////////////////

void phaser_audio_module::set_sample_rate(uint32_t sr)
{
    srate = sr;
    left.setup(sr);
    right.setup(sr);
}

void phaser_audio_module::activate()
{
    is_active = true;
    left.reset();
    right.reset();
    last_r_phase = *params[par_stereo] * (1.f / 360.f);
    left.reset_phase(0.f);
    right.reset_phase(last_r_phase);
}

void phaser_audio_module::deactivate()
{
    is_active = false;
}

bool phaser_audio_module::get_graph(int index, int subindex, float *data, int points, cairo_iface *context)
{
    if (!is_active)
        return false;
    if (subindex < 2) 
    {
        set_channel_color(context, subindex);
        return ::get_graph(*this, subindex, data, points);
    }
    return false;
}

float phaser_audio_module::freq_gain(int subindex, float freq, float srate)
{
    return (subindex ? right : left).freq_gain(freq, srate);                
}

bool phaser_audio_module::get_gridline(int index, int subindex, float &pos, bool &vertical, std::string &legend, cairo_iface *context)
{
    return get_freq_gridline(subindex, pos, vertical, legend, context);
}

///////////////////////////////////////////////////////////////////////////////////////////////

void reverb_audio_module::activate()
{
    reverb.reset();
}

void reverb_audio_module::deactivate()
{
}

void reverb_audio_module::set_sample_rate(uint32_t sr)
{
    srate = sr;
    reverb.setup(sr);
    amount.set_sample_rate(sr);
}

///////////////////////////////////////////////////////////////////////////////////////////////

bool filter_audio_module::get_graph(int index, int subindex, float *data, int points, cairo_iface *context)
{
    if (!is_active)
        return false;
    if (index == par_cutoff && !subindex) {
        context->set_line_width(1.5);
        return ::get_graph(*this, subindex, data, points);
    }
    return false;
}

int filter_audio_module::get_changed_offsets(int index, int generation, int &subindex_graph, int &subindex_dot, int &subindex_gridline)
{
    if (fabs(inertia_cutoff.get_last() - old_cutoff) + 100 * fabs(inertia_resonance.get_last() - old_resonance) + fabs(*params[par_mode] - old_mode) > 0.1f)
    {
        old_cutoff = inertia_cutoff.get_last();
        old_resonance = inertia_resonance.get_last();
        old_mode = *params[par_mode];
        last_generation++;
        subindex_graph = 0;
        subindex_dot = INT_MAX;
        subindex_gridline = INT_MAX;
    }
    else {
        subindex_graph = 0;
        subindex_dot = subindex_gridline = generation ? INT_MAX : 0;
    }
    if (generation == last_calculated_generation)
        subindex_graph = INT_MAX;
    return last_generation;
}


///////////////////////////////////////////////////////////////////////////////////////////////

bool filterclavier_audio_module::get_graph(int index, int subindex, float *data, int points, cairo_iface *context)
{
    if (!is_active || index != par_mode) {
        return false;
    }
    if (!subindex) {
        context->set_line_width(1.5);
        return ::get_graph(*this, subindex, data, points);
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////

rotary_speaker_audio_module::rotary_speaker_audio_module()
{
    mwhl_value = hold_value = 0.f;
    phase_h = phase_l = 0.f;
    aspeed_l = 1.f;
    aspeed_h = 1.f;
    dspeed = 0.f;
}    

void rotary_speaker_audio_module::set_sample_rate(uint32_t sr)
{
    srate = sr;
    setup();
}

void rotary_speaker_audio_module::setup()
{
    crossover1l.set_lp_rbj(800.f, 0.7, (float)srate);
    crossover1r.set_lp_rbj(800.f, 0.7, (float)srate);
    crossover2l.set_hp_rbj(800.f, 0.7, (float)srate);
    crossover2r.set_hp_rbj(800.f, 0.7, (float)srate);
}

void rotary_speaker_audio_module::activate()
{
    phase_h = phase_l = 0.f;
    maspeed_h = maspeed_l = 0.f;
    setup();
}

void rotary_speaker_audio_module::deactivate()
{
}

void rotary_speaker_audio_module::control_change(int ctl, int val)
{
    if (vibrato_mode == 3 && ctl == 64)
    {
        hold_value = val / 127.f;
        set_vibrato();
        return;
    }
    if (vibrato_mode == 4 && ctl == 1)
    {
        mwhl_value = val / 127.f;
        set_vibrato();
        return;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////

void multichorus_audio_module::activate()
{
    is_active = true;
    params_changed();
}

void multichorus_audio_module::deactivate()
{
    is_active = false;
}

void multichorus_audio_module::set_sample_rate(uint32_t sr) {
    srate = sr;
    left.setup(sr);
    right.setup(sr);
}

bool multichorus_audio_module::get_graph(int index, int subindex, float *data, int points, cairo_iface *context)
{
    if (!is_active)
        return false;
    int nvoices = (int)*params[par_voices];
    if (index == par_delay && subindex < 3) 
    {
        if (subindex < 2)
            set_channel_color(context, subindex);
        else {
            context->set_source_rgba(0.35, 0.4, 0.2);
            context->set_line_width(1.0);
        }
        return ::get_graph(*this, subindex, data, points);
    }
    if (index == par_rate && subindex < nvoices) {
        sine_multi_lfo<float, 8> &lfo = left.lfo;
        for (int i = 0; i < points; i++) {
            float phase = i * 2 * M_PI / points;
            // original -65536 to 65535 value
            float orig = subindex * lfo.voice_offset + ((lfo.voice_depth >> (30-13)) * 65536.0 * (0.95 * sin(phase) + 1)/ 8192.0) - 65536;
            // scale to -1..1
            data[i] = orig / 65536.0;
        }
        return true;
    }
    return false;
}

bool multichorus_audio_module::get_dot(int index, int subindex, float &x, float &y, int &size, cairo_iface *context)
{
    int voice = subindex >> 1;
    int nvoices = (int)*params[par_voices];
    if ((index != par_rate && index != par_depth) || voice >= nvoices)
        return false;

    float unit = (1 - *params[par_overlap]);
    float scw = 1 + unit * (nvoices - 1);
    set_channel_color(context, subindex);
    sine_multi_lfo<float, 8> &lfo = (subindex & 1 ? right : left).lfo;
    if (index == par_rate)
    {
        x = (double)(lfo.phase + lfo.vphase * voice) / 4096.0;
        y = 0.95 * sin(x * 2 * M_PI);
        y = (voice * unit + (y + 1) / 2) / scw * 2 - 1;
    }
    else
    {
        double ph = (double)(lfo.phase + lfo.vphase * voice) / 4096.0;
        x = 0.5 + 0.5 * sin(ph * 2 * M_PI);
        y = subindex & 1 ? -0.75 : 0.75;
        x = (voice * unit + x) / scw;
    }
    return true;
}

bool multichorus_audio_module::get_gridline(int index, int subindex, float &pos, bool &vertical, std::string &legend, cairo_iface *context)
{
    if (index == par_rate && !subindex)
    {
        pos = 0;
        vertical = false;
        return true;
    }
    if (index == par_delay)
        return get_freq_gridline(subindex, pos, vertical, legend, context);
    return false;
}

float multichorus_audio_module::freq_gain(int subindex, float freq, float srate)
{
    if (subindex == 2)
        return *params[par_amount] * left.post.freq_gain(freq, srate);
    return (subindex ? right : left).freq_gain(freq, srate);                
}

///////////////////////////////////////////////////////////////////////////////////////////////

compressor_audio_module::compressor_audio_module()
{
    is_active = false;
    srate = 0;
    last_generation = 0;
}

void compressor_audio_module::activate()
{
    is_active = true;
    linSlope = 0.f;
    peak = 0.f;
    clip = 0.f;
}

void compressor_audio_module::deactivate()
{
    is_active = false;
}

void compressor_audio_module::set_sample_rate(uint32_t sr)
{
    srate = sr;
    awL.set(sr);
    awR.set(sr);
}

bool compressor_audio_module::get_graph(int index, int subindex, float *data, int points, cairo_iface *context)
{ 
    if (!is_active)
        return false;
    if (subindex > 1) // 1
        return false;
    for (int i = 0; i < points; i++)
    {
        float input = dB_grid_inv(-1.0 + i * 2.0 / (points - 1));
        float output = output_level(input);
        if (subindex == 0)
            data[i] = dB_grid(input);
        else
            data[i] = dB_grid(output); 
    }
    if (subindex == (*params[param_bypass] > 0.5f ? 1 : 0))
        context->set_source_rgba(0.35, 0.4, 0.2, 0.3);
    else {
        context->set_source_rgba(0.35, 0.4, 0.2, 1);
        context->set_line_width(2);
    }
    return true;
}

bool compressor_audio_module::get_dot(int index, int subindex, float &x, float &y, int &size, cairo_iface *context)
{
    if (!is_active)
        return false;
    if (!subindex)
    {
        bool rms = *params[param_detection] == 0;
        float det = rms ? sqrt(detected) : detected;
        x = 0.5 + 0.5 * dB_grid(det);
        y = dB_grid(*params[param_bypass] > 0.5f ? det : output_level(det));
        return *params[param_bypass] > 0.5f ? false : true;
    }
    return false;
}

bool compressor_audio_module::get_gridline(int index, int subindex, float &pos, bool &vertical, std::string &legend, cairo_iface *context)
{
    bool tmp;
    vertical = (subindex & 1) != 0;
    bool result = get_freq_gridline(subindex >> 1, pos, tmp, legend, context, false);
    if (result && vertical) {
        if ((subindex & 4) && !legend.empty()) {
            legend = "";
        }
        else {
            size_t pos = legend.find(" dB");
            if (pos != std::string::npos)
                legend.erase(pos);
        }
        pos = 0.5 + 0.5 * pos;
    }
    return result;
}

// In case of doubt: this function is written by Thor. I just moved it to this file, damaging
// the output of "git annotate" in the process.
uint32_t compressor_audio_module::process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask)
{
    bool bypass = *params[param_bypass] > 0.5f;
    
    if(bypass) {
        numsamples += offset;
        while(offset < numsamples) {
            outs[0][offset] = ins[0][offset];
            outs[1][offset] = ins[1][offset];
            ++offset;
        }

        if(params[param_compression] != NULL) {
            *params[param_compression] = 1.f;
        }

        if(params[param_clip] != NULL) {
            *params[param_clip] = 0.f;
        }

        if(params[param_peak] != NULL) {
            *params[param_peak] = 0.f;
        }
  
        return inputs_mask;
    }

    bool rms = *params[param_detection] == 0;
    bool average = *params[param_stereo_link] == 0;
    int aweighting = fastf2i_drm(*params[param_aweighting]);
    float linThreshold = *params[param_threshold];
    ratio = *params[param_ratio];
    float attack = *params[param_attack];
    float attack_coeff = std::min(1.f, 1.f / (attack * srate / 4000.f));
    float release = *params[param_release];
    float release_coeff = std::min(1.f, 1.f / (release * srate / 4000.f));
    makeup = *params[param_makeup];
    knee = *params[param_knee];

    float linKneeSqrt = sqrt(knee);
    linKneeStart = linThreshold / linKneeSqrt;
    adjKneeStart = linKneeStart*linKneeStart;
    float linKneeStop = linThreshold * linKneeSqrt;
    
    threshold = log(linThreshold);
    kneeStart = log(linKneeStart);
    kneeStop = log(linKneeStop);
    compressedKneeStop = (kneeStop - threshold) / ratio + threshold;
    
    if (aweighting >= 2)
    {
        bpL.set_highshelf_rbj(5000, 0.707, 10 << (aweighting - 2), srate);
        bpR.copy_coeffs(bpL);
        bpL.sanitize();
        bpR.sanitize();
    }

    numsamples += offset;
    
    float compression = 1.f;

    peak -= peak * 5.f * numsamples / srate;
    
    clip -= std::min(clip, numsamples);

    while(offset < numsamples) {
        float left = ins[0][offset] * *params[param_input];
        float right = ins[1][offset] * *params[param_input];
        
        if(aweighting == 1) {
            left = awL.process(left);
            right = awR.process(right);
        }
        else if(aweighting >= 2) {
            left = bpL.process(left);
            right = bpR.process(right);
        }
        
        float absample = average ? (fabs(left) + fabs(right)) * 0.5f : std::max(fabs(left), fabs(right));
        if(rms) absample *= absample;
        
        linSlope += (absample - linSlope) * (absample > linSlope ? attack_coeff : release_coeff);
        
        float gain = 1.f;

        if(linSlope > 0.f) {
            gain = output_gain(linSlope, rms);
        }

        compression = gain;
        gain *= makeup;

        float outL = ins[0][offset] * gain;
        float outR = ins[1][offset] * gain;
        
        outs[0][offset] = outL;
        outs[1][offset] = outR;
        
        ++offset;
        
        float maxLR = std::max(fabs(outL), fabs(outR));
        
        if(maxLR > 1.f) clip = srate >> 3; /* blink clip LED for 125 ms */
        
        if(maxLR > peak) {
            peak = maxLR;
        }
    }
    
    detected = linSlope;
    
    if(params[param_compression] != NULL) {
        *params[param_compression] = compression;
    }

    if(params[param_clip] != NULL) {
        *params[param_clip] = clip;
    }

    if(params[param_peak] != NULL) {
        *params[param_peak] = peak;
    }

    return inputs_mask;
}


/// Multibandcompressor by Markus Schmidt
///
/// This module splits the signal in four different bands
/// and sends them through multiple filters (implemented by
/// Krzysztof). They are processed by a compressing routine
/// (implemented by Thor) afterwards and summed up to the
/// final output again.
///////////////////////////////////////////////////////////////////////////////////////////////

multibandcompressor_audio_module::multibandcompressor_audio_module()
{
    is_active = false;
    srate = 0;
    // zero all displays
    clip_inL    = 0.f;
    clip_inR    = 0.f;
    clip_outL   = 0.f;
    clip_outR   = 0.f;
    meter_inL  = 0.f;
    meter_inR  = 0.f;
    meter_outL = 0.f;
    meter_outR = 0.f;
}

void multibandcompressor_audio_module::activate()
{
    is_active = true;
    // set all filters and strips
    params_changed();
    // activate all strips
    for (int j = 0; j < strips; j ++) {
        strip[j].activate();
        strip[j].id = j;
    }
}

void multibandcompressor_audio_module::deactivate()
{
    is_active = false;
    // deactivate all strips
    for (int j = 0; j < strips; j ++) {
        strip[j].deactivate();
    }
}

void multibandcompressor_audio_module::params_changed()
{
    // set the params of all filters
    if(*params[param_freq0] != freq_old[0] or *params[param_sep0] != sep_old[0] or *params[param_q0] != q_old[0]) {
        lpL0.set_lp_rbj((float)(*params[param_freq0] * (1 - *params[param_sep0])), *params[param_q0], (float)srate);
        lpR0.set_lp_rbj((float)(*params[param_freq0] * (1 - *params[param_sep0])), *params[param_q0], (float)srate);
        hpL0.set_hp_rbj((float)(*params[param_freq0] * (1 + *params[param_sep0])), *params[param_q0], (float)srate);
        hpR0.set_hp_rbj((float)(*params[param_freq0] * (1 + *params[param_sep0])), *params[param_q0], (float)srate);
        freq_old[0] = *params[param_freq0];
        sep_old[0]  = *params[param_sep2];
        q_old[0]    = *params[param_q2];
    }
    if(*params[param_freq1] != freq_old[1] or *params[param_sep1] != sep_old[1] or *params[param_q1] != q_old[1]) {
        lpL1.set_lp_rbj((float)(*params[param_freq1] * (1 - *params[param_sep1])), *params[param_q1], (float)srate);
        lpR1.set_lp_rbj((float)(*params[param_freq1] * (1 - *params[param_sep1])), *params[param_q1], (float)srate);
        hpL1.set_hp_rbj((float)(*params[param_freq1] * (1 + *params[param_sep1])), *params[param_q1], (float)srate);
        hpR1.set_hp_rbj((float)(*params[param_freq1] * (1 + *params[param_sep1])), *params[param_q1], (float)srate);
        freq_old[1] = *params[param_freq1];
        sep_old[1]  = *params[param_sep2];
        q_old[1]    = *params[param_q2];
    }
    if(*params[param_freq2] != freq_old[2] or *params[param_sep2] != sep_old[2] or *params[param_q2] != q_old[2]) {
        lpL2.set_lp_rbj((float)(*params[param_freq2] * (1 - *params[param_sep2])), *params[param_q2], (float)srate);
        lpR2.set_lp_rbj((float)(*params[param_freq2] * (1 - *params[param_sep2])), *params[param_q2], (float)srate);
        hpL2.set_hp_rbj((float)(*params[param_freq2] * (1 + *params[param_sep2])), *params[param_q2], (float)srate);
        hpR2.set_hp_rbj((float)(*params[param_freq2] * (1 + *params[param_sep2])), *params[param_q2], (float)srate);
        freq_old[2] = *params[param_freq2];
        sep_old[2]  = *params[param_sep2];
        q_old[2]    = *params[param_q2];
    }
    // set the params of all strips
    for (int j = 0; j < strips; j ++) {
        switch (j) {
            case 0:
                strip[j].set_params(*params[param_attack0], *params[param_release0], *params[param_threshold0], *params[param_ratio0], *params[param_knee0], *params[param_makeup0], *params[param_detection0], *params[param_bypass0], *params[param_mute0]);
                break;
            case 1:
                strip[j].set_params(*params[param_attack1], *params[param_release1], *params[param_threshold1], *params[param_ratio1], *params[param_knee1], *params[param_makeup1], *params[param_detection1], *params[param_bypass1], *params[param_mute1]);
                break;
            case 2:
                strip[j].set_params(*params[param_attack2], *params[param_release2], *params[param_threshold2], *params[param_ratio2], *params[param_knee2], *params[param_makeup2], *params[param_detection2], *params[param_bypass2], *params[param_mute2]);
                break;
            case 3:
                strip[j].set_params(*params[param_attack3], *params[param_release3], *params[param_threshold3], *params[param_ratio3], *params[param_knee3], *params[param_makeup3], *params[param_detection3], *params[param_bypass3], *params[param_mute3]);
                break;
        }
    }
}

void multibandcompressor_audio_module::set_sample_rate(uint32_t sr)
{
    srate = sr;
    // set srate of all strips
    for (int j = 0; j < strips; j ++) {
        strip[j].set_sample_rate(srate);
    }
}

uint32_t multibandcompressor_audio_module::process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask)
{
    bool bypass = *params[param_bypass] > 0.5f;
    numsamples += offset;
    if(bypass) {
        // everything bypassed
        while(offset < numsamples) {
            outs[0][offset] = ins[0][offset];
            outs[1][offset] = ins[1][offset];
            ++offset;
        }
        // displays, too
        clip_inL    = 0.f;
        clip_inR    = 0.f;
        clip_outL   = 0.f;
        clip_outR   = 0.f;
        meter_inL  = 0.f;
        meter_inR  = 0.f;
        meter_outL = 0.f;
        meter_outR = 0.f;
    } else {
        // process all strips
        
        // determine mute state of strips
        mute[0] = *params[param_mute0] > 0.f ? true : false;
        mute[1] = *params[param_mute1] > 0.f ? true : false;
        mute[2] = *params[param_mute2] > 0.f ? true : false;
        mute[3] = *params[param_mute3] > 0.f ? true : false;
        
        // let meters fall a bit
        clip_inL    -= std::min(clip_inL,  numsamples);
        clip_inR    -= std::min(clip_inR,  numsamples);
        clip_outL   -= std::min(clip_outL, numsamples);
        clip_outR   -= std::min(clip_outR, numsamples);
        meter_inL  -= meter_inL * 2.5 * numsamples / srate;
        meter_inR  -= meter_inR * 2.5 * numsamples / srate;
        meter_outL -= meter_outL * 2.5 * numsamples / srate;
        meter_outR -= meter_outR * 2.5 * numsamples / srate;
        
        while(offset < numsamples) {
            // cycle through samples
            float inL = ins[0][offset];
            float inR = ins[1][offset];
            // in level
            inR *= *params[param_level_in];
            inL *= *params[param_level_in];
            // out vars
            float outL = 0.f;
            float outR = 0.f;
            for (int i = 0; i < strips; i ++) {
                // cycle trough strips
                if (!mute[i]) {
                    // strip unmuted
                    float left  = inL;
                    float right = inR;
                    // send trough filters
                    switch (i) {
                        case 0:
                            left  = lpL0.process(left);
                            right = lpR0.process(right);
                            lpL0.sanitize();
                            lpR0.sanitize();
                            break;
                        case 1:
                            left  = lpL1.process(left);
                            right = lpR1.process(right);
                            left  = hpL0.process(left);
                            right = hpR0.process(right);
                            lpL1.sanitize();
                            lpR1.sanitize();
                            hpL0.sanitize();
                            hpR0.sanitize();
                            break;
                        case 2:
                            left  = lpL2.process(left);
                            right = lpR2.process(right);
                            left  = hpL1.process(left);
                            right = hpR1.process(right);
                            lpL2.sanitize();
                            lpR2.sanitize();
                            hpL1.sanitize();
                            hpR1.sanitize();
                            break;
                        case 3:
                            left  = hpL2.process(left);
                            right = hpR2.process(right);
                            hpL2.sanitize();
                            hpR2.sanitize();
                            break;
                    }
                    // process gain reduction
                    strip[i].process(left, right);
                    // sum up output
                    outL += left;
                    outR += right;
                } else {
                    // strip muted
                    
                }
                
                
            } // process single strip
            
            // even out filters gain reduction
            // 3dB - levelled manually (based on default sep and q settings)
            outL *= 1.414213562;
            outR *= 1.414213562;
            
            // out level
            outL *= *params[param_level_out];
            outR *= *params[param_level_out];
            
            // send to output
            outs[0][offset] = outL;
            outs[1][offset] = outR;
            
            // clip LED's
            if(inL > 1.f) {
                clip_inL  = srate >> 3;
            }
            if(inR > 1.f) {
                clip_inR  = srate >> 3;
            }
            if(outL > 1.f) {
                clip_outL = srate >> 3;
            }
            if(outR > 1.f) {
                clip_outR = srate >> 3;
            }
            // rise up in / out meters
            if(inL > meter_inL) {
                meter_inL = inL;
            }
            if(inR > meter_inR) {
                meter_inR = inR;
            }
            if(outL > meter_outL) {
                meter_outL = outL;
            }
            if(outR > meter_outR) {
                meter_outR = outR;
            }
            // next sample
            ++offset;
        } // cycle trough samples
        
    } // process all strips (no bypass)
    
    // draw meters
    if(params[param_clip_inL] != NULL) {
        *params[param_clip_inL] = clip_inL;
    }
    if(params[param_clip_inR] != NULL) {
        *params[param_clip_inR] = clip_inR;
    }
    if(params[param_clip_outL] != NULL) {
        *params[param_clip_outL] = clip_outL;
    }
    if(params[param_clip_outR] != NULL) {
        *params[param_clip_outR] = clip_outR;
    }
    
    if(params[param_meter_inL] != NULL) {
        *params[param_meter_inL] = meter_inL;
    }
    if(params[param_meter_inR] != NULL) {
        *params[param_meter_inR] = meter_inR;
    }
    if(params[param_meter_outL] != NULL) {
        *params[param_meter_outL] = meter_outL;
    }
    if(params[param_meter_outR] != NULL) {
        *params[param_meter_outR] = meter_outR;
    }
    // draw strip meters
    if(bypass > 0.5f) {
        if(params[param_compression0] != NULL) {
            *params[param_compression0] = 1.0f;
        }
        if(params[param_compression1] != NULL) {
            *params[param_compression1] = 1.0f;
        }
        if(params[param_compression2] != NULL) {
            *params[param_compression2] = 1.0f;
        }
        if(params[param_compression3] != NULL) {
            *params[param_compression3] = 1.0f;
        }

        if(params[param_output0] != NULL) {
            *params[param_output0] = 0.0f;
        }
        if(params[param_output1] != NULL) {
            *params[param_output1] = 0.0f;
        }
        if(params[param_output2] != NULL) {
            *params[param_output2] = 0.0f;
        }
        if(params[param_output3] != NULL) {
            *params[param_output3] = 0.0f;
        }
    } else {
        if(params[param_compression0] != NULL) {
            *params[param_compression0] = strip[0].get_comp_level();
        }
        if(params[param_compression1] != NULL) {
            *params[param_compression1] = strip[1].get_comp_level();
        }
        if(params[param_compression2] != NULL) {
            *params[param_compression2] = strip[2].get_comp_level();
        }
        if(params[param_compression3] != NULL) {
            *params[param_compression3] = strip[3].get_comp_level();
        }

        if(params[param_output0] != NULL) {
            *params[param_output0] = strip[0].get_output_level();
        }
        if(params[param_output1] != NULL) {
            *params[param_output1] = strip[1].get_output_level();
        }
        if(params[param_output2] != NULL) {
            *params[param_output2] = strip[2].get_output_level();
        }
        if(params[param_output3] != NULL) {
            *params[param_output3] = strip[3].get_output_level();
        }
    }
    // whatever has to be returned x)
    return outputs_mask;
}
bool multibandcompressor_audio_module::get_graph(int index, int subindex, float *data, int points, cairo_iface *context)
{
    // let's handle by the corresponding strip
    switch (index) {
        case param_compression0:
            return strip[0].get_graph(subindex, data, points, context);
            break;
        case param_compression1:
            return strip[1].get_graph(subindex, data, points, context);
            break;
        case param_compression2:
            return strip[2].get_graph(subindex, data, points, context);
            break;
        case param_compression3:
            return strip[3].get_graph(subindex, data, points, context);
            break;
    }
    return false;
}

bool multibandcompressor_audio_module::get_dot(int index, int subindex, float &x, float &y, int &size, cairo_iface *context)
{
    // let's handle by the corresponding strip
    switch (index) {
        case param_compression0:
            return strip[0].get_dot(subindex, x, y, size, context);
            break;
        case param_compression1:
            return strip[1].get_dot(subindex, x, y, size, context);
            break;
        case param_compression2:
            return strip[2].get_dot(subindex, x, y, size, context);
            break;
        case param_compression3:
            return strip[3].get_dot(subindex, x, y, size, context);
            break;
    }
    return false;
}

bool multibandcompressor_audio_module::get_gridline(int index, int subindex, float &pos, bool &vertical, std::string &legend, cairo_iface *context)
{
    // let's handle by the corresponding strip
    switch (index) {
        case param_compression0:
            return strip[0].get_gridline(subindex, pos, vertical, legend, context);
            break;
        case param_compression1:
            return strip[1].get_gridline(subindex, pos, vertical, legend, context);
            break;
        case param_compression2:
            return strip[2].get_gridline(subindex, pos, vertical, legend, context);
            break;
        case param_compression3:
            return strip[3].get_gridline(subindex, pos, vertical, legend, context);
            break;
    }
    return false;
}

int multibandcompressor_audio_module::get_changed_offsets(int index, int generation, int &subindex_graph, int &subindex_dot, int &subindex_gridline)
{
    // let's handle by the corresponding strip
    switch (index) {
        case param_compression0:
            return strip[0].get_changed_offsets(generation, subindex_graph, subindex_dot, subindex_gridline);
            break;
        case param_compression1:
            return strip[1].get_changed_offsets(generation, subindex_graph, subindex_dot, subindex_gridline);
            break;
        case param_compression2:
            return strip[2].get_changed_offsets(generation, subindex_graph, subindex_dot, subindex_gridline);
            break;
        case param_compression3:
            return strip[3].get_changed_offsets(generation, subindex_graph, subindex_dot, subindex_gridline);
            break;
    }
    return 0;
}
/// Gain reduction module implemented by Markus Schmidt
/// Nearly all functions of this module are originally written
/// by Thor, while some features have been stripped (mainly stereo linking
/// and frequency correction as implemented in his Compressor above)
/// To save some CPU.
////////////////////////////////////////////////////////////////////////////////
gain_reduction_audio_module::gain_reduction_audio_module()
{
    is_active       = false;
    last_generation = 0;
}

void gain_reduction_audio_module::activate()
{
    is_active = true;
    linSlope   = 0.f;
    meter_out  = 0.f;
    meter_comp = 1.f;
    float l, r;
    l = r = 0.f;
    float byp = bypass;
    bypass = 0.0;
    process(l, r);
    bypass = byp;
}

void gain_reduction_audio_module::deactivate()
{
    is_active = false;
}

void gain_reduction_audio_module::process(float &left, float &right)
{
    meter_out -= meter_out * 5.f * 1 / srate;
    meter_comp += (1 - meter_comp) * 5.f * 1 / srate;
    float gain = 1.f;
    if(bypass < 0.5f) {
        // this routine is mainly copied from thor's compressor module
        // greatest sounding compressor I've heard!
        bool rms = detection == 0;
        float linThreshold = threshold;
        float attack_coeff = std::min(1.f, 1.f / (attack * srate / 4000.f));
        float release_coeff = std::min(1.f, 1.f / (release * srate / 4000.f));
        float linKneeSqrt = sqrt(knee);
        linKneeStart = linThreshold / linKneeSqrt;
        adjKneeStart = linKneeStart*linKneeStart;
        float linKneeStop = linThreshold * linKneeSqrt;
        thres = log(linThreshold);
        kneeStart = log(linKneeStart);
        kneeStop = log(linKneeStop);
        compressedKneeStop = (kneeStop - thres) / ratio + thres;
        
        float absample = (fabs(left) + fabs(right)) * 0.5f;
        if(rms) absample *= absample;
            
        linSlope += (absample - linSlope) * (absample > linSlope ? attack_coeff : release_coeff);
        
        if(linSlope > 0.f) {
            gain = output_gain(linSlope, rms);
        }

        gain *= makeup;
        
        left *= gain;
        right *= gain;
        
        detected = rms ? sqrt(linSlope) : linSlope;
    }
    
    float maxLR = std::max(fabs(left), fabs(right));
    
    if(maxLR > meter_out) {
        meter_out = maxLR;
    }
    if(gain < meter_comp) {
        meter_comp = gain;
    }
}

float gain_reduction_audio_module::output_level(float slope) {
    return slope * output_gain(slope, false) * makeup;
}

float gain_reduction_audio_module::output_gain(float linSlope, bool rms) {
    //this calculation is also thor's work
    if(linSlope > (rms ? adjKneeStart : linKneeStart)) {
        float slope = log(linSlope);
        if(rms) slope *= 0.5f;

        float gain = 0.f;
        float delta = 0.f;
        if(IS_FAKE_INFINITY(ratio)) {
            gain = thres;
            delta = 0.f;
        } else {
            gain = (slope - thres) / ratio + thres;
            delta = 1.f / ratio;
        }
        
        if(knee > 1.f && slope < kneeStop) {
            gain = hermite_interpolation(slope, kneeStart, kneeStop, kneeStart, compressedKneeStop, 1.f, delta);
        }
        
        return exp(gain - slope);
    }

    return 1.f;
}

void gain_reduction_audio_module::set_sample_rate(uint32_t sr)
{
    srate = sr;
}
void gain_reduction_audio_module::set_params(float att, float rel, float thr, float rat, float kn, float mak, float det, float byp, float mu)
{
    // set all params
    attack          = att;
    release         = rel;
    threshold       = thr;
    ratio           = rat;
    knee            = kn;
    makeup          = mak;
    detection       = det;
    bypass          = byp;
    mute            = mu;
    if(mute > 0.f) {
        meter_out  = 0.f;
        meter_comp = 1.f;
    }
}
float gain_reduction_audio_module::get_output_level() {
    // returns output level (max(left, right))
    return meter_out;
}
float gain_reduction_audio_module::get_comp_level() {
    // returns amount of compression
    return meter_comp;
}

bool gain_reduction_audio_module::get_graph(int subindex, float *data, int points, cairo_iface *context)
{
    if (!is_active)
        return false;
    if (subindex > 1) // 1
        return false;
    for (int i = 0; i < points; i++)
    {
        float input = dB_grid_inv(-1.0 + i * 2.0 / (points - 1));
        if (subindex == 0)
            data[i] = dB_grid(input);
        else {
            float output = output_level(input);
            data[i] = dB_grid(output);
        }
    }
    if (subindex == (bypass > 0.5f ? 1 : 0) or mute > 0.1f)
        context->set_source_rgba(0.35, 0.4, 0.2, 0.3);
    else {
        context->set_source_rgba(0.35, 0.4, 0.2, 1);
        context->set_line_width(1.5);
    }
    return true;
}

bool gain_reduction_audio_module::get_dot(int subindex, float &x, float &y, int &size, cairo_iface *context)
{
    if (!is_active)
        return false;
    if (!subindex)
    {
        if(bypass > 0.5f or mute > 0.f) {
            return false;
        } else {
            bool rms = detection == 0;
            float det = rms ? sqrt(detected) : detected;
            x = 0.5 + 0.5 * dB_grid(det);
            y = dB_grid(bypass > 0.5f or mute > 0.f ? det : output_level(det));
            return true;
        }
    }
    return false;
}

bool gain_reduction_audio_module::get_gridline(int subindex, float &pos, bool &vertical, std::string &legend, cairo_iface *context)
{
    bool tmp;
    vertical = (subindex & 1) != 0;
    bool result = get_freq_gridline(subindex >> 1, pos, tmp, legend, context, false);
    if (result && vertical) {
        if ((subindex & 4) && !legend.empty()) {
            legend = "";
        }
        else {
            size_t pos = legend.find(" dB");
            if (pos != std::string::npos)
                legend.erase(pos);
        }
        pos = 0.5 + 0.5 * pos;
    }
    return result;
}

int gain_reduction_audio_module::get_changed_offsets(int generation, int &subindex_graph, int &subindex_dot, int &subindex_gridline)
{
    subindex_graph = 0;
    subindex_dot = 0;
    subindex_gridline = generation ? INT_MAX : 0;

    if (fabs(threshold-old_threshold) + fabs(ratio - old_ratio) + fabs(knee - old_knee) + fabs(makeup - old_makeup) + fabs(detection - old_detection) + fabs(bypass - old_bypass) + fabs(mute - old_mute) > 0.000001f)
    {
        old_threshold = threshold;
        old_ratio     = ratio;
        old_knee      = knee;
        old_makeup    = makeup;
        old_detection = detection;
        old_bypass    = bypass;
        old_mute      = mute;
        last_generation++;
    }

    if (generation == last_generation)
        subindex_graph = 2;
    return last_generation;
}
