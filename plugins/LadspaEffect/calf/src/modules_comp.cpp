/* Calf DSP plugin pack
 * Compression related plugins
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
#include <calf/modules_comp.h>

using namespace dsp;
using namespace calf_plugins;

#define SET_IF_CONNECTED(name) if (params[AM::param_##name] != NULL) *params[AM::param_##name] = name;

/// Multiband Compressor by Markus Schmidt
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
    for(int i = 0; i < strips - 1; i ++) {
        freq_old[i] = -1;
        sep_old[i] = -1;
        q_old[i] = -1;
    }
    mode_old = -1;
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
    // determine mute/solo states
    solo[0] = *params[param_solo0] > 0.f ? true : false;
    solo[1] = *params[param_solo1] > 0.f ? true : false;
    solo[2] = *params[param_solo2] > 0.f ? true : false;
    solo[3] = *params[param_solo3] > 0.f ? true : false;
    no_solo = (*params[param_solo0] > 0.f ||
            *params[param_solo1] > 0.f ||
            *params[param_solo2] > 0.f ||
            *params[param_solo3] > 0.f) ? false : true;
    int i;
    int j1;
    switch(mode) {
        case 0:
        default:
            j1 = 0;
            break;
        case 1:
            j1 = 2;
            break;
    }
    // set the params of all filters
    if(*params[param_freq0] != freq_old[0] or *params[param_sep0] != sep_old[0] or *params[param_q0] != q_old[0] or *params[param_mode] != mode_old) {
        lpL[0][0].set_lp_rbj((float)(*params[param_freq0] * (1 - *params[param_sep0])), *params[param_q0], (float)srate);
        hpL[0][0].set_hp_rbj((float)(*params[param_freq0] * (1 + *params[param_sep0])), *params[param_q0], (float)srate);
        lpR[0][0].copy_coeffs(lpL[0][0]);
        hpR[0][0].copy_coeffs(hpL[0][0]);
        for(i = 1; i <= j1; i++) {
            lpL[0][i].copy_coeffs(lpL[0][0]);
            hpL[0][i].copy_coeffs(hpL[0][0]);
            lpR[0][i].copy_coeffs(lpL[0][0]);
            hpR[0][i].copy_coeffs(hpL[0][0]);
        }
        freq_old[0] = *params[param_freq0];
        sep_old[0]  = *params[param_sep0];
        q_old[0]    = *params[param_q0];
    }
    if(*params[param_freq1] != freq_old[1] or *params[param_sep1] != sep_old[1] or *params[param_q1] != q_old[1] or *params[param_mode] != mode_old) {
        lpL[1][0].set_lp_rbj((float)(*params[param_freq1] * (1 - *params[param_sep1])), *params[param_q1], (float)srate);
        hpL[1][0].set_hp_rbj((float)(*params[param_freq1] * (1 + *params[param_sep1])), *params[param_q1], (float)srate);
        lpR[1][0].copy_coeffs(lpL[1][0]);
        hpR[1][0].copy_coeffs(hpL[1][0]);
        for(i = 1; i <= j1; i++) {
            lpL[1][i].copy_coeffs(lpL[1][0]);
            hpL[1][i].copy_coeffs(hpL[1][0]);
            lpR[1][i].copy_coeffs(lpL[1][0]);
            hpR[1][i].copy_coeffs(hpL[1][0]);
        }
        freq_old[1] = *params[param_freq1];
        sep_old[1]  = *params[param_sep1];
        q_old[1]    = *params[param_q1];
    }
    if(*params[param_freq2] != freq_old[2] or *params[param_sep2] != sep_old[2] or *params[param_q2] != q_old[2] or *params[param_mode] != mode_old) {
        lpL[2][0].set_lp_rbj((float)(*params[param_freq2] * (1 - *params[param_sep2])), *params[param_q2], (float)srate);
        hpL[2][0].set_hp_rbj((float)(*params[param_freq2] * (1 + *params[param_sep2])), *params[param_q2], (float)srate);
        lpR[2][0].copy_coeffs(lpL[2][0]);
        hpR[2][0].copy_coeffs(hpL[2][0]);
        for(i = 1; i <= j1; i++) {
            lpL[2][i].copy_coeffs(lpL[2][0]);
            hpL[2][i].copy_coeffs(hpL[2][0]);
            lpR[2][i].copy_coeffs(lpL[2][0]);
            hpR[2][i].copy_coeffs(hpL[2][0]);
        }
        freq_old[2] = *params[param_freq2];
        sep_old[2]  = *params[param_sep2];
        q_old[2]    = *params[param_q2];
    }
    // set the params of all strips
    strip[0].set_params(*params[param_attack0], *params[param_release0], *params[param_threshold0], *params[param_ratio0], *params[param_knee0], *params[param_makeup0], *params[param_detection0], 1.f, *params[param_bypass0], !(solo[0] || no_solo));
    strip[1].set_params(*params[param_attack1], *params[param_release1], *params[param_threshold1], *params[param_ratio1], *params[param_knee1], *params[param_makeup1], *params[param_detection1], 1.f, *params[param_bypass1], !(solo[1] || no_solo));
    strip[2].set_params(*params[param_attack2], *params[param_release2], *params[param_threshold2], *params[param_ratio2], *params[param_knee2], *params[param_makeup2], *params[param_detection2], 1.f, *params[param_bypass2], !(solo[2] || no_solo));
    strip[3].set_params(*params[param_attack3], *params[param_release3], *params[param_threshold3], *params[param_ratio3], *params[param_knee3], *params[param_makeup3], *params[param_detection3], 1.f, *params[param_bypass3], !(solo[3] || no_solo));
}

void multibandcompressor_audio_module::set_sample_rate(uint32_t sr)
{
    srate = sr;
    // set srate of all strips
    for (int j = 0; j < strips; j ++) {
        strip[j].set_sample_rate(srate);
    }
}

#define BYPASSED_COMPRESSION(index) \
    if(params[param_compression##index] != NULL) \
        *params[param_compression##index] = 1.0; \
    if(params[param_output##index] != NULL) \
        *params[param_output##index] = 0.0;

#define ACTIVE_COMPRESSION(index) \
    if(params[param_compression##index] != NULL) \
        *params[param_compression##index] = strip[index].get_comp_level(); \
    if(params[param_output##index] != NULL) \
        *params[param_output##index] = strip[index].get_output_level();

uint32_t multibandcompressor_audio_module::process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask)
{
    bool bypass = *params[param_bypass] > 0.5f;
    numsamples += offset;
    for (int i = 0; i < strips; i++)
        strip[i].update_curve();
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

        // let meters fall a bit
        clip_inL    -= std::min(clip_inL,  numsamples);
        clip_inR    -= std::min(clip_inR,  numsamples);
        clip_outL   -= std::min(clip_outL, numsamples);
        clip_outR   -= std::min(clip_outR, numsamples);
        meter_inL = 0.f;
        meter_inR = 0.f;
        meter_outL = 0.f;
        meter_outR = 0.f;
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
            int j1;
            for (int i = 0; i < strips; i ++) {
                // cycle trough strips
                if (solo[i] || no_solo) {
                    // strip unmuted
                    float left  = inL;
                    float right = inR;
                    // send trough filters
                    switch(mode) {
                        case 0:
                        default:
                            j1 = 0;
                            break;
                        case 1:
                            j1 = 2;
                            break;
                    }
                    for (int j = 0; j <= j1; j++){
                        if(i + 1 < strips) {
                            left  = lpL[i][j].process(left);
                            right = lpR[i][j].process(right);
                            lpL[i][j].sanitize();
                            lpR[i][j].sanitize();
                        }
                        if(i - 1 >= 0) {
                            left  = hpL[i - 1][j].process(left);
                            right = hpR[i - 1][j].process(right);
                            hpL[i - 1][j].sanitize();
                            hpR[i - 1][j].sanitize();
                        }
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
            switch(mode) {
                case 0:
                    outL *= 1.414213562;
                    outR *= 1.414213562;
                    break;
                case 1:
                    outL *= 0.88;
                    outR *= 0.88;
                    break;
            }

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
            // set up in / out meters
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
    SET_IF_CONNECTED(clip_inL);
    SET_IF_CONNECTED(clip_inR);
    SET_IF_CONNECTED(clip_outL);
    SET_IF_CONNECTED(clip_outR);
    SET_IF_CONNECTED(meter_inL);
    SET_IF_CONNECTED(meter_inR);
    SET_IF_CONNECTED(meter_outL);
    SET_IF_CONNECTED(meter_outR);
    // draw strip meters
    if(bypass > 0.5f) {
        BYPASSED_COMPRESSION(0)
        BYPASSED_COMPRESSION(1)
        BYPASSED_COMPRESSION(2)
        BYPASSED_COMPRESSION(3)
    } else {
        ACTIVE_COMPRESSION(0)
        ACTIVE_COMPRESSION(1)
        ACTIVE_COMPRESSION(2)
        ACTIVE_COMPRESSION(3)
    }
    // whatever has to be returned x)
    return outputs_mask;
}

const gain_reduction_audio_module *multibandcompressor_audio_module::get_strip_by_param_index(int index) const
{
    // let's handle by the corresponding strip
    switch (index) {
        case param_compression0:
            return &strip[0];
        case param_compression1:
            return &strip[1];
        case param_compression2:
            return &strip[2];
        case param_compression3:
            return &strip[3];
    }
    return NULL;
}

bool multibandcompressor_audio_module::get_graph(int index, int subindex, float *data, int points, cairo_iface *context) const
{
    const gain_reduction_audio_module *m = get_strip_by_param_index(index);
    if (m)
        return m->get_graph(subindex, data, points, context);
    return false;
}

bool multibandcompressor_audio_module::get_dot(int index, int subindex, float &x, float &y, int &size, cairo_iface *context) const
{
    const gain_reduction_audio_module *m = get_strip_by_param_index(index);
    if (m)
        return m->get_dot(subindex, x, y, size, context);
    return false;
}

bool multibandcompressor_audio_module::get_gridline(int index, int subindex, float &pos, bool &vertical, std::string &legend, cairo_iface *context) const
{
    const gain_reduction_audio_module *m = get_strip_by_param_index(index);
    if (m)
        return m->get_gridline(subindex, pos, vertical, legend, context);
    return false;
}

int multibandcompressor_audio_module::get_changed_offsets(int index, int generation, int &subindex_graph, int &subindex_dot, int &subindex_gridline) const
{
    const gain_reduction_audio_module *m = get_strip_by_param_index(index);
    if (m)
        return m->get_changed_offsets(generation, subindex_graph, subindex_dot, subindex_gridline);
    return 0;
}

/// Compressor originally by Thor
///
/// This module provides Thor's original compressor without any sidechain or weighting
///////////////////////////////////////////////////////////////////////////////////////////////

compressor_audio_module::compressor_audio_module()
{
    is_active = false;
    srate = 0;
    last_generation = 0;
    meters.reset();
}

void compressor_audio_module::activate()
{
    is_active = true;
    // set all filters and strips
    compressor.activate();
    params_changed();
    meters.reset();
}
void compressor_audio_module::deactivate()
{
    is_active = false;
    compressor.deactivate();
}

void compressor_audio_module::params_changed()
{
    compressor.set_params(*params[param_attack], *params[param_release], *params[param_threshold], *params[param_ratio], *params[param_knee], *params[param_makeup], *params[param_detection], *params[param_stereo_link], *params[param_bypass], 0.f);
}

void compressor_audio_module::set_sample_rate(uint32_t sr)
{
    srate = sr;
    compressor.set_sample_rate(srate);
    meters.set_sample_rate(srate);
}

uint32_t compressor_audio_module::process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask)
{
    uint32_t orig_offset = offset;
    uint32_t orig_numsamples = numsamples;
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
        meters.bypassed(params, orig_numsamples);
    } else {
        // process

        compressor.update_curve();

        while(offset < numsamples) {
            // cycle through samples
            float outL = 0.f;
            float outR = 0.f;
            float inL = ins[0][offset];
            float inR = ins[1][offset];
            // in level
            inR *= *params[param_level_in];
            inL *= *params[param_level_in];

            float leftAC = inL;
            float rightAC = inR;

            compressor.process(leftAC, rightAC);

            outL = leftAC;
            outR = rightAC;

            // send to output
            outs[0][offset] = outL;
            outs[1][offset] = outR;

            // next sample
            ++offset;
        } // cycle trough samples
        meters.process(params, ins, outs, orig_offset, orig_numsamples);
    }
    // draw strip meter
    if(bypass > 0.5f) {
        if(params[param_compression] != NULL) {
            *params[param_compression] = 1.0f;
        }
    } else {
        if(params[param_compression] != NULL) {
            *params[param_compression] = compressor.get_comp_level();
        }
    }
    // whatever has to be returned x)
    return outputs_mask;
}
bool compressor_audio_module::get_graph(int index, int subindex, float *data, int points, cairo_iface *context) const
{
    if (!is_active)
        return false;
    return compressor.get_graph(subindex, data, points, context);
}

bool compressor_audio_module::get_dot(int index, int subindex, float &x, float &y, int &size, cairo_iface *context) const
{
    if (!is_active)
        return false;
    return compressor.get_dot(subindex, x, y, size, context);
}

bool compressor_audio_module::get_gridline(int index, int subindex, float &pos, bool &vertical, std::string &legend, cairo_iface *context) const
{
    if (!is_active)
        return false;
    return compressor.get_gridline(subindex, pos, vertical, legend, context);
}

int compressor_audio_module::get_changed_offsets(int index, int generation, int &subindex_graph, int &subindex_dot, int &subindex_gridline) const
{
    if (!is_active)
        return false;
    return compressor.get_changed_offsets(generation, subindex_graph, subindex_dot, subindex_gridline);
}

/// Sidecain Compressor by Markus Schmidt
///
/// This module splits the signal in a sidechain- and a process signal.
/// The sidechain is processed through Krzystofs filters and compresses
/// the process signal via Thor's compression routine afterwards.
///////////////////////////////////////////////////////////////////////////////////////////////

sidechaincompressor_audio_module::sidechaincompressor_audio_module()
{
    is_active = false;
    srate = 0;
    last_generation = 0;
    f1_freq_old1  = 0.f;
    f2_freq_old1  = 0.f;
    f1_level_old1 = 0.f;
    f2_level_old1 = 0.f;
    f1_freq_old  = 0.f;
    f2_freq_old  = 0.f;
    f1_level_old = 0.f;
    f2_level_old = 0.f;
    sc_mode_old1  = WIDEBAND;
    meters.reset();
}

void sidechaincompressor_audio_module::activate()
{
    is_active = true;
    // set all filters and strips
    compressor.activate();
    params_changed();
    meters.reset();
}
void sidechaincompressor_audio_module::deactivate()
{
    is_active = false;
    compressor.deactivate();
}

sidechaincompressor_audio_module::cfloat sidechaincompressor_audio_module::h_z(const cfloat &z) const
{
    switch ((CalfScModes)sc_mode) {
        default:
        case WIDEBAND:
            return false;
            break;
        case DEESSER_WIDE:
        case DERUMBLER_WIDE:
        case WEIGHTED_1:
        case WEIGHTED_2:
        case WEIGHTED_3:
        case BANDPASS_2:
            return f1L.h_z(z) * f2L.h_z(z);
            break;
        case DEESSER_SPLIT:
            return f2L.h_z(z);
            break;
        case DERUMBLER_SPLIT:
        case BANDPASS_1:
            return f1L.h_z(z);
            break;
    }
}

float sidechaincompressor_audio_module::freq_gain(int index, double freq, uint32_t sr) const
{
    typedef std::complex<double> cfloat;
    freq *= 2.0 * M_PI / sr;
    cfloat z = 1.0 / exp(cfloat(0.0, freq));

    return std::abs(h_z(z));
}

void sidechaincompressor_audio_module::params_changed()
{
    // set the params of all filters
    if(*params[param_f1_freq] != f1_freq_old or *params[param_f1_level] != f1_level_old
        or *params[param_f2_freq] != f2_freq_old or *params[param_f2_level] != f2_level_old
        or *params[param_sc_mode] != sc_mode) {
        float q = 0.707;
        switch ((CalfScModes)*params[param_sc_mode]) {
            default:
            case WIDEBAND:
                f1L.set_hp_rbj((float)*params[param_f1_freq], q, (float)srate, *params[param_f1_level]);
                f1R.copy_coeffs(f1L);
                f2L.set_lp_rbj((float)*params[param_f2_freq], q, (float)srate, *params[param_f2_level]);
                f2R.copy_coeffs(f2L);
                f1_active = 0.f;
                f2_active = 0.f;
                break;
            case DEESSER_WIDE:
                f1L.set_peakeq_rbj((float)*params[param_f1_freq], q, *params[param_f1_level], (float)srate);
                f1R.copy_coeffs(f1L);
                f2L.set_hp_rbj((float)*params[param_f2_freq], q, (float)srate, *params[param_f2_level]);
                f2R.copy_coeffs(f2L);
                f1_active = 0.5f;
                f2_active = 1.f;
                break;
            case DEESSER_SPLIT:
                f1L.set_lp_rbj((float)*params[param_f2_freq] * (1 + 0.17), q, (float)srate);
                f1R.copy_coeffs(f1L);
                f2L.set_hp_rbj((float)*params[param_f2_freq] * (1 - 0.17), q, (float)srate, *params[param_f2_level]);
                f2R.copy_coeffs(f2L);
                f1_active = 0.f;
                f2_active = 1.f;
                break;
            case DERUMBLER_WIDE:
                f1L.set_lp_rbj((float)*params[param_f1_freq], q, (float)srate, *params[param_f1_level]);
                f1R.copy_coeffs(f1L);
                f2L.set_peakeq_rbj((float)*params[param_f2_freq], q, *params[param_f2_level], (float)srate);
                f2R.copy_coeffs(f2L);
                f1_active = 1.f;
                f2_active = 0.5f;
                break;
            case DERUMBLER_SPLIT:
                f1L.set_lp_rbj((float)*params[param_f1_freq] * (1 + 0.17), q, (float)srate, *params[param_f1_level]);
                f1R.copy_coeffs(f1L);
                f2L.set_hp_rbj((float)*params[param_f1_freq] * (1 - 0.17), q, (float)srate);
                f2R.copy_coeffs(f2L);
                f1_active = 1.f;
                f2_active = 0.f;
                break;
            case WEIGHTED_1:
                f1L.set_lowshelf_rbj((float)*params[param_f1_freq], q, *params[param_f1_level], (float)srate);
                f1R.copy_coeffs(f1L);
                f2L.set_highshelf_rbj((float)*params[param_f2_freq], q, *params[param_f2_level], (float)srate);
                f2R.copy_coeffs(f2L);
                f1_active = 0.5f;
                f2_active = 0.5f;
                break;
            case WEIGHTED_2:
                f1L.set_lowshelf_rbj((float)*params[param_f1_freq], q, *params[param_f1_level], (float)srate);
                f1R.copy_coeffs(f1L);
                f2L.set_peakeq_rbj((float)*params[param_f2_freq], q, *params[param_f2_level], (float)srate);
                f2R.copy_coeffs(f2L);
                f1_active = 0.5f;
                f2_active = 0.5f;
                break;
            case WEIGHTED_3:
                f1L.set_peakeq_rbj((float)*params[param_f1_freq], q, *params[param_f1_level], (float)srate);
                f1R.copy_coeffs(f1L);
                f2L.set_highshelf_rbj((float)*params[param_f2_freq], q, *params[param_f2_level], (float)srate);
                f2R.copy_coeffs(f2L);
                f1_active = 0.5f;
                f2_active = 0.5f;
                break;
            case BANDPASS_1:
                f1L.set_bp_rbj((float)*params[param_f1_freq], q, (float)srate, *params[param_f1_level]);
                f1R.copy_coeffs(f1L);
                f2L.set_hp_rbj((float)*params[param_f2_freq], q, *params[param_f2_level], (float)srate);
                f2R.copy_coeffs(f2L);
                f1_active = 1.f;
                f2_active = 0.f;
                break;
            case BANDPASS_2:
                f1L.set_hp_rbj((float)*params[param_f1_freq], q, (float)srate, *params[param_f1_level]);
                f1R.copy_coeffs(f1L);
                f2L.set_lp_rbj((float)*params[param_f2_freq], q, (float)srate, *params[param_f2_level]);
                f2R.copy_coeffs(f2L);
                f1_active = 1.f;
                f2_active = 1.f;
                break;
        }
        f1_freq_old = *params[param_f1_freq];
        f1_level_old = *params[param_f1_level];
        f2_freq_old = *params[param_f2_freq];
        f2_level_old = *params[param_f2_level];
        sc_mode = (CalfScModes)*params[param_sc_mode];
    }
    // light LED's
    if(params[param_f1_active] != NULL) {
        *params[param_f1_active] = f1_active;
    }
    if(params[param_f2_active] != NULL) {
        *params[param_f2_active] = f2_active;
    }
    // and set the compressor module
    compressor.set_params(*params[param_attack], *params[param_release], *params[param_threshold], *params[param_ratio], *params[param_knee], *params[param_makeup], *params[param_detection], *params[param_stereo_link], *params[param_bypass], 0.f);
}

void sidechaincompressor_audio_module::set_sample_rate(uint32_t sr)
{
    srate = sr;
    compressor.set_sample_rate(srate);
    meters.set_sample_rate(srate);
}

uint32_t sidechaincompressor_audio_module::process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask)
{
    uint32_t orig_offset = offset;
    uint32_t orig_numsamples = numsamples;
    bool bypass = *params[param_bypass] > 0.5f;
    numsamples += offset;
    if(bypass) {
        // everything bypassed
        while(offset < numsamples) {
            switch ((CalfScRoute)*params[param_sc_route]) {
                case STEREO:
                    outs[0][offset] = ins[0][offset];
                    outs[1][offset] = ins[1][offset];
                    break;
                case RIGHT_LEFT:
                    outs[0][offset] = ins[0][offset];
                    outs[1][offset] = ins[0][offset];
                    break;
                case LEFT_RIGHT:
                    outs[0][offset] = ins[1][offset];
                    outs[1][offset] = ins[1][offset];
                    break;
            }
            ++offset;
        }
        // displays, too
        meters.bypassed(params, orig_numsamples);
    } else {
        // process

        compressor.update_curve();

        while(offset < numsamples) {
            // cycle through samples
            float outL = 0.f;
            float outR = 0.f;
            float inL = ins[0][offset];
            float inR = ins[1][offset];
            // in level
            inR *= *params[param_level_in];
            inL *= *params[param_level_in];

            float leftAC = inL;
            float rightAC = inR;
            float leftSC = inL;
            float rightSC = inR;
            float leftMC = inL;
            float rightMC = inR;
            
            switch ((CalfScRoute)*params[param_sc_route]) {
                case STEREO:
                    leftAC = inL;
                    rightAC = inR;
                    leftSC = inL;
                    rightSC = inR;
                    leftMC = inL;
                    rightMC = inR;
                    break;
                case RIGHT_LEFT:
                    leftAC = inL;
                    rightAC = inL;
                    leftSC = inR;
                    rightSC = inR;
                    leftMC = inL;
                    rightMC = inL;
                    break;
                case LEFT_RIGHT:
                    leftAC = inR;
                    rightAC = inR;
                    leftSC = inL;
                    rightSC = inL;
                    leftMC = inR;
                    rightMC = inR;
                    break;
            }
            
            leftSC  *= *params[param_sc_level];
            rightSC *= *params[param_sc_level];
            
            switch ((CalfScModes)*params[param_sc_mode]) {
                default:
                case WIDEBAND:
                    compressor.process(leftAC, rightAC, &leftSC, &rightSC);
                    leftMC = leftSC;
                    rightMC = rightSC;
                    break;
                case DEESSER_WIDE:
                case DERUMBLER_WIDE:
                case WEIGHTED_1:
                case WEIGHTED_2:
                case WEIGHTED_3:
                case BANDPASS_2:
                    leftSC = f2L.process(f1L.process(leftSC));
                    rightSC = f2R.process(f1R.process(rightSC));
                    leftMC = leftSC;
                    rightMC = rightSC;
                    compressor.process(leftAC, rightAC, &leftSC, &rightSC);
                    break;
                case DEESSER_SPLIT:
                    leftSC = f2L.process(leftSC);
                    rightSC = f2R.process(rightSC);
                    leftMC = leftSC;
                    rightMC = rightSC;
                    compressor.process(leftSC, rightSC, &leftSC, &rightSC);
                    leftAC = f1L.process(leftAC);
                    rightAC = f1R.process(rightAC);
                    leftAC += leftSC;
                    rightAC += rightSC;
                    break;
                case DERUMBLER_SPLIT:
                    leftSC = f1L.process(leftSC);
                    rightSC = f1R.process(rightSC);
                    leftMC = leftSC;
                    rightMC = rightSC;
                    compressor.process(leftSC, rightSC, &leftSC, &rightSC);
                    leftAC = f2L.process(leftAC);
                    rightAC = f2R.process(rightAC);
                    leftAC += leftSC;
                    rightAC += rightSC;
                    break;
                case BANDPASS_1:
                    leftSC = f1L.process(leftSC);
                    rightSC = f1R.process(rightSC);
                    leftMC = leftSC;
                    rightMC = rightSC;
                    compressor.process(leftAC, rightAC, &leftSC, &rightSC);
                    break;
            }

            if(*params[param_sc_listen] > 0.f) {
                outL = leftMC;
                outR = rightMC;
            } else {
                outL = leftAC;
                outR = rightAC;
            }

            // send to output
            outs[0][offset] = outL;
            outs[1][offset] = outR;

            // next sample
            ++offset;
        } // cycle trough samples
        meters.process(params, ins, outs, orig_offset, orig_numsamples);
        f1L.sanitize();
        f1R.sanitize();
        f2L.sanitize();
        f2R.sanitize();
    }
    // draw strip meter
    if(bypass > 0.5f) {
        if(params[param_compression] != NULL) {
            *params[param_compression] = 1.0f;
        }
    } else {
        if(params[param_compression] != NULL) {
            *params[param_compression] = compressor.get_comp_level();
        }
    }
    // whatever has to be returned x)
    return outputs_mask;
}
bool sidechaincompressor_audio_module::get_graph(int index, int subindex, float *data, int points, cairo_iface *context) const
{
    if (!is_active)
        return false;
    if (index == param_f1_freq && !subindex) {
        context->set_line_width(1.5);
        return ::get_graph(*this, subindex, data, points);
    } else if(index == param_compression) {
        return compressor.get_graph(subindex, data, points, context);
    }
    return false;
}

bool sidechaincompressor_audio_module::get_dot(int index, int subindex, float &x, float &y, int &size, cairo_iface *context) const
{
    if (!is_active)
        return false;
    if (index == param_compression) {
        return compressor.get_dot(subindex, x, y, size, context);
    }
    return false;
}

bool sidechaincompressor_audio_module::get_gridline(int index, int subindex, float &pos, bool &vertical, std::string &legend, cairo_iface *context) const
{
    if (!is_active)
        return false;
    if (index == param_compression) {
        return compressor.get_gridline(subindex, pos, vertical, legend, context);
    } else {
        return get_freq_gridline(subindex, pos, vertical, legend, context);
    }
//    return false;
}

int sidechaincompressor_audio_module::get_changed_offsets(int index, int generation, int &subindex_graph, int &subindex_dot, int &subindex_gridline) const
{
    if (!is_active)
        return false;
    if(index == param_compression) {
        return compressor.get_changed_offsets(generation, subindex_graph, subindex_dot, subindex_gridline);
    } else {
        //  (fabs(inertia_cutoff.get_last() - old_cutoff) + 100 * fabs(inertia_resonance.get_last() - old_resonance) + fabs(*params[par_mode] - old_mode) > 0.1f)
        if (*params[param_f1_freq] != f1_freq_old1
            or *params[param_f2_freq] != f2_freq_old1
            or *params[param_f1_level] != f1_level_old1
            or *params[param_f2_level] != f2_level_old1
            or *params[param_sc_mode] !=sc_mode_old1)
        {
            f1_freq_old1 = *params[param_f1_freq];
            f2_freq_old1 = *params[param_f2_freq];
            f1_level_old1 = *params[param_f1_level];
            f2_level_old1 = *params[param_f2_level];
            sc_mode_old1 = (CalfScModes)*params[param_sc_mode];
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

/// Deesser by Markus Schmidt
///
/// This module splits the signal in a sidechain- and a process signal.
/// The sidechain is processed through Krzystofs filters and compresses
/// the process signal via Thor's compression routine afterwards.
///////////////////////////////////////////////////////////////////////////////////////////////

deesser_audio_module::deesser_audio_module()
{
    is_active = false;
    srate = 0;
    last_generation = 0;
    f1_freq_old1  = 0.f;
    f2_freq_old1  = 0.f;
    f1_level_old1 = 0.f;
    f2_level_old1 = 0.f;
    f2_q_old1     = 0.f;
    f1_freq_old  = 0.f;
    f2_freq_old  = 0.f;
    f1_level_old = 0.f;
    f2_level_old = 0.f;
    f2_q_old     = 0.f;
    detected_led = 0;
    clip_led     = 0;
}

void deesser_audio_module::activate()
{
    is_active = true;
    // set all filters and strips
    compressor.activate();
    params_changed();
    detected = 0.f;
    detected_led = 0.f;
    clip_out = 0.f;
}
void deesser_audio_module::deactivate()
{
    is_active = false;
    compressor.deactivate();
}

void deesser_audio_module::params_changed()
{
    // set the params of all filters
    if(*params[param_f1_freq] != f1_freq_old or *params[param_f1_level] != f1_level_old
        or *params[param_f2_freq] != f2_freq_old or *params[param_f2_level] != f2_level_old
        or *params[param_f2_q] != f2_q_old) {
        float q = 0.707;

        hpL.set_hp_rbj((float)*params[param_f1_freq] * (1 - 0.17), q, (float)srate, *params[param_f1_level]);
        hpR.copy_coeffs(hpL);
        lpL.set_lp_rbj((float)*params[param_f1_freq] * (1 + 0.17), q, (float)srate);
        lpR.copy_coeffs(lpL);
        pL.set_peakeq_rbj((float)*params[param_f2_freq], *params[param_f2_q], *params[param_f2_level], (float)srate);
        pR.copy_coeffs(pL);
        f1_freq_old = *params[param_f1_freq];
        f1_level_old = *params[param_f1_level];
        f2_freq_old = *params[param_f2_freq];
        f2_level_old = *params[param_f2_level];
        f2_q_old = *params[param_f2_q];
    }
    // and set the compressor module
    compressor.set_params((float)*params[param_laxity], (float)*params[param_laxity] * 1.33, *params[param_threshold], *params[param_ratio], 2.8, *params[param_makeup], *params[param_detection], 0.f, *params[param_bypass], 0.f);
}

void deesser_audio_module::set_sample_rate(uint32_t sr)
{
    srate = sr;
    compressor.set_sample_rate(srate);
}

uint32_t deesser_audio_module::process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask)
{
    bool bypass = *params[param_bypass] > 0.5f;
    numsamples += offset;
    if(bypass) {
        // everything bypassed81e8da266
        while(offset < numsamples) {
            outs[0][offset] = ins[0][offset];
            outs[1][offset] = ins[1][offset];
            ++offset;
        }
        // displays, too
        clip_out   = 0.f;
        detected = 0.f;
        detected_led = 0.f;
    } else {
        // process

        detected_led -= std::min(detected_led,  numsamples);
        clip_led   -= std::min(clip_led,  numsamples);
        compressor.update_curve();

        while(offset < numsamples) {
            // cycle through samples
            float outL = 0.f;
            float outR = 0.f;
            float inL = ins[0][offset];
            float inR = ins[1][offset];


            float leftAC = inL;
            float rightAC = inR;
            float leftSC = inL;
            float rightSC = inR;
            float leftRC = inL;
            float rightRC = inR;
            float leftMC = inL;
            float rightMC = inR;

            leftSC = pL.process(hpL.process(leftSC));
            rightSC = pR.process(hpR.process(rightSC));
            leftMC = leftSC;
            rightMC = rightSC;

            switch ((int)*params[param_mode]) {
                default:
                case WIDE:
                    compressor.process(leftAC, rightAC, &leftSC, &rightSC);
                    break;
                case SPLIT:
                    hpL.sanitize();
                    hpR.sanitize();
                    leftRC = hpL.process(leftRC);
                    rightRC = hpR.process(rightRC);
                    compressor.process(leftRC, rightRC, &leftSC, &rightSC);
                    leftAC = lpL.process(leftAC);
                    rightAC = lpR.process(rightAC);
                    leftAC += leftRC;
                    rightAC += rightRC;
                    break;
            }

            if(*params[param_sc_listen] > 0.f) {
                outL = leftMC;
                outR = rightMC;
            } else {
                outL = leftAC;
                outR = rightAC;
            }

            // send to output
            outs[0][offset] = outL;
            outs[1][offset] = outR;

            if(std::max(fabs(leftSC), fabs(rightSC)) > *params[param_threshold]) {
                detected_led   = srate >> 3;
            }
            if(std::max(fabs(leftAC), fabs(rightAC)) > 1.f) {
                clip_led   = srate >> 3;
            }
            if(clip_led > 0) {
                clip_out = 1.f;
            } else {
                clip_out = std::max(fabs(outL), fabs(outR));
            }
            detected = std::max(fabs(leftMC), fabs(rightMC));

            // next sample
            ++offset;
        } // cycle trough samples
        hpL.sanitize();
        hpR.sanitize();
        lpL.sanitize();
        lpR.sanitize();
        pL.sanitize();
        pR.sanitize();
    }
    // draw meters
    if(params[param_detected_led] != NULL) {
        *params[param_detected_led] = detected_led;
    }
    if(params[param_clip_out] != NULL) {
        *params[param_clip_out] = clip_out;
    }
    if(params[param_detected] != NULL) {
        *params[param_detected] = detected;
    }
    // draw strip meter
    if(bypass > 0.5f) {
        if(params[param_compression] != NULL) {
            *params[param_compression] = 1.0f;
        }
    } else {
        if(params[param_compression] != NULL) {
            *params[param_compression] = compressor.get_comp_level();
        }
    }
    // whatever has to be returned x)
    return outputs_mask;
}
bool deesser_audio_module::get_graph(int index, int subindex, float *data, int points, cairo_iface *context) const
{
    if (!is_active)
        return false;
    if (index == param_f1_freq && !subindex) {
        context->set_line_width(1.5);
        return ::get_graph(*this, subindex, data, points);
    }
    return false;
}

bool deesser_audio_module::get_gridline(int index, int subindex, float &pos, bool &vertical, std::string &legend, cairo_iface *context) const
{
    return get_freq_gridline(subindex, pos, vertical, legend, context);

//    return false;
}

int deesser_audio_module::get_changed_offsets(int index, int generation, int &subindex_graph, int &subindex_dot, int &subindex_gridline) const
{
    if (!is_active) {
        return false;
    } else {
        //  (fabs(inertia_cutoff.get_last() - old_cutoff) + 100 * fabs(inertia_resonance.get_last() - old_resonance) + fabs(*params[par_mode] - old_mode) > 0.1f)
        if (*params[param_f1_freq] != f1_freq_old1
            or *params[param_f2_freq] != f2_freq_old1
            or *params[param_f1_level] != f1_level_old1
            or *params[param_f2_level] != f2_level_old1
            or *params[param_f2_q] !=f2_q_old1)
        {
            f1_freq_old1 = *params[param_f1_freq];
            f2_freq_old1 = *params[param_f2_freq];
            f1_level_old1 = *params[param_f1_level];
            f2_level_old1 = *params[param_f2_level];
            f2_q_old1 = *params[param_f2_q];
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

/// Gate originally by Damien
///
/// This module provides Damien's original expander based on Thor's compressor
/// without any weighting
///////////////////////////////////////////////////////////////////////////////////////////////

gate_audio_module::gate_audio_module()
{
    is_active = false;
    srate = 0;
    last_generation = 0;
    meters.reset();
}

void gate_audio_module::activate()
{
    is_active = true;
    // set all filters and strips
    gate.activate();
    params_changed();
    meters.reset();
}
void gate_audio_module::deactivate()
{
    is_active = false;
    gate.deactivate();
}

void gate_audio_module::params_changed()
{
    gate.set_params(*params[param_attack], *params[param_release], *params[param_threshold], *params[param_ratio], *params[param_knee], *params[param_makeup], *params[param_detection], *params[param_stereo_link], *params[param_bypass], 0.f, *params[param_range]);
}

void gate_audio_module::set_sample_rate(uint32_t sr)
{
    srate = sr;
    gate.set_sample_rate(srate);
    meters.set_sample_rate(srate);
}

uint32_t gate_audio_module::process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask)
{
    uint32_t orig_offset = offset;
    uint32_t orig_numsamples = numsamples;
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
        meters.bypassed(params, orig_numsamples);
    } else {
        // process
        gate.update_curve();

        while(offset < numsamples) {
            // cycle through samples
            float outL = 0.f;
            float outR = 0.f;
            float inL = ins[0][offset];
            float inR = ins[1][offset];
            // in level
            inR *= *params[param_level_in];
            inL *= *params[param_level_in];

            float leftAC = inL;
            float rightAC = inR;

            gate.process(leftAC, rightAC);

            outL = leftAC;
            outR = rightAC;

            // send to output
            outs[0][offset] = outL;
            outs[1][offset] = outR;

            // next sample
            ++offset;
        } // cycle trough samples
        meters.process(params, ins, outs, orig_offset, orig_numsamples);
    }
    // draw strip meter
    if(bypass > 0.5f) {
        if(params[param_gating] != NULL) {
            *params[param_gating] = 1.0f;
        }
    } else {
        if(params[param_gating] != NULL) {
            *params[param_gating] = gate.get_expander_level();
        }
    }
    // whatever has to be returned x)
    return outputs_mask;
}
bool gate_audio_module::get_graph(int index, int subindex, float *data, int points, cairo_iface *context) const
{
    if (!is_active)
        return false;
    return gate.get_graph(subindex, data, points, context);
}

bool gate_audio_module::get_dot(int index, int subindex, float &x, float &y, int &size, cairo_iface *context) const
{
    if (!is_active)
        return false;
    return gate.get_dot(subindex, x, y, size, context);
}

bool gate_audio_module::get_gridline(int index, int subindex, float &pos, bool &vertical, std::string &legend, cairo_iface *context) const
{
    if (!is_active)
        return false;
    return gate.get_gridline(subindex, pos, vertical, legend, context);
}

int gate_audio_module::get_changed_offsets(int index, int generation, int &subindex_graph, int &subindex_dot, int &subindex_gridline) const
{
    if (!is_active)
        return false;
    return gate.get_changed_offsets(generation, subindex_graph, subindex_dot, subindex_gridline);
}

/// Sidecain Gate by Markus Schmidt
///
/// This module splits the signal in a sidechain- and a process signal.
/// The sidechain is processed through Krzystofs filters and gates
/// the process signal via Damiens's gating routine afterwards.
///////////////////////////////////////////////////////////////////////////////////////////////

sidechaingate_audio_module::sidechaingate_audio_module()
{
    is_active = false;
    srate = 0;
    last_generation = 0;

    f1_freq_old = f2_freq_old = f1_level_old = f2_level_old = 0;
    f1_freq_old1 = f2_freq_old1 = f1_level_old1 = f2_level_old1 = 0;
    sc_mode_old = sc_mode_old1 = WIDEBAND; // doesn't matter as long as it's sane
    meters.reset();
}

void sidechaingate_audio_module::activate()
{
    is_active = true;
    // set all filters and strips
    gate.activate();
    params_changed();
    meters.reset();
}
void sidechaingate_audio_module::deactivate()
{
    is_active = false;
    gate.deactivate();
}

sidechaingate_audio_module::cfloat sidechaingate_audio_module::h_z(const cfloat &z) const
{
    switch ((CalfScModes)sc_mode) {
        default:
        case WIDEBAND:
            return false;
            break;
        case HIGHGATE_WIDE:
        case LOWGATE_WIDE:
        case WEIGHTED_1:
        case WEIGHTED_2:
        case WEIGHTED_3:
        case BANDPASS_2:
            return f1L.h_z(z) * f2L.h_z(z);
            break;
        case HIGHGATE_SPLIT:
            return f2L.h_z(z);
            break;
        case LOWGATE_SPLIT:
        case BANDPASS_1:
            return f1L.h_z(z);
            break;
    }
}

float sidechaingate_audio_module::freq_gain(int index, double freq, uint32_t sr) const
{
    typedef std::complex<double> cfloat;
    freq *= 2.0 * M_PI / sr;
    cfloat z = 1.0 / exp(cfloat(0.0, freq));

    return std::abs(h_z(z));
}

void sidechaingate_audio_module::params_changed()
{
    // set the params of all filters
    if(*params[param_f1_freq] != f1_freq_old or *params[param_f1_level] != f1_level_old
        or *params[param_f2_freq] != f2_freq_old or *params[param_f2_level] != f2_level_old
        or *params[param_sc_mode] != sc_mode) {
        float q = 0.707;
        switch ((CalfScModes)*params[param_sc_mode]) {
            default:
            case WIDEBAND:
                f1L.set_hp_rbj((float)*params[param_f1_freq], q, (float)srate, *params[param_f1_level]);
                f1R.copy_coeffs(f1L);
                f2L.set_lp_rbj((float)*params[param_f2_freq], q, (float)srate, *params[param_f2_level]);
                f2R.copy_coeffs(f2L);
                f1_active = 0.f;
                f2_active = 0.f;
                break;
            case HIGHGATE_WIDE:
                f1L.set_peakeq_rbj((float)*params[param_f1_freq], q, *params[param_f1_level], (float)srate);
                f1R.copy_coeffs(f1L);
                f2L.set_hp_rbj((float)*params[param_f2_freq], q, (float)srate, *params[param_f2_level]);
                f2R.copy_coeffs(f2L);
                f1_active = 0.5f;
                f2_active = 1.f;
                break;
            case HIGHGATE_SPLIT:
                f1L.set_lp_rbj((float)*params[param_f2_freq] * (1 + 0.17), q, (float)srate);
                f1R.copy_coeffs(f1L);
                f2L.set_hp_rbj((float)*params[param_f2_freq] * (1 - 0.17), q, (float)srate, *params[param_f2_level]);
                f2R.copy_coeffs(f2L);
                f1_active = 0.f;
                f2_active = 1.f;
                break;
            case LOWGATE_WIDE:
                f1L.set_lp_rbj((float)*params[param_f1_freq], q, (float)srate, *params[param_f1_level]);
                f1R.copy_coeffs(f1L);
                f2L.set_peakeq_rbj((float)*params[param_f2_freq], q, *params[param_f2_level], (float)srate);
                f2R.copy_coeffs(f2L);
                f1_active = 1.f;
                f2_active = 0.5f;
                break;
            case LOWGATE_SPLIT:
                f1L.set_lp_rbj((float)*params[param_f1_freq] * (1 + 0.17), q, (float)srate, *params[param_f1_level]);
                f1R.copy_coeffs(f1L);
                f2L.set_hp_rbj((float)*params[param_f1_freq] * (1 - 0.17), q, (float)srate);
                f2R.copy_coeffs(f2L);
                f1_active = 1.f;
                f2_active = 0.f;
                break;
            case WEIGHTED_1:
                f1L.set_lowshelf_rbj((float)*params[param_f1_freq], q, *params[param_f1_level], (float)srate);
                f1R.copy_coeffs(f1L);
                f2L.set_highshelf_rbj((float)*params[param_f2_freq], q, *params[param_f2_level], (float)srate);
                f2R.copy_coeffs(f2L);
                f1_active = 0.5f;
                f2_active = 0.5f;
                break;
            case WEIGHTED_2:
                f1L.set_lowshelf_rbj((float)*params[param_f1_freq], q, *params[param_f1_level], (float)srate);
                f1R.copy_coeffs(f1L);
                f2L.set_peakeq_rbj((float)*params[param_f2_freq], q, *params[param_f2_level], (float)srate);
                f2R.copy_coeffs(f2L);
                f1_active = 0.5f;
                f2_active = 0.5f;
                break;
            case WEIGHTED_3:
                f1L.set_peakeq_rbj((float)*params[param_f1_freq], q, *params[param_f1_level], (float)srate);
                f1R.copy_coeffs(f1L);
                f2L.set_highshelf_rbj((float)*params[param_f2_freq], q, *params[param_f2_level], (float)srate);
                f2R.copy_coeffs(f2L);
                f1_active = 0.5f;
                f2_active = 0.5f;
                break;
            case BANDPASS_1:
                f1L.set_bp_rbj((float)*params[param_f1_freq], q, (float)srate, *params[param_f1_level]);
                f1R.copy_coeffs(f1L);
                f2L.set_hp_rbj((float)*params[param_f2_freq], q, *params[param_f2_level], (float)srate);
                f2R.copy_coeffs(f2L);
                f1_active = 1.f;
                f2_active = 0.f;
                break;
            case BANDPASS_2:
                f1L.set_hp_rbj((float)*params[param_f1_freq], q, (float)srate, *params[param_f1_level]);
                f1R.copy_coeffs(f1L);
                f2L.set_lp_rbj((float)*params[param_f2_freq], q, (float)srate, *params[param_f2_level]);
                f2R.copy_coeffs(f2L);
                f1_active = 1.f;
                f2_active = 1.f;
                break;
        }
        f1_freq_old = *params[param_f1_freq];
        f1_level_old = *params[param_f1_level];
        f2_freq_old = *params[param_f2_freq];
        f2_level_old = *params[param_f2_level];
        sc_mode = (CalfScModes)*params[param_sc_mode];
    }
    // light LED's
    if(params[param_f1_active] != NULL) {
        *params[param_f1_active] = f1_active;
    }
    if(params[param_f2_active] != NULL) {
        *params[param_f2_active] = f2_active;
    }
    // and set the expander module
    gate.set_params(*params[param_attack], *params[param_release], *params[param_threshold], *params[param_ratio], *params[param_knee], *params[param_makeup], *params[param_detection], *params[param_stereo_link], *params[param_bypass], 0.f, *params[param_range]);
}

void sidechaingate_audio_module::set_sample_rate(uint32_t sr)
{
    srate = sr;
    gate.set_sample_rate(srate);
    meters.set_sample_rate(srate);
}

uint32_t sidechaingate_audio_module::process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask)
{
    uint32_t orig_offset = offset;
    uint32_t orig_numsamples = numsamples;
    bool bypass = *params[param_bypass] > 0.5f;
    numsamples += offset;
    if(bypass) {
        // everything bypassed
        while(offset < numsamples) {
            switch ((CalfScRoute)*params[param_sc_route]) {
                case STEREO:
                    outs[0][offset] = ins[0][offset];
                    outs[1][offset] = ins[1][offset];
                    break;
                case RIGHT_LEFT:
                    outs[0][offset] = ins[0][offset];
                    outs[1][offset] = ins[0][offset];
                    break;
                case LEFT_RIGHT:
                    outs[0][offset] = ins[1][offset];
                    outs[1][offset] = ins[1][offset];
                    break;
            }
            outs[0][offset] = ins[0][offset];
            outs[1][offset] = ins[1][offset];
            ++offset;
        }
        // displays, too
        meters.bypassed(params, orig_offset);
    } else {
        // process

        gate.update_curve();

        while(offset < numsamples) {
            // cycle through samples
            float outL = 0.f;
            float outR = 0.f;
            float inL = ins[0][offset];
            float inR = ins[1][offset];
            // in level
            inR *= *params[param_level_in];
            inL *= *params[param_level_in];
            
            float leftAC = inL;
            float rightAC = inR;
            float leftSC = inL;
            float rightSC = inR;
            float leftMC = inL;
            float rightMC = inR;
            
            switch ((CalfScRoute)*params[param_sc_route]) {
                case STEREO:
                    leftAC = inL;
                    rightAC = inR;
                    leftSC = inL;
                    rightSC = inR;
                    leftMC = inL;
                    rightMC = inR;
                    break;
                case RIGHT_LEFT:
                    leftAC = inL;
                    rightAC = inL;
                    leftSC = inR;
                    rightSC = inR;
                    leftMC = inL;
                    rightMC = inL;
                    break;
                case LEFT_RIGHT:
                    leftAC = inR;
                    rightAC = inR;
                    leftSC = inL;
                    rightSC = inL;
                    leftMC = inR;
                    rightMC = inR;
                    break;
            }
            
            leftSC  *= *params[param_sc_level];
            rightSC *= *params[param_sc_level];
            
            switch ((CalfScModes)*params[param_sc_mode]) {
                default:
                case WIDEBAND:
                    gate.process(leftAC, rightAC, &leftSC, &rightSC);
                    leftMC = leftSC;
                    rightMC = rightSC;
                    break;
                case HIGHGATE_WIDE:
                case LOWGATE_WIDE:
                case WEIGHTED_1:
                case WEIGHTED_2:
                case WEIGHTED_3:
                case BANDPASS_2:
                    leftSC = f2L.process(f1L.process(leftSC));
                    rightSC = f2R.process(f1R.process(rightSC));
                    leftMC = leftSC;
                    rightMC = rightSC;
                    gate.process(leftAC, rightAC, &leftSC, &rightSC);
                    break;
                case HIGHGATE_SPLIT:
                    leftSC = f2L.process(leftSC);
                    rightSC = f2R.process(rightSC);
                    leftMC = leftSC;
                    rightMC = rightSC;
                    gate.process(leftSC, rightSC, &leftSC, &rightSC);
                    leftAC = f1L.process(leftAC);
                    rightAC = f1R.process(rightAC);
                    leftAC += leftSC;
                    rightAC += rightSC;
                    break;
                case LOWGATE_SPLIT:
                    leftSC = f1L.process(leftSC);
                    rightSC = f1R.process(rightSC);
                    leftMC = leftSC;
                    rightMC = rightSC;
                    gate.process(leftSC, rightSC, &leftSC, &rightSC);
                    leftAC = f2L.process(leftAC);
                    rightAC = f2R.process(rightAC);
                    leftAC += leftSC;
                    rightAC += rightSC;
                    break;
                case BANDPASS_1:
                    leftSC = f1L.process(leftSC);
                    rightSC = f1R.process(rightSC);
                    leftMC = leftSC;
                    rightMC = rightSC;
                    gate.process(leftAC, rightAC, &leftSC, &rightSC);
                    break;
            }

            if(*params[param_sc_listen] > 0.f) {
                outL = leftMC;
                outR = rightMC;
            } else {
                outL = leftAC;
                outR = rightAC;
            }

            // send to output
            outs[0][offset] = outL;
            outs[1][offset] = outR;

            // next sample
            ++offset;
        } // cycle trough samples
        meters.process(params, ins, outs, orig_offset, orig_numsamples);
        f1L.sanitize();
        f1R.sanitize();
        f2L.sanitize();
        f2R.sanitize();

    }
    // draw strip meter
    if(bypass > 0.5f) {
        if(params[param_gating] != NULL) {
            *params[param_gating] = 1.0f;
        }
    } else {
        if(params[param_gating] != NULL) {
            *params[param_gating] = gate.get_expander_level();
        }
    }
    // whatever has to be returned x)
    return outputs_mask;
}
bool sidechaingate_audio_module::get_graph(int index, int subindex, float *data, int points, cairo_iface *context) const
{
    if (!is_active)
        return false;
    if (index == param_f1_freq && !subindex) {
        context->set_line_width(1.5);
        return ::get_graph(*this, subindex, data, points);
    } else if(index == param_gating) {
        return gate.get_graph(subindex, data, points, context);
    }
    return false;
}

bool sidechaingate_audio_module::get_dot(int index, int subindex, float &x, float &y, int &size, cairo_iface *context) const
{
    if (!is_active)
        return false;
    if (index == param_gating) {
        return gate.get_dot(subindex, x, y, size, context);
    }
    return false;
}

bool sidechaingate_audio_module::get_gridline(int index, int subindex, float &pos, bool &vertical, std::string &legend, cairo_iface *context) const
{
    if (!is_active)
        return false;
    if (index == param_gating) {
        return gate.get_gridline(subindex, pos, vertical, legend, context);
    } else {
        return get_freq_gridline(subindex, pos, vertical, legend, context);
    }
//    return false;
}

int sidechaingate_audio_module::get_changed_offsets(int index, int generation, int &subindex_graph, int &subindex_dot, int &subindex_gridline) const
{
    if (!is_active)
        return false;
    if(index == param_gating) {
        return gate.get_changed_offsets(generation, subindex_graph, subindex_dot, subindex_gridline);
    } else {
        //  (fabs(inertia_cutoff.get_last() - old_cutoff) + 100 * fabs(inertia_resonance.get_last() - old_resonance) + fabs(*params[par_mode] - old_mode) > 0.1f)
        if (*params[param_f1_freq] != f1_freq_old1
            or *params[param_f2_freq] != f2_freq_old1
            or *params[param_f1_level] != f1_level_old1
            or *params[param_f2_level] != f2_level_old1
            or *params[param_sc_mode] !=sc_mode_old1)
        {
            f1_freq_old1 = *params[param_f1_freq];
            f2_freq_old1 = *params[param_f2_freq];
            f1_level_old1 = *params[param_f1_level];
            f2_level_old1 = *params[param_f2_level];
            sc_mode_old1 = (CalfScModes)*params[param_sc_mode];
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


/// Multiband Compressor by Markus Schmidt
///
/// This module splits the signal in four different bands
/// and sends them through multiple filters (implemented by
/// Krzysztof). They are processed by a compressing routine
/// (implemented by Thor) afterwards and summed up to the
/// final output again.
///////////////////////////////////////////////////////////////////////////////////////////////

multibandgate_audio_module::multibandgate_audio_module()
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
    for(int i = 0; i < strips - 1; i ++) {
        freq_old[i] = -1;
        sep_old[i] = -1;
        q_old[i] = -1;
    }
    mode_old = -1;
}

void multibandgate_audio_module::activate()
{
    is_active = true;
    // set all filters and strips
    params_changed();
    // activate all strips
    for (int j = 0; j < strips; j ++) {
        gate[j].activate();
        gate[j].id = j;
    }
}

void multibandgate_audio_module::deactivate()
{
    is_active = false;
    // deactivate all strips
    for (int j = 0; j < strips; j ++) {
        gate[j].deactivate();
    }
}

void multibandgate_audio_module::params_changed()
{
    // determine mute/solo states
    solo[0] = *params[param_solo0] > 0.f ? true : false;
    solo[1] = *params[param_solo1] > 0.f ? true : false;
    solo[2] = *params[param_solo2] > 0.f ? true : false;
    solo[3] = *params[param_solo3] > 0.f ? true : false;
    no_solo = (*params[param_solo0] > 0.f ||
            *params[param_solo1] > 0.f ||
            *params[param_solo2] > 0.f ||
            *params[param_solo3] > 0.f) ? false : true;
    int i;
    int j1;
    switch(mode) {
        case 0:
        default:
            j1 = 0;
            break;
        case 1:
            j1 = 2;
            break;
    }
    // set the params of all filters
    if(*params[param_freq0] != freq_old[0] or *params[param_sep0] != sep_old[0] or *params[param_q0] != q_old[0] or *params[param_mode] != mode_old) {
        lpL[0][0].set_lp_rbj((float)(*params[param_freq0] * (1 - *params[param_sep0])), *params[param_q0], (float)srate);
        hpL[0][0].set_hp_rbj((float)(*params[param_freq0] * (1 + *params[param_sep0])), *params[param_q0], (float)srate);
        lpR[0][0].copy_coeffs(lpL[0][0]);
        hpR[0][0].copy_coeffs(hpL[0][0]);
        for(i = 1; i <= j1; i++) {
            lpL[0][i].copy_coeffs(lpL[0][0]);
            hpL[0][i].copy_coeffs(hpL[0][0]);
            lpR[0][i].copy_coeffs(lpL[0][0]);
            hpR[0][i].copy_coeffs(hpL[0][0]);
        }
        freq_old[0] = *params[param_freq0];
        sep_old[0]  = *params[param_sep0];
        q_old[0]    = *params[param_q0];
    }
    if(*params[param_freq1] != freq_old[1] or *params[param_sep1] != sep_old[1] or *params[param_q1] != q_old[1] or *params[param_mode] != mode_old) {
        lpL[1][0].set_lp_rbj((float)(*params[param_freq1] * (1 - *params[param_sep1])), *params[param_q1], (float)srate);
        hpL[1][0].set_hp_rbj((float)(*params[param_freq1] * (1 + *params[param_sep1])), *params[param_q1], (float)srate);
        lpR[1][0].copy_coeffs(lpL[1][0]);
        hpR[1][0].copy_coeffs(hpL[1][0]);
        for(i = 1; i <= j1; i++) {
            lpL[1][i].copy_coeffs(lpL[1][0]);
            hpL[1][i].copy_coeffs(hpL[1][0]);
            lpR[1][i].copy_coeffs(lpL[1][0]);
            hpR[1][i].copy_coeffs(hpL[1][0]);
        }
        freq_old[1] = *params[param_freq1];
        sep_old[1]  = *params[param_sep1];
        q_old[1]    = *params[param_q1];
    }
    if(*params[param_freq2] != freq_old[2] or *params[param_sep2] != sep_old[2] or *params[param_q2] != q_old[2] or *params[param_mode] != mode_old) {
        lpL[2][0].set_lp_rbj((float)(*params[param_freq2] * (1 - *params[param_sep2])), *params[param_q2], (float)srate);
        hpL[2][0].set_hp_rbj((float)(*params[param_freq2] * (1 + *params[param_sep2])), *params[param_q2], (float)srate);
        lpR[2][0].copy_coeffs(lpL[2][0]);
        hpR[2][0].copy_coeffs(hpL[2][0]);
        for(i = 1; i <= j1; i++) {
            lpL[2][i].copy_coeffs(lpL[2][0]);
            hpL[2][i].copy_coeffs(hpL[2][0]);
            lpR[2][i].copy_coeffs(lpL[2][0]);
            hpR[2][i].copy_coeffs(hpL[2][0]);
        }
        freq_old[2] = *params[param_freq2];
        sep_old[2]  = *params[param_sep2];
        q_old[2]    = *params[param_q2];
    }
    // set the params of all strips
    gate[0].set_params(*params[param_attack0], *params[param_release0], *params[param_threshold0], *params[param_ratio0], *params[param_knee0], *params[param_makeup0], *params[param_detection0], 1.f, *params[param_bypass0], !(solo[0] || no_solo), *params[param_range0]);
    gate[1].set_params(*params[param_attack1], *params[param_release1], *params[param_threshold1], *params[param_ratio1], *params[param_knee1], *params[param_makeup1], *params[param_detection1], 1.f, *params[param_bypass1], !(solo[1] || no_solo), *params[param_range1]);
    gate[2].set_params(*params[param_attack2], *params[param_release2], *params[param_threshold2], *params[param_ratio2], *params[param_knee2], *params[param_makeup2], *params[param_detection2], 1.f, *params[param_bypass2], !(solo[2] || no_solo), *params[param_range2]);
    gate[3].set_params(*params[param_attack3], *params[param_release3], *params[param_threshold3], *params[param_ratio3], *params[param_knee3], *params[param_makeup3], *params[param_detection3], 1.f, *params[param_bypass3], !(solo[3] || no_solo), *params[param_range3]);
}

void multibandgate_audio_module::set_sample_rate(uint32_t sr)
{
    srate = sr;
    // set srate of all strips
    for (int j = 0; j < strips; j ++) {
        gate[j].set_sample_rate(srate);
    }
}

#define BYPASSED_GATING(index) \
    if(params[param_gating##index] != NULL) \
        *params[param_gating##index] = 1.0; \
    if(params[param_output##index] != NULL) \
        *params[param_output##index] = 0.0;

#define ACTIVE_GATING(index) \
    if(params[param_gating##index] != NULL) \
        *params[param_gating##index] = gate[index].get_expander_level(); \
    if(params[param_output##index] != NULL) \
        *params[param_output##index] = gate[index].get_output_level();

uint32_t multibandgate_audio_module::process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask)
{
    bool bypass = *params[param_bypass] > 0.5f;
    numsamples += offset;
    for (int i = 0; i < strips; i++)
        gate[i].update_curve();
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

        // let meters fall a bit
        clip_inL    -= std::min(clip_inL,  numsamples);
        clip_inR    -= std::min(clip_inR,  numsamples);
        clip_outL   -= std::min(clip_outL, numsamples);
        clip_outR   -= std::min(clip_outR, numsamples);
        meter_inL = 0.f;
        meter_inR = 0.f;
        meter_outL = 0.f;
        meter_outR = 0.f;
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
            int j1;
            for (int i = 0; i < strips; i ++) {
                // cycle trough strips
                if (solo[i] || no_solo) {
                    // strip unmuted
                    float left  = inL;
                    float right = inR;
                    // send trough filters
                    switch(mode) {
                        case 0:
                        default:
                            j1 = 0;
                            break;
                        case 1:
                            j1 = 2;
                            break;
                    }
                    for (int j = 0; j <= j1; j++){
                        if(i + 1 < strips) {
                            left  = lpL[i][j].process(left);
                            right = lpR[i][j].process(right);
                            lpL[i][j].sanitize();
                            lpR[i][j].sanitize();
                        }
                        if(i - 1 >= 0) {
                            left  = hpL[i - 1][j].process(left);
                            right = hpR[i - 1][j].process(right);
                            hpL[i - 1][j].sanitize();
                            hpR[i - 1][j].sanitize();
                        }
                    }
                    // process gain reduction
                    gate[i].process(left, right);
                    // sum up output
                    outL += left;
                    outR += right;
                } else {
                    // strip muted

                }


            } // process single strip

            // even out filters gain reduction
            // 3dB - levelled manually (based on default sep and q settings)
            switch(mode) {
                case 0:
                    outL *= 1.414213562;
                    outR *= 1.414213562;
                    break;
                case 1:
                    outL *= 0.88;
                    outR *= 0.88;
                    break;
            }

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
            // set up in / out meters
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
    SET_IF_CONNECTED(clip_inL);
    SET_IF_CONNECTED(clip_inR);
    SET_IF_CONNECTED(clip_outL);
    SET_IF_CONNECTED(clip_outR);
    SET_IF_CONNECTED(meter_inL);
    SET_IF_CONNECTED(meter_inR);
    SET_IF_CONNECTED(meter_outL);
    SET_IF_CONNECTED(meter_outR);
    // draw strip meters
    if(bypass > 0.5f) {
        BYPASSED_GATING(0)
        BYPASSED_GATING(1)
        BYPASSED_GATING(2)
        BYPASSED_GATING(3)
    } else {
        ACTIVE_GATING(0)
        ACTIVE_GATING(1)
        ACTIVE_GATING(2)
        ACTIVE_GATING(3)
    }
    // whatever has to be returned x)
    return outputs_mask;
}

const expander_audio_module *multibandgate_audio_module::get_strip_by_param_index(int index) const
{
    // let's handle by the corresponding strip
    switch (index) {
        case param_gating0:
            return &gate[0];
        case param_gating1:
            return &gate[1];
        case param_gating2:
            return &gate[2];
        case param_gating3:
            return &gate[3];
    }
    return NULL;
}

bool multibandgate_audio_module::get_graph(int index, int subindex, float *data, int points, cairo_iface *context) const
{
    const expander_audio_module *m = get_strip_by_param_index(index);
    if (m)
        return m->get_graph(subindex, data, points, context);
    return false;
}

bool multibandgate_audio_module::get_dot(int index, int subindex, float &x, float &y, int &size, cairo_iface *context) const
{
    const expander_audio_module *m = get_strip_by_param_index(index);
    if (m)
        return m->get_dot(subindex, x, y, size, context);
    return false;
}

bool multibandgate_audio_module::get_gridline(int index, int subindex, float &pos, bool &vertical, std::string &legend, cairo_iface *context) const
{
    const expander_audio_module *m = get_strip_by_param_index(index);
    if (m)
        return m->get_gridline(subindex, pos, vertical, legend, context);
    return false;
}

int multibandgate_audio_module::get_changed_offsets(int index, int generation, int &subindex_graph, int &subindex_dot, int &subindex_gridline) const
{
    const expander_audio_module *m = get_strip_by_param_index(index);
    if (m)
        return m->get_changed_offsets(generation, subindex_graph, subindex_dot, subindex_gridline);
    return 0;
}


/// Gain reduction module by Thor
/// All functions of this module are originally written
/// by Thor, while some features have been stripped (mainly stereo linking
/// and frequency correction as implemented in Sidechain Compressor above)
/// To save some CPU.
////////////////////////////////////////////////////////////////////////////////
gain_reduction_audio_module::gain_reduction_audio_module()
{
    is_active       = false;
    srate           = 0;
    last_generation = 0;
    old_threshold   = 0.f;
    old_ratio       = 0.f;
    old_knee        = 0.f;
    old_makeup      = 0.f;
    old_detection   = 0.f;
    old_bypass      = 0.f;
    old_mute        = 0.f;
    linSlope        = 0.f;
    attack          = 0.f;
    release         = 0.f;
    detection       = -1;
    stereo_link     = -1;
    threshold       = -1;
    ratio           = -1;
    knee            = -1;
    makeup          = -1;
    bypass          = -1;
    mute            = -1;
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
    process(l, r, 0, 0);
    bypass = byp;
}

void gain_reduction_audio_module::deactivate()
{
    is_active = false;
}

void gain_reduction_audio_module::update_curve()
{
    float linThreshold = threshold;
    float linKneeSqrt = sqrt(knee);
    linKneeStart = linThreshold / linKneeSqrt;
    adjKneeStart = linKneeStart*linKneeStart;
    float linKneeStop = linThreshold * linKneeSqrt;
    thres = log(linThreshold);
    kneeStart = log(linKneeStart);
    kneeStop = log(linKneeStop);
    compressedKneeStop = (kneeStop - thres) / ratio + thres;
}

void gain_reduction_audio_module::process(float &left, float &right, const float *det_left, const float *det_right)
{
    if(!det_left) {
        det_left = &left;
    }
    if(!det_right) {
        det_right = &right;
    }
    if(bypass < 0.5f) {
        // this routine is mainly copied from thor's compressor module
        // greatest sounding compressor I've heard!
        bool rms = (detection == 0);
        bool average = (stereo_link == 0);
        float attack_coeff = std::min(1.f, 1.f / (attack * srate / 4000.f));
        float release_coeff = std::min(1.f, 1.f / (release * srate / 4000.f));

        float absample = average ? (fabs(*det_left) + fabs(*det_right)) * 0.5f : std::max(fabs(*det_left), fabs(*det_right));
        if(rms) absample *= absample;

        dsp::sanitize(linSlope);

        linSlope += (absample - linSlope) * (absample > linSlope ? attack_coeff : release_coeff);
        float gain = 1.f;
        if(linSlope > 0.f) {
            gain = output_gain(linSlope, rms);
        }

        left *= gain * makeup;
        right *= gain * makeup;
        meter_out = std::max(fabs(left), fabs(right));;
        meter_comp = gain;
        detected = rms ? sqrt(linSlope) : linSlope;
    }
}

float gain_reduction_audio_module::output_level(float slope) const {
    return slope * output_gain(slope, false) * makeup;
}

float gain_reduction_audio_module::output_gain(float linSlope, bool rms) const {
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
void gain_reduction_audio_module::set_params(float att, float rel, float thr, float rat, float kn, float mak, float det, float stl, float byp, float mu)
{
    // set all params
    attack          = att;
    release         = rel;
    threshold       = thr;
    ratio           = rat;
    knee            = kn;
    makeup          = mak;
    detection       = det;
    stereo_link     = stl;
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

bool gain_reduction_audio_module::get_graph(int subindex, float *data, int points, cairo_iface *context) const
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

bool gain_reduction_audio_module::get_dot(int subindex, float &x, float &y, int &size, cairo_iface *context) const
{
    if (!is_active)
        return false;
    if (!subindex)
    {
        if(bypass > 0.5f or mute > 0.f) {
            return false;
        } else {
            bool rms = (detection == 0);
            float det = rms ? sqrt(detected) : detected;
            x = 0.5 + 0.5 * dB_grid(det);
            y = dB_grid(bypass > 0.5f or mute > 0.f ? det : output_level(det));
            return true;
        }
    }
    return false;
}

bool gain_reduction_audio_module::get_gridline(int subindex, float &pos, bool &vertical, std::string &legend, cairo_iface *context) const
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

int gain_reduction_audio_module::get_changed_offsets(int generation, int &subindex_graph, int &subindex_dot, int &subindex_gridline) const
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


/// Gate module by Damien
/// All functions of this module are originally written
/// by Damien, while some features have been stripped (mainly stereo linking
/// and frequency correction as implemented in Sidechain Gate above)
/// To save some CPU.
////////////////////////////////////////////////////////////////////////////////
expander_audio_module::expander_audio_module()
{
    is_active       = false;
    srate           = 0;
    last_generation = 0;
    range     = -1.f;
    threshold = -1.f;
    ratio     = -1.f;
    knee      = -1.f;
    makeup    = -1.f;
    detection = -1.f;
    bypass    = -1.f;
    mute      = -1.f;
    stereo_link = -1.f;
    old_range     = 0.f;
    old_threshold = 0.f;
    old_ratio     = 0.f;
    old_knee      = 0.f;
    old_makeup    = 0.f;
    old_detection = 0.f;
    old_bypass    = 0.f;
    old_mute      = 0.f;
    old_trigger   = 0.f;
    old_stereo_link = 0.f;
    linSlope      = -1;
    linKneeStop   = 0;
}

void expander_audio_module::activate()
{
    is_active = true;
    linSlope   = 0.f;
    meter_out  = 0.f;
    meter_gate = 1.f;
    float l, r;
    l = r = 0.f;
    float byp = bypass;
    bypass = 0.0;
    process(l, r);
    bypass = byp;
}

void expander_audio_module::deactivate()
{
    is_active = false;
}

void expander_audio_module::update_curve()
{
    bool rms = (detection == 0);
    float linThreshold = threshold;
    if (rms)
        linThreshold = linThreshold * linThreshold;
    attack_coeff = std::min(1.f, 1.f / (attack * srate / 4000.f));
    release_coeff = std::min(1.f, 1.f / (release * srate / 4000.f));
    float linKneeSqrt = sqrt(knee);
    linKneeStart = linThreshold / linKneeSqrt;
    adjKneeStart = linKneeStart*linKneeStart;
    linKneeStop = linThreshold * linKneeSqrt;
    thres = log(linThreshold);
    kneeStart = log(linKneeStart);
    kneeStop = log(linKneeStop);
    compressedKneeStop = (kneeStop - thres) / ratio + thres;
}

void expander_audio_module::process(float &left, float &right, const float *det_left, const float *det_right)
{
    if(!det_left) {
        det_left = &left;
    }
    if(!det_right) {
        det_right = &right;
    }
    if(bypass < 0.5f) {
        // this routine is mainly copied from Damien's expander module based on Thor's compressor
        bool rms = (detection == 0);
        bool average = (stereo_link == 0);
        float absample = average ? (fabs(*det_left) + fabs(*det_right)) * 0.5f : std::max(fabs(*det_left), fabs(*det_right));
        if(rms) absample *= absample;

        dsp::sanitize(linSlope);

        linSlope += (absample - linSlope) * (absample > linSlope ? attack_coeff : release_coeff);
        float gain = 1.f;
        if(linSlope > 0.f) {
            gain = output_gain(linSlope, rms);
        }
        left *= gain * makeup;
        right *= gain * makeup;
        meter_out = std::max(fabs(left), fabs(right));
        meter_gate = gain;
        detected = linSlope;
    }
}

float expander_audio_module::output_level(float slope) const {
    bool rms = (detection == 0);
    return slope * output_gain(rms ? slope*slope : slope, rms) * makeup;
}

float expander_audio_module::output_gain(float linSlope, bool rms) const {
    //this calculation is also Damiens's work based on Thor's compressor
    if(linSlope < linKneeStop) {
        float slope = log(linSlope);
        //float tratio = rms ? sqrt(ratio) : ratio;
        float tratio = ratio;
        float gain = 0.f;
        float delta = 0.f;
        if(IS_FAKE_INFINITY(ratio))
            tratio = 1000.f;
        gain = (slope-thres) * tratio + thres;
        delta = tratio;

        if(knee > 1.f && slope > kneeStart ) {
            gain = dsp::hermite_interpolation(slope, kneeStart, kneeStop, ((kneeStart - thres) * tratio  + thres), kneeStop, delta,1.f);
        }
        return std::max(range, expf(gain-slope));
    }
    return 1.f;
}

void expander_audio_module::set_sample_rate(uint32_t sr)
{
    srate = sr;
}
void expander_audio_module::set_params(float att, float rel, float thr, float rat, float kn, float mak, float det, float stl, float byp, float mu, float ran)
{
    // set all params
    attack          = att;
    release         = rel;
    threshold       = thr;
    ratio           = rat;
    knee            = kn;
    makeup          = mak;
    detection       = det;
    stereo_link     = stl;
    bypass          = byp;
    mute            = mu;
    range           = ran;
    if(mute > 0.f) {
        meter_out  = 0.f;
        meter_gate = 1.f;
    }
}
float expander_audio_module::get_output_level() {
    // returns output level (max(left, right))
    return meter_out;
}
float expander_audio_module::get_expander_level() {
    // returns amount of gating
    return meter_gate;
}

bool expander_audio_module::get_graph(int subindex, float *data, int points, cairo_iface *context) const
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

bool expander_audio_module::get_dot(int subindex, float &x, float &y, int &size, cairo_iface *context) const
{
    if (!is_active)
        return false;
    if (!subindex)
    {
        if(bypass > 0.5f or mute > 0.f) {
            return false;
        } else {
            bool rms = (detection == 0);
            float det = rms ? sqrt(detected) : detected;
            x = 0.5 + 0.5 * dB_grid(det);
            y = dB_grid(bypass > 0.5f or mute > 0.f ? det : output_level(det));
            return true;
        }
    }
    return false;
}

bool expander_audio_module::get_gridline(int subindex, float &pos, bool &vertical, std::string &legend, cairo_iface *context) const
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

int expander_audio_module::get_changed_offsets(int generation, int &subindex_graph, int &subindex_dot, int &subindex_gridline) const
{
    subindex_graph = 0;
    subindex_dot = 0;
    subindex_gridline = generation ? INT_MAX : 0;

    if (fabs(range-old_range) + fabs(threshold-old_threshold) + fabs(ratio - old_ratio) + fabs(knee - old_knee) + fabs(makeup - old_makeup) + fabs(detection - old_detection) + fabs(bypass - old_bypass) + fabs(mute - old_mute) > 0.000001f)
    {
        old_range     = range;
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
