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
        context->set_source_rgba(0.75, 1, 0);
    else
        context->set_source_rgba(0, 1, 0.75);
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
                context->set_source_rgba(0.25, 0.25, 0.25, 0.75);
            else
                context->set_source_rgba(0.25, 0.25, 0.25, 0.5);
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
        context->set_source_rgba(0.25, 0.25, 0.25, subindex & 1 ? 0.5 : 0.75);
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

int frequency_response_line_graph::get_changed_offsets(int generation, int &subindex_graph, int &subindex_dot, int &subindex_gridline)
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

int filter_audio_module::get_changed_offsets(int generation, int &subindex_graph, int &subindex_dot, int &subindex_gridline)
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
            context->set_source_rgba(0, 1, 0);
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
        context->set_source_rgba(0.5, 0.5, 0.5, 0.5);
    else {
        context->set_source_rgba(0, 1, 0, 1);
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
        int count = numsamples * sizeof(float);
        memcpy(outs[0], ins[0], count);
        memcpy(outs[1], ins[1], count);

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
        float left = ins[0][offset];
        float right = ins[1][offset];
        
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
