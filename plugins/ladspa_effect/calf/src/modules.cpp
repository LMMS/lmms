/* Calf DSP plugin pack
 * Assorted plugins
 *
 * Copyright (C) 2001-2010 Krzysztof Foltman, Markus Schmidt, Thor Harald Johansen
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
#include <calf/modules.h>
#include <calf/modules_dev.h>

using namespace dsp;
using namespace calf_plugins;

#define SET_IF_CONNECTED(name) if (params[AM::param_##name] != NULL) *params[AM::param_##name] = name;

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

void reverb_audio_module::params_changed()
{
    reverb.set_type_and_diffusion(fastf2i_drm(*params[par_roomsize]), *params[par_diffusion]);
    reverb.set_time(*params[par_decay]);
    reverb.set_cutoff(*params[par_hfdamp]);
    amount.set_inertia(*params[par_amount]);
    dryamount.set_inertia(*params[par_dry]);
    left_lo.set_lp(dsp::clip(*params[par_treblecut], 20.f, (float)(srate * 0.49f)), srate);
    left_hi.set_hp(dsp::clip(*params[par_basscut], 20.f, (float)(srate * 0.49f)), srate);
    right_lo.copy_coeffs(left_lo);
    right_hi.copy_coeffs(left_hi);
    predelay_amt = (int) (srate * (*params[par_predelay]) * (1.0f / 1000.0f) + 1);
}

uint32_t reverb_audio_module::process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask)
{
    numsamples += offset;
    clip   -= std::min(clip, numsamples);
    for (uint32_t i = offset; i < numsamples; i++) {
        float dry = dryamount.get();
        float wet = amount.get();
        stereo_sample<float> s(ins[0][i], ins[1][i]);
        stereo_sample<float> s2 = pre_delay.process(s, predelay_amt);
        
        float rl = s2.left, rr = s2.right;
        rl = left_lo.process(left_hi.process(rl));
        rr = right_lo.process(right_hi.process(rr));
        reverb.process(rl, rr);
        outs[0][i] = dry*s.left + wet*rl;
        outs[1][i] = dry*s.right + wet*rr;
        meter_wet = std::max(fabs(wet*rl), fabs(wet*rr));
        meter_out = std::max(fabs(outs[0][i]), fabs(outs[1][i]));
        if(outs[0][i] > 1.f or outs[1][i] > 1.f) {
            clip = srate >> 3;
        }
    }
    reverb.extra_sanitize();
    left_lo.sanitize();
    left_hi.sanitize();
    right_lo.sanitize();
    right_hi.sanitize();
    if(params[par_meter_wet] != NULL) {
        *params[par_meter_wet] = meter_wet;
    }
    if(params[par_meter_out] != NULL) {
        *params[par_meter_out] = meter_out;
    }
    if(params[par_clip] != NULL) {
        *params[par_clip] = clip;
    }
    return outputs_mask;
}

///////////////////////////////////////////////////////////////////////////////////////////////

vintage_delay_audio_module::vintage_delay_audio_module()
{
    old_medium = -1;
    for (int i = 0; i < MAX_DELAY; i++) {
        buffers[0][i] = 0.f;
        buffers[1][i] = 0.f;
    }
}

void vintage_delay_audio_module::params_changed()
{
    float unit = 60.0 * srate / (*params[par_bpm] * *params[par_divide]);
    deltime_l = dsp::fastf2i_drm(unit * *params[par_time_l]);
    deltime_r = dsp::fastf2i_drm(unit * *params[par_time_r]);
    int deltime_fb = deltime_l + deltime_r;
    float fb = *params[par_feedback];
    dry.set_inertia(*params[par_dryamount]);
    mixmode = dsp::fastf2i_drm(*params[par_mixmode]);
    medium = dsp::fastf2i_drm(*params[par_medium]);
    switch(mixmode)
    {
    case MIXMODE_STEREO:
        fb_left.set_inertia(fb);
        fb_right.set_inertia(pow(fb, *params[par_time_r] / *params[par_time_l]));
        amt_left.set_inertia(*params[par_amount]);
        amt_right.set_inertia(*params[par_amount]);
        break;
    case MIXMODE_PINGPONG:
        fb_left.set_inertia(fb);
        fb_right.set_inertia(fb);
        amt_left.set_inertia(*params[par_amount]);
        amt_right.set_inertia(*params[par_amount]);
        break;
    case MIXMODE_LR:
        fb_left.set_inertia(fb);
        fb_right.set_inertia(fb);
        amt_left.set_inertia(*params[par_amount]);                                          // L is straight 'amount'
        amt_right.set_inertia(*params[par_amount] * pow(fb, 1.0 * deltime_r / deltime_fb)); // R is amount with feedback based dampening as if it ran through R/FB*100% of delay line's dampening
        // deltime_l <<< deltime_r -> pow() = fb -> full delay line worth of dampening
        // deltime_l >>> deltime_r -> pow() = 1 -> no dampening
        break;
    case MIXMODE_RL:
        fb_left.set_inertia(fb);
        fb_right.set_inertia(fb);
        amt_left.set_inertia(*params[par_amount] * pow(fb, 1.0 * deltime_l / deltime_fb));
        amt_right.set_inertia(*params[par_amount]);
        break;
    }
    chmix.set_inertia((1 - *params[par_width]) * 0.5);
    if (medium != old_medium)
        calc_filters();
}

void vintage_delay_audio_module::activate()
{
    bufptr = 0;
    age = 0;
}

void vintage_delay_audio_module::deactivate()
{
}

void vintage_delay_audio_module::set_sample_rate(uint32_t sr)
{
    srate = sr;
    old_medium = -1;
    amt_left.set_sample_rate(sr); amt_right.set_sample_rate(sr);
    fb_left.set_sample_rate(sr); fb_right.set_sample_rate(sr);
}

void vintage_delay_audio_module::calc_filters()
{
    // parameters are heavily influenced by gordonjcp and his tape delay unit
    // although, don't blame him if it sounds bad - I've messed with them too :)
    biquad_left[0].set_lp_rbj(6000, 0.707, srate);
    biquad_left[1].set_bp_rbj(4500, 0.250, srate);
    biquad_right[0].copy_coeffs(biquad_left[0]);
    biquad_right[1].copy_coeffs(biquad_left[1]);
}

/// Single delay line with feedback at the same tap
static inline void delayline_impl(int age, int deltime, float dry_value, const float &delayed_value, float &out, float &del, gain_smoothing &amt, gain_smoothing &fb)
{
    // if the buffer hasn't been cleared yet (after activation), pretend we've read zeros
    if (age <= deltime) {
        out = 0;
        del = dry_value;
        amt.step();
        fb.step();
    }
    else
    {
        float delayed = delayed_value; // avoid dereferencing the pointer in 'then' branch of the if()
        dsp::sanitize(delayed);
        out = delayed * amt.get();
        del = dry_value + delayed * fb.get();
    }
}

/// Single delay line with tap output
static inline void delayline2_impl(int age, int deltime, float dry_value, const float &delayed_value, const float &delayed_value_for_fb, float &out, float &del, gain_smoothing &amt, gain_smoothing &fb)
{
    if (age <= deltime) {
        out = 0;
        del = dry_value;
        amt.step();
        fb.step();
    }
    else
    {
        out = delayed_value * amt.get();
        del = dry_value + delayed_value_for_fb * fb.get();
        dsp::sanitize(out);
        dsp::sanitize(del);
    }
}

static inline void delay_mix(float dry_left, float dry_right, float &out_left, float &out_right, float dry, float chmix)
{
    float tmp_left = lerp(out_left, out_right, chmix);
    float tmp_right = lerp(out_right, out_left, chmix);
    out_left = dry_left * dry + tmp_left;
    out_right = dry_right * dry + tmp_right;
}

uint32_t vintage_delay_audio_module::process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask)
{
    uint32_t ostate = 3; // XXXKF optimize!
    uint32_t end = offset + numsamples;
    int orig_bufptr = bufptr;
    float out_left, out_right, del_left, del_right;
    
    switch(mixmode)
    {
        case MIXMODE_STEREO:
        case MIXMODE_PINGPONG:
        {
            int v = mixmode == MIXMODE_PINGPONG ? 1 : 0;
            for(uint32_t i = offset; i < end; i++)
            {                
                delayline_impl(age, deltime_l, ins[0][i], buffers[v][(bufptr - deltime_l) & ADDR_MASK], out_left, del_left, amt_left, fb_left);
                delayline_impl(age, deltime_r, ins[1][i], buffers[1 - v][(bufptr - deltime_r) & ADDR_MASK], out_right, del_right, amt_right, fb_right);
                delay_mix(ins[0][i], ins[1][i], out_left, out_right, dry.get(), chmix.get());
                
                age++;
                outs[0][i] = out_left; outs[1][i] = out_right; buffers[0][bufptr] = del_left; buffers[1][bufptr] = del_right;
                bufptr = (bufptr + 1) & (MAX_DELAY - 1);
            }
        }
        break;
        
        case MIXMODE_LR:
        case MIXMODE_RL:
        {
            int v = mixmode == MIXMODE_RL ? 1 : 0;
            int deltime_fb = deltime_l + deltime_r;
            int deltime_l_corr = mixmode == MIXMODE_RL ? deltime_fb : deltime_l;
            int deltime_r_corr = mixmode == MIXMODE_LR ? deltime_fb : deltime_r;
            
            for(uint32_t i = offset; i < end; i++)
            {
                delayline2_impl(age, deltime_l, ins[0][i], buffers[v][(bufptr - deltime_l_corr) & ADDR_MASK], buffers[v][(bufptr - deltime_fb) & ADDR_MASK], out_left, del_left, amt_left, fb_left);
                delayline2_impl(age, deltime_r, ins[1][i], buffers[1 - v][(bufptr - deltime_r_corr) & ADDR_MASK], buffers[1-v][(bufptr - deltime_fb) & ADDR_MASK], out_right, del_right, amt_right, fb_right);
                delay_mix(ins[0][i], ins[1][i], out_left, out_right, dry.get(), chmix.get());
                
                age++;
                outs[0][i] = out_left; outs[1][i] = out_right; buffers[0][bufptr] = del_left; buffers[1][bufptr] = del_right;
                bufptr = (bufptr + 1) & (MAX_DELAY - 1);
            }
        }
    }
    if (age >= MAX_DELAY)
        age = MAX_DELAY;
    if (medium > 0) {
        bufptr = orig_bufptr;
        if (medium == 2)
        {
            for(uint32_t i = offset; i < end; i++)
            {
                buffers[0][bufptr] = biquad_left[0].process_lp(biquad_left[1].process(buffers[0][bufptr]));
                buffers[1][bufptr] = biquad_right[0].process_lp(biquad_right[1].process(buffers[1][bufptr]));
                bufptr = (bufptr + 1) & (MAX_DELAY - 1);
            }
            biquad_left[0].sanitize();biquad_right[0].sanitize();
        } else {
            for(uint32_t i = offset; i < end; i++)
            {
                buffers[0][bufptr] = biquad_left[1].process(buffers[0][bufptr]);
                buffers[1][bufptr] = biquad_right[1].process(buffers[1][bufptr]);
                bufptr = (bufptr + 1) & (MAX_DELAY - 1);
            }
        }
        biquad_left[1].sanitize();biquad_right[1].sanitize();
        
    }
    return ostate;
}

///////////////////////////////////////////////////////////////////////////////////////////////

bool filter_audio_module::get_graph(int index, int subindex, float *data, int points, cairo_iface *context) const
{
    if (!is_active)
        return false;
    if (index == par_cutoff && !subindex) {
        context->set_line_width(1.5);
        return ::get_graph(*this, subindex, data, points);
    }
    return false;
}

int filter_audio_module::get_changed_offsets(int index, int generation, int &subindex_graph, int &subindex_dot, int &subindex_gridline) const
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

filterclavier_audio_module::filterclavier_audio_module() 
: filter_module_with_inertia<biquad_filter_module, filterclavier_metadata>(ins, outs, params)
, min_gain(1.0)
, max_gain(32.0)
, last_note(-1)
, last_velocity(-1)
{
}
    
void filterclavier_audio_module::params_changed()
{ 
    inertia_filter_module::inertia_cutoff.set_inertia(
        note_to_hz(last_note + *params[par_transpose], *params[par_detune]));
    
    float min_resonance = param_props[par_max_resonance].min;
     inertia_filter_module::inertia_resonance.set_inertia( 
             (float(last_velocity) / 127.0)
             // 0.001: see below
             * (*params[par_max_resonance] - min_resonance + 0.001)
             + min_resonance);
         
    adjust_gain_according_to_filter_mode(last_velocity);
    
    inertia_filter_module::calculate_filter(); 
}

void filterclavier_audio_module::activate()
{
    inertia_filter_module::activate();
}

void filterclavier_audio_module::set_sample_rate(uint32_t sr)
{
    inertia_filter_module::set_sample_rate(sr);
}

void filterclavier_audio_module::deactivate()
{
    inertia_filter_module::deactivate();
}


void filterclavier_audio_module::note_on(int channel, int note, int vel)
{
    last_note     = note;
    last_velocity = vel;
    inertia_filter_module::inertia_cutoff.set_inertia(
            note_to_hz(note + *params[par_transpose], *params[par_detune]));

    float min_resonance = param_props[par_max_resonance].min;
    inertia_filter_module::inertia_resonance.set_inertia( 
            (float(vel) / 127.0) 
            // 0.001: if the difference is equal to zero (which happens
            // when the max_resonance knom is at minimum position
            // then the filter gain doesnt seem to snap to zero on most note offs
            * (*params[par_max_resonance] - min_resonance + 0.001) 
            + min_resonance);
    
    adjust_gain_according_to_filter_mode(vel);
    
    inertia_filter_module::calculate_filter();
}

void filterclavier_audio_module::note_off(int channel, int note, int vel)
{
    if (note == last_note) {
        inertia_filter_module::inertia_resonance.set_inertia(param_props[par_max_resonance].min);
        inertia_filter_module::inertia_gain.set_inertia(min_gain);
        inertia_filter_module::calculate_filter();
        last_velocity = 0;
    }
}

void filterclavier_audio_module::adjust_gain_according_to_filter_mode(int velocity)
{
    int   mode = dsp::fastf2i_drm(*params[par_mode]);
    
    // for bandpasses: boost gain for velocities > 0
    if ( (mode_6db_bp <= mode) && (mode <= mode_18db_bp) ) {
        // gain for velocity 0:   1.0
        // gain for velocity 127: 32.0
        float mode_max_gain = max_gain;
        // max_gain is right for mode_6db_bp
        if (mode == mode_12db_bp)
            mode_max_gain /= 6.0;
        if (mode == mode_18db_bp)
            mode_max_gain /= 10.5;
        
        inertia_filter_module::inertia_gain.set_now(
                (float(velocity) / 127.0) * (mode_max_gain - min_gain) + min_gain);
    } else {
        inertia_filter_module::inertia_gain.set_now(min_gain);
    }
}

bool filterclavier_audio_module::get_graph(int index, int subindex, float *data, int points, cairo_iface *context) const
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

