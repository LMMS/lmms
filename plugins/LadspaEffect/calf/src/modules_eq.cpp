/* Calf DSP plugin pack
 * Equalization related plugins
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
#include <limits.h>
#include <memory.h>
#include <calf/giface.h>
#include <calf/modules_eq.h>

using namespace dsp;
using namespace calf_plugins;

/// Equalizer 12 Band by Markus Schmidt
///
/// This module is based on Krzysztof's filters. It provides a couple
/// of different chained filters.
///////////////////////////////////////////////////////////////////////////////////////////////

template<class BaseClass, bool has_lphp>
equalizerNband_audio_module<BaseClass, has_lphp>::equalizerNband_audio_module()
{
    is_active = false;
    srate = 0;
    last_generation = 0;
    hp_freq_old = lp_freq_old = 0;
    hs_freq_old = ls_freq_old = 0;
    hs_level_old = ls_level_old = 0;
    for (int i = 0; i < AM::PeakBands; i++)
    {
        p_freq_old[i] = 0;
        p_level_old[i] = 0;
        p_q_old[i] = 0;
    }
}

template<class BaseClass, bool has_lphp>
void equalizerNband_audio_module<BaseClass, has_lphp>::activate()
{
    is_active = true;
    // set all filters
    params_changed();
    meters.reset();
}

template<class BaseClass, bool has_lphp>
void equalizerNband_audio_module<BaseClass, has_lphp>::deactivate()
{
    is_active = false;
}

static inline void copy_lphp(biquad_d2<float> filters[3][2])
{
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 2; j++)
            if (i || j)
                filters[i][j].copy_coeffs(filters[0][0]);
}

template<class BaseClass, bool has_lphp>
void equalizerNband_audio_module<BaseClass, has_lphp>::params_changed()
{
    // set the params of all filters
    
    // lp/hp first (if available)
    if (has_lphp)
    {
        hp_mode = (CalfEqMode)(int)*params[AM::param_hp_mode];
        lp_mode = (CalfEqMode)(int)*params[AM::param_lp_mode];

        float hpfreq = *params[AM::param_hp_freq], lpfreq = *params[AM::param_lp_freq];
        
        if(hpfreq != hp_freq_old) {
            hp[0][0].set_hp_rbj(hpfreq, 0.707, (float)srate, 1.0);
            copy_lphp(hp);
            hp_freq_old = hpfreq;
        }
        if(lpfreq != lp_freq_old) {
            lp[0][0].set_lp_rbj(lpfreq, 0.707, (float)srate, 1.0);
            copy_lphp(lp);
            lp_freq_old = lpfreq;
        }
    }
    
    // then shelves
    float hsfreq = *params[AM::param_hs_freq], hslevel = *params[AM::param_hs_level];
    float lsfreq = *params[AM::param_ls_freq], lslevel = *params[AM::param_ls_level];
    
    if(lsfreq != ls_freq_old or lslevel != ls_level_old) {
        lsL.set_lowshelf_rbj(lsfreq, 0.707, lslevel, (float)srate);
        lsR.copy_coeffs(lsL);
        ls_level_old = lslevel;
        ls_freq_old = lsfreq;
    }
    if(hsfreq != hs_freq_old or hslevel != hs_level_old) {
        hsL.set_highshelf_rbj(hsfreq, 0.707, hslevel, (float)srate);
        hsR.copy_coeffs(hsL);
        hs_level_old = hslevel;
        hs_freq_old = hsfreq;
    }
    for (int i = 0; i < AM::PeakBands; i++)
    {
        int offset = i * params_per_band;
        float freq = *params[AM::param_p1_freq + offset];
        float level = *params[AM::param_p1_level + offset];
        float q = *params[AM::param_p1_q + offset];
        if(freq != p_freq_old[i] or level != p_level_old[i] or q != p_q_old[i]) {
            pL[i].set_peakeq_rbj(freq, q, level, (float)srate);
            pR[i].copy_coeffs(pL[i]);
            p_freq_old[i] = freq;
            p_level_old[i] = level;
            p_q_old[i] = q;
        }
    }
}

template<class BaseClass, bool has_lphp>
inline void equalizerNband_audio_module<BaseClass, has_lphp>::process_hplp(float &left, float &right)
{
    if (!has_lphp)
        return;
    if (*params[AM::param_lp_active] > 0.f)
    {
        switch(lp_mode)
        {
            case MODE12DB:
                left = lp[0][0].process(left);
                right = lp[0][1].process(right);
                break;
            case MODE24DB:
                left = lp[1][0].process(lp[0][0].process(left));
                right = lp[1][1].process(lp[0][1].process(right));
                break;
            case MODE36DB:
                left = lp[2][0].process(lp[1][0].process(lp[0][0].process(left)));
                right = lp[2][1].process(lp[1][1].process(lp[0][1].process(right)));
                break;
        }
    }
    if (*params[AM::param_hp_active] > 0.f)
    {
        switch(hp_mode)
        {
            case MODE12DB:
                left = hp[0][0].process(left);
                right = hp[0][1].process(right);
                break;
            case MODE24DB:
                left = hp[1][0].process(hp[0][0].process(left));
                right = hp[1][1].process(hp[0][1].process(right));
                break;
            case MODE36DB:
                left = hp[2][0].process(hp[1][0].process(hp[0][0].process(left)));
                right = hp[2][1].process(hp[1][1].process(hp[0][1].process(right)));
                break;
        }
    }
}

template<class BaseClass, bool has_lphp>
uint32_t equalizerNband_audio_module<BaseClass, has_lphp>::process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask)
{
    bool bypass = *params[AM::param_bypass] > 0.f;
    uint32_t orig_offset = offset;
    uint32_t orig_numsamples = numsamples;
    numsamples += offset;
    if(bypass) {
        // everything bypassed
        while(offset < numsamples) {
            outs[0][offset] = ins[0][offset];
            outs[1][offset] = ins[1][offset];
            ++offset;
        }
        // displays, too
        meters.bypassed(params, orig_numsamples);
    } else {
        // process
        while(offset < numsamples) {
            // cycle through samples
            float outL = 0.f;
            float outR = 0.f;
            float inL = ins[0][offset];
            float inR = ins[1][offset];
            // in level
            inR *= *params[AM::param_level_in];
            inL *= *params[AM::param_level_in];
            
            float procL = inL;
            float procR = inR;
            
            // all filters in chain
            process_hplp(procL, procR);
            if(*params[AM::param_ls_active] > 0.f) {
                procL = lsL.process(procL);
                procR = lsR.process(procR);
            }
            if(*params[AM::param_hs_active] > 0.f) {
                procL = hsL.process(procL);
                procR = hsR.process(procR);
            }
            for (int i = 0; i < AM::PeakBands; i++)
            {
                if(*params[AM::param_p1_active + i * params_per_band] > 0.f) {
                    procL = pL[i].process(procL);
                    procR = pR[i].process(procR);
                }
            }
            
            outL = procL * *params[AM::param_level_out];
            outR = procR * *params[AM::param_level_out];
            
            // send to output
            outs[0][offset] = outL;
            outs[1][offset] = outR;
                        
            // next sample
            ++offset;
        } // cycle trough samples
        meters.process(params, ins, outs, orig_offset, orig_numsamples);
        // clean up
        for(int i = 0; i < 3; ++i) {
            hp[i][0].sanitize();
            hp[i][1].sanitize();
            lp[i][0].sanitize();
            lp[i][1].sanitize();
        }
        lsL.sanitize();
        hsR.sanitize();
        for(int i = 0; i < AM::PeakBands; ++i) {
            pL[i].sanitize();
            pR[i].sanitize();
        }
    }
    // whatever has to be returned x)
    return outputs_mask;
}

template<class BaseClass, bool has_lphp>
bool equalizerNband_audio_module<BaseClass, has_lphp>::get_graph(int index, int subindex, float *data, int points, cairo_iface *context) const
{
    if (!is_active)
        return false;
    if (index == AM::param_p1_freq && !subindex) {
        context->set_line_width(1.5);
        return ::get_graph(*this, subindex, data, points, 32, 0);
    }
    return false;
}

template<class BaseClass, bool has_lphp>
bool equalizerNband_audio_module<BaseClass, has_lphp>::get_gridline(int index, int subindex, float &pos, bool &vertical, std::string &legend, cairo_iface *context) const
{
    if (!is_active) {
        return false;
    } else {
        return get_freq_gridline(subindex, pos, vertical, legend, context, true, 32, 0);
    }
}

template<class BaseClass, bool has_lphp>
int equalizerNband_audio_module<BaseClass, has_lphp>::get_changed_offsets(int index, int generation, int &subindex_graph, int &subindex_dot, int &subindex_gridline) const
{
    if (!is_active) {
        return false;
    } else {
        bool changed = false;
        for (int i = 0; i < graph_param_count && !changed; i++)
        {
            if (*params[AM::first_graph_param + i] != old_params_for_graph[i])
                changed = true;
        }
        if (changed)
        {
            for (int i = 0; i < graph_param_count; i++)
                old_params_for_graph[i] = *params[AM::first_graph_param + i];
            
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
    return false;
}

static inline float adjusted_lphp_gain(const float *const *params, int param_active, int param_mode, const biquad_d2<float> &filter, float freq, float srate)
{
    if(*params[param_active] > 0.f) {
        float gain = filter.freq_gain(freq, srate);
        switch((int)*params[param_mode]) {
            case MODE12DB:
                return gain;
            case MODE24DB:
                return gain * gain;
            case MODE36DB:
                return gain * gain * gain;
        }
    }
    return 1;
}

template<class BaseClass, bool use_hplp>
float equalizerNband_audio_module<BaseClass, use_hplp>::freq_gain(int index, double freq, uint32_t sr) const
{
    float ret = 1.f;
    if (use_hplp)
    {
        ret *= adjusted_lphp_gain(params, AM::param_hp_active, AM::param_hp_mode, hp[0][0], freq, (float)sr);
        ret *= adjusted_lphp_gain(params, AM::param_lp_active, AM::param_lp_mode, lp[0][0], freq, (float)sr);
    }
    ret *= (*params[AM::param_ls_active] > 0.f) ? lsL.freq_gain(freq, sr) : 1;
    ret *= (*params[AM::param_hs_active] > 0.f) ? hsL.freq_gain(freq, sr) : 1;
    for (int i = 0; i < PeakBands; i++)
        ret *= (*params[AM::param_p1_active + i * params_per_band] > 0.f) ? pL[i].freq_gain(freq, (float)sr) : 1;
    return ret;
}

template class equalizerNband_audio_module<equalizer5band_metadata, false>;
template class equalizerNband_audio_module<equalizer8band_metadata, true>;
template class equalizerNband_audio_module<equalizer12band_metadata, true>;

