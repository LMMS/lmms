/* Calf DSP plugin pack
 * Distortion related plugins
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
#include <calf/modules_dist.h>

using namespace dsp;
using namespace calf_plugins;

/// Saturator Band by Markus Schmidt
///
/// This module is based on Krzysztof's filters and Tom Szilagyi's distortion routine.
/// It provides a blendable saturation stage followed by a highpass, a lowpass and a peak filter
///////////////////////////////////////////////////////////////////////////////////////////////

saturator_audio_module::saturator_audio_module()
{
    is_active = false;
    srate = 0;
    meter_drive = 0.f;
    lp_pre_freq_old = -1;
    hp_pre_freq_old = -1;
    lp_post_freq_old = -1;
    hp_post_freq_old = -1;
    p_freq_old = -1;
    p_level_old = -1;
}

void saturator_audio_module::activate()
{
    is_active = true;
    // set all filters
    params_changed();
    meters.reset();
    meter_drive = 0.f;
}
void saturator_audio_module::deactivate()
{
    is_active = false;
}

void saturator_audio_module::params_changed()
{
    // set the params of all filters
    if(*params[param_lp_pre_freq] != lp_pre_freq_old) {
        lp[0][0].set_lp_rbj(*params[param_lp_pre_freq], 0.707, (float)srate);
        if(in_count > 1 && out_count > 1)
            lp[1][0].copy_coeffs(lp[0][0]);
        lp[0][1].copy_coeffs(lp[0][0]);
        if(in_count > 1 && out_count > 1)
            lp[1][1].copy_coeffs(lp[0][0]);
        lp_pre_freq_old = *params[param_lp_pre_freq];
    }
    if(*params[param_hp_pre_freq] != hp_pre_freq_old) {
        hp[0][0].set_hp_rbj(*params[param_hp_pre_freq], 0.707, (float)srate);
        if(in_count > 1 && out_count > 1)
            hp[1][0].copy_coeffs(hp[0][0]);
        hp[0][1].copy_coeffs(hp[0][0]);
        if(in_count > 1 && out_count > 1)
            hp[1][1].copy_coeffs(hp[0][0]);
        hp_pre_freq_old = *params[param_hp_pre_freq];
    }
    if(*params[param_lp_post_freq] != lp_post_freq_old) {
        lp[0][2].set_lp_rbj(*params[param_lp_post_freq], 0.707, (float)srate);
        if(in_count > 1 && out_count > 1)
            lp[1][2].copy_coeffs(lp[0][2]);
        lp[0][3].copy_coeffs(lp[0][2]);
        if(in_count > 1 && out_count > 1)
            lp[1][3].copy_coeffs(lp[0][2]);
        lp_post_freq_old = *params[param_lp_post_freq];
    }
    if(*params[param_hp_post_freq] != hp_post_freq_old) {
        hp[0][2].set_hp_rbj(*params[param_hp_post_freq], 0.707, (float)srate);
        if(in_count > 1 && out_count > 1)
            hp[1][2].copy_coeffs(hp[0][2]);
        hp[0][3].copy_coeffs(hp[0][2]);
        if(in_count > 1 && out_count > 1)
            hp[1][3].copy_coeffs(hp[0][2]);
        hp_post_freq_old = *params[param_hp_post_freq];
    }
    if(*params[param_p_freq] != p_freq_old or *params[param_p_level] != p_level_old or *params[param_p_q] != p_q_old) {
        p[0].set_peakeq_rbj((float)*params[param_p_freq], (float)*params[param_p_q], (float)*params[param_p_level], (float)srate);
        if(in_count > 1 && out_count > 1)
            p[1].copy_coeffs(p[0]);
        p_freq_old = *params[param_p_freq];
        p_level_old = *params[param_p_level];
        p_q_old = *params[param_p_q];
    }
    // set distortion
    dist[0].set_params(*params[param_blend], *params[param_drive]);
    if(in_count > 1 && out_count > 1)
        dist[1].set_params(*params[param_blend], *params[param_drive]);
}

void saturator_audio_module::set_sample_rate(uint32_t sr)
{
    srate = sr;
    dist[0].set_sample_rate(sr);
    if(in_count > 1 && out_count > 1)
        dist[1].set_sample_rate(sr);
    meters.set_sample_rate(srate);
}

uint32_t saturator_audio_module::process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask)
{
    bool bypass = *params[param_bypass] > 0.5f;
    uint32_t orig_offset = offset;
    uint32_t orig_numsamples = numsamples;
    numsamples += offset;
    if(bypass) {
        // everything bypassed
        while(offset < numsamples) {
            if(in_count > 1 && out_count > 1) {
                outs[0][offset] = ins[0][offset];
                outs[1][offset] = ins[1][offset];
            } else if(in_count > 1) {
                outs[0][offset] = (ins[0][offset] + ins[1][offset]) / 2;
            } else if(out_count > 1) {
                outs[0][offset] = ins[0][offset];
                outs[1][offset] = ins[0][offset];
            } else {
                outs[0][offset] = ins[0][offset];
            }
            ++offset;
        }
        meters.bypassed(params, orig_numsamples);
    } else {
        meter_drive = 0.f;
        float in_avg[2] = {0.f, 0.f};
        float out_avg[2] = {0.f, 0.f};
        float tube_avg = 0.f;
        // process
        while(offset < numsamples) {
            // cycle through samples
            float out[2], in[2] = {0.f, 0.f};
            int c = 0;
            
            if(in_count > 1 && out_count > 1) {
                // stereo in/stereo out
                // handle full stereo
                in[0] = ins[0][offset];
                in[1] = ins[1][offset];
                c = 2;
            } else {
                // in and/or out mono
                // handle mono
                in[0] = ins[0][offset];
                in[1] = in[0];
                c = 1;
            }
            
            float proc[2];
            proc[0] = in[0] * *params[param_level_in];
            proc[1] = in[1] * *params[param_level_in];
            
            float onedivlevelin = 1.0 / *params[param_level_in];
            
            for (int i = 0; i < c; ++i) {
                // all pre filters in chain
                proc[i] = lp[i][1].process(lp[i][0].process(proc[i]));
                proc[i] = hp[i][1].process(hp[i][0].process(proc[i]));
                
                // get average for display purposes before...
                in_avg[i] += fabs(pow(proc[i], 2.f));

                // ...saturate...
                proc[i] = dist[i].process(proc[i]);
                
                // ...and get average after...
                out_avg[i] += fabs(pow(proc[i], 2.f));
                
                // tone control
                proc[i] = p[i].process(proc[i]);

                // all post filters in chain
                proc[i] = lp[i][2].process(lp[i][3].process(proc[i]));
                proc[i] = hp[i][2].process(hp[i][3].process(proc[i]));
                
                //subtract gain
                proc[i] *= onedivlevelin;
            }
            
            if(in_count > 1 && out_count > 1) {
                // full stereo
                out[0] = ((proc[0] * *params[param_mix]) + in[0] * (1 - *params[param_mix])) * *params[param_level_out];
                outs[0][offset] = out[0];
                out[1] = ((proc[1] * *params[param_mix]) + in[1] * (1 - *params[param_mix])) * *params[param_level_out];
                outs[1][offset] = out[1];
            } else if(out_count > 1) {
                // mono -> pseudo stereo
                out[0] = ((proc[0] * *params[param_mix]) + in[0] * (1 - *params[param_mix])) * *params[param_level_out];
                outs[0][offset] = out[0];
                out[1] = out[0];
                outs[1][offset] = out[1];
            } else {
                // stereo -> mono
                // or full mono
                out[0] = ((proc[0] * *params[param_mix]) + in[0] * (1 - *params[param_mix])) * *params[param_level_out];
                outs[0][offset] = out[0];
            }
                        
            // next sample
            ++offset;
        } // cycle trough samples
        meters.process(params, ins, outs, orig_offset, orig_numsamples);
        
        tube_avg = (sqrt(std::max(out_avg[0], out_avg[1])) / numsamples) - (sqrt(std::max(in_avg[0], in_avg[1])) / numsamples);
        meter_drive = (5.0f * fabs(tube_avg) * (float(*params[param_blend]) + 30.0f));
        // printf("out:%.6f in: %.6f avg: %.6f drv: %.3f\n", sqrt(std::max(out_avg[0], out_avg[1])) / numsamples, sqrt(std::max(in_avg[0], in_avg[1])) / numsamples, tube_avg, meter_drive);
        // clean up
        lp[0][0].sanitize();
        lp[1][0].sanitize();
        lp[0][1].sanitize();
        lp[1][1].sanitize();
        lp[0][2].sanitize();
        lp[1][2].sanitize();
        lp[0][3].sanitize();
        lp[1][3].sanitize();
        hp[0][0].sanitize();
        hp[1][0].sanitize();
        hp[0][1].sanitize();
        hp[1][1].sanitize();
        hp[0][2].sanitize();
        hp[1][2].sanitize();
        hp[0][3].sanitize();
        hp[1][3].sanitize();
        p[0].sanitize();
        p[1].sanitize();
    }
    // draw meters
    if(params[param_meter_drive] != NULL) {
        *params[param_meter_drive] = meter_drive;
    }
    // whatever has to be returned x)
    return outputs_mask;
}

/// Exciter by Markus Schmidt
///
/// This module is based on Krzysztof's filters and Tom Szilagyi's distortion routine.
/// It provides a blendable saturation stage followed by a highpass, a lowpass and a peak filter
///////////////////////////////////////////////////////////////////////////////////////////////

exciter_audio_module::exciter_audio_module()
{
    is_active = false;
    srate = 0;
    meter_drive = 0.f;
}

void exciter_audio_module::activate()
{
    is_active = true;
    // set all filters
    params_changed();
}

void exciter_audio_module::deactivate()
{
    is_active = false;
}

void exciter_audio_module::params_changed()
{
    // set the params of all filters
    if(*params[param_freq] != freq_old) {
        hp[0][0].set_hp_rbj(*params[param_freq], 0.707, (float)srate);
        hp[0][1].copy_coeffs(hp[0][0]);
        hp[0][2].copy_coeffs(hp[0][0]);
        hp[0][3].copy_coeffs(hp[0][0]);
        if(in_count > 1 && out_count > 1) {
            hp[1][0].copy_coeffs(hp[0][0]);
            hp[1][1].copy_coeffs(hp[0][0]);
            hp[1][2].copy_coeffs(hp[0][0]);
            hp[1][3].copy_coeffs(hp[0][0]);
        }
        freq_old = *params[param_freq];
    }
    // set the params of all filters
    if(*params[param_ceil] != ceil_old or *params[param_ceil_active] != ceil_active_old) {
        lp[0][0].set_lp_rbj(*params[param_ceil], 0.707, (float)srate);
        lp[0][1].copy_coeffs(lp[0][0]);
        lp[1][0].copy_coeffs(lp[0][0]);
        lp[1][1].copy_coeffs(lp[0][0]);
        ceil_old = *params[param_ceil];
        ceil_active_old = *params[param_ceil_active];
    }
    // set distortion
    dist[0].set_params(*params[param_blend], *params[param_drive]);
    if(in_count > 1 && out_count > 1)
        dist[1].set_params(*params[param_blend], *params[param_drive]);
}

void exciter_audio_module::set_sample_rate(uint32_t sr)
{
    srate = sr;
    dist[0].set_sample_rate(sr);
    if(in_count > 1 && out_count > 1)
        dist[1].set_sample_rate(sr);
    meters.set_sample_rate(srate);
}

uint32_t exciter_audio_module::process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask)
{
    uint32_t orig_offset = offset;
    uint32_t orig_numsamples = numsamples;
    bool bypass = *params[param_bypass] > 0.5f;
    numsamples += offset;
    if(bypass) {
        // everything bypassed
        while(offset < numsamples) {
            if(in_count > 1 && out_count > 1) {
                outs[0][offset] = ins[0][offset];
                outs[1][offset] = ins[1][offset];
            } else if(in_count > 1) {
                outs[0][offset] = (ins[0][offset] + ins[1][offset]) / 2;
            } else if(out_count > 1) {
                outs[0][offset] = ins[0][offset];
                outs[1][offset] = ins[0][offset];
            } else {
                outs[0][offset] = ins[0][offset];
            }
            ++offset;
        }
        meters.bypassed(params, orig_numsamples);
        // displays, too
        meter_drive = 0.f;
    } else {
        
        meter_drive = 0.f;
        
        float in2out = *params[param_listen] > 0.f ? 0.f : 1.f;
        
        // process
        while(offset < numsamples) {
            // cycle through samples
            float out[2], in[2] = {0.f, 0.f};
            float maxDrive = 0.f;
            int c = 0;
            
            if(in_count > 1 && out_count > 1) {
                // stereo in/stereo out
                // handle full stereo
                in[0] = ins[0][offset];
                in[0] *= *params[param_level_in];
                in[1] = ins[1][offset];
                in[1] *= *params[param_level_in];
                c = 2;
            } else {
                // in and/or out mono
                // handle mono
                in[0] = ins[0][offset];
                in[0] *= *params[param_level_in];
                in[1] = in[0];
                c = 1;
            }
            
            float proc[2];
            proc[0] = in[0];
            proc[1] = in[1];
            
            for (int i = 0; i < c; ++i) {
                // all pre filters in chain
                proc[i] = hp[i][1].process(hp[i][0].process(proc[i]));
                
                // saturate
                proc[i] = dist[i].process(proc[i]);

                // all post filters in chain
                proc[i] = hp[i][2].process(hp[i][3].process(proc[i]));
                
                if(*params[param_ceil_active] > 0.5f) {
                    // all H/P post filters in chain
                    proc[i] = lp[i][0].process(lp[i][1].process(proc[i]));
                    
                }
            }
            maxDrive = dist[0].get_distortion_level() * *params[param_amount];
            
            if(in_count > 1 && out_count > 1) {
                maxDrive = std::max(maxDrive, dist[1].get_distortion_level() * *params[param_amount]);
                // full stereo
                out[0] = (proc[0] * *params[param_amount] + in2out * in[0]) * *params[param_level_out];
                out[1] = (proc[1] * *params[param_amount] + in2out * in[1]) * *params[param_level_out];
                outs[0][offset] = out[0];
                outs[1][offset] = out[1];
            } else if(out_count > 1) {
                // mono -> pseudo stereo
                out[1] = out[0] = (proc[0] * *params[param_amount] + in2out * in[0]) * *params[param_level_out];
                outs[0][offset] = out[0];
                outs[1][offset] = out[1];
            } else {
                // stereo -> mono
                // or full mono
                out[0] = (proc[0] * *params[param_amount] + in2out * in[0]) * *params[param_level_out];
                outs[0][offset] = out[0];
            }
            
            // set up in / out meters
            if(maxDrive > meter_drive) {
                meter_drive = maxDrive;
            }
            
            // next sample
            ++offset;
        } // cycle trough samples
        meters.process(params, ins, outs, orig_offset, orig_numsamples);
        // clean up
        hp[0][0].sanitize();
        hp[1][0].sanitize();
        hp[0][1].sanitize();
        hp[1][1].sanitize();
        hp[0][2].sanitize();
        hp[1][2].sanitize();
        hp[0][3].sanitize();
        hp[1][3].sanitize();
    }
    // draw meters
    if(params[param_meter_drive] != NULL) {
        *params[param_meter_drive] = meter_drive;
    }
    // whatever has to be returned x)
    return outputs_mask;
}

/// Bass Enhancer by Markus Schmidt
///
/// This module is based on Krzysztof's filters and Tom's distortion routine.
/// It sends the signal through a lowpass, saturates it and sends it through a lowpass again
///////////////////////////////////////////////////////////////////////////////////////////////

bassenhancer_audio_module::bassenhancer_audio_module()
{
    is_active = false;
    srate = 0;
    meters.reset();
    meter_drive = 0.f;
}

void bassenhancer_audio_module::activate()
{
    is_active = true;
    meters.reset();
    // set all filters
    params_changed();
}
void bassenhancer_audio_module::deactivate()
{
    is_active = false;
}

void bassenhancer_audio_module::params_changed()
{
    // set the params of all filters
    if(*params[param_freq] != freq_old) {
        lp[0][0].set_lp_rbj(*params[param_freq], 0.707, (float)srate);
        lp[0][1].copy_coeffs(lp[0][0]);
        lp[0][2].copy_coeffs(lp[0][0]);
        lp[0][3].copy_coeffs(lp[0][0]);
        if(in_count > 1 && out_count > 1) {
            lp[1][0].copy_coeffs(lp[0][0]);
            lp[1][1].copy_coeffs(lp[0][0]);
            lp[1][2].copy_coeffs(lp[0][0]);
            lp[1][3].copy_coeffs(lp[0][0]);
        }
        freq_old = *params[param_freq];
    }
    // set the params of all filters
    if(*params[param_floor] != floor_old or *params[param_floor_active] != floor_active_old) {
        hp[0][0].set_hp_rbj(*params[param_floor], 0.707, (float)srate);
        hp[0][1].copy_coeffs(hp[0][0]);
        hp[1][0].copy_coeffs(hp[0][0]);
        hp[1][1].copy_coeffs(hp[0][0]);
        floor_old = *params[param_floor];
        floor_active_old = *params[param_floor_active];
    }
    // set distortion
    dist[0].set_params(*params[param_blend], *params[param_drive]);
    if(in_count > 1 && out_count > 1)
        dist[1].set_params(*params[param_blend], *params[param_drive]);
}

void bassenhancer_audio_module::set_sample_rate(uint32_t sr)
{
    srate = sr;
    dist[0].set_sample_rate(sr);
    if(in_count > 1 && out_count > 1)
        dist[1].set_sample_rate(sr);
    meters.set_sample_rate(srate);
}

uint32_t bassenhancer_audio_module::process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask)
{
    bool bypass = *params[param_bypass] > 0.5f;
    uint32_t orig_offset = offset;
    uint32_t orig_numsamples = numsamples;
    numsamples += offset;
    if(bypass) {
        // everything bypassed
        while(offset < numsamples) {
            if(in_count > 1 && out_count > 1) {
                outs[0][offset] = ins[0][offset];
                outs[1][offset] = ins[1][offset];
            } else if(in_count > 1) {
                outs[0][offset] = (ins[0][offset] + ins[1][offset]) / 2;
            } else if(out_count > 1) {
                outs[0][offset] = ins[0][offset];
                outs[1][offset] = ins[0][offset];
            } else {
                outs[0][offset] = ins[0][offset];
            }
            ++offset;
        }
        // displays, too
        meters.bypassed(params, orig_numsamples);
        meter_drive = 0.f;
    } else {
        meter_drive = 0.f;
        
        // process
        while(offset < numsamples) {
            // cycle through samples
            float out[2], in[2] = {0.f, 0.f};
            float maxDrive = 0.f;
            int c = 0;
            
            if(in_count > 1 && out_count > 1) {
                // stereo in/stereo out
                // handle full stereo
                in[0] = ins[0][offset];
                in[0] *= *params[param_level_in];
                in[1] = ins[1][offset];
                in[1] *= *params[param_level_in];
                c = 2;
            } else {
                // in and/or out mono
                // handle mono
                in[0] = ins[0][offset];
                in[0] *= *params[param_level_in];
                in[1] = in[0];
                c = 1;
            }
            
            float proc[2];
            proc[0] = in[0];
            proc[1] = in[1];
            
            for (int i = 0; i < c; ++i) {
                // all pre filters in chain
                proc[i] = lp[i][1].process(lp[i][0].process(proc[i]));
                
                // saturate
                proc[i] = dist[i].process(proc[i]);

                // all post filters in chain
                proc[i] = lp[i][2].process(lp[i][3].process(proc[i]));
                
                if(*params[param_floor_active] > 0.5f) {
                    // all H/P post filters in chain
                    proc[i] = hp[i][0].process(hp[i][1].process(proc[i]));
                    
                }
            }
            
            if(in_count > 1 && out_count > 1) {
                // full stereo
                if(*params[param_listen] > 0.f)
                    out[0] = proc[0] * *params[param_amount] * *params[param_level_out];
                else
                    out[0] = (proc[0] * *params[param_amount] + in[0]) * *params[param_level_out];
                outs[0][offset] = out[0];
                if(*params[param_listen] > 0.f)
                    out[1] = proc[1] * *params[param_amount] * *params[param_level_out];
                else
                    out[1] = (proc[1] * *params[param_amount] + in[1]) * *params[param_level_out];
                outs[1][offset] = out[1];
                maxDrive = std::max(dist[0].get_distortion_level() * *params[param_amount],
                                            dist[1].get_distortion_level() * *params[param_amount]);
            } else if(out_count > 1) {
                // mono -> pseudo stereo
                if(*params[param_listen] > 0.f)
                    out[0] = proc[0] * *params[param_amount] * *params[param_level_out];
                else
                    out[0] = (proc[0] * *params[param_amount] + in[0]) * *params[param_level_out];
                outs[0][offset] = out[0];
                out[1] = out[0];
                outs[1][offset] = out[1];
                maxDrive = dist[0].get_distortion_level() * *params[param_amount];
            } else {
                // stereo -> mono
                // or full mono
                if(*params[param_listen] > 0.f)
                    out[0] = proc[0] * *params[param_amount] * *params[param_level_out];
                else
                    out[0] = (proc[0] * *params[param_amount] + in[0]) * *params[param_level_out];
                outs[0][offset] = out[0];
                maxDrive = dist[0].get_distortion_level() * *params[param_amount];
            }
            
            // set up in / out meters
            if(maxDrive > meter_drive) {
                meter_drive = maxDrive;
            }
            
            // next sample
            ++offset;
        } // cycle trough samples
        meters.process(params, ins, outs, orig_offset, orig_numsamples);
        // clean up
        lp[0][0].sanitize();
        lp[1][0].sanitize();
        lp[0][1].sanitize();
        lp[1][1].sanitize();
        lp[0][2].sanitize();
        lp[1][2].sanitize();
        lp[0][3].sanitize();
        lp[1][3].sanitize();
    }
    // draw meters
    if(params[param_meter_drive] != NULL) {
        *params[param_meter_drive] = meter_drive;
    }
    // whatever has to be returned x)
    return outputs_mask;
}
