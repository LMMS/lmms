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
 * Boston, MA 02111-1307, USA.
 */
#ifndef CALF_MODULES_DIST_H
#define CALF_MODULES_DIST_H

#include <assert.h>
#include <limits.h>
#include "biquad.h"
#include "inertia.h"
#include "audio_fx.h"
#include "giface.h"
#include "metadata.h"
#include "plugin_tools.h"

namespace calf_plugins {

/// Saturator by Markus Schmidt (based on Krzysztof's filters and Tom's distortion algorythm)
class saturator_audio_module: public audio_module<saturator_metadata> {
private:
    float hp_pre_freq_old, lp_pre_freq_old;
    float hp_post_freq_old, lp_post_freq_old;
    float p_level_old, p_freq_old, p_q_old;
    stereo_in_out_metering<saturator_metadata> meters;
    float meter_drive;
    dsp::biquad_d2<float> lp[2][4], hp[2][4];
    dsp::biquad_d2<float> p[2];
    dsp::tap_distortion dist[2];
public:
    uint32_t srate;
    bool is_active;
    saturator_audio_module();
    void activate();
    void deactivate();
    void params_changed();
    void set_sample_rate(uint32_t sr);
    uint32_t process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask);
};

/// Exciter by Markus Schmidt (based on Krzysztof's filters and Tom's distortion algorythm)
class exciter_audio_module: public audio_module<exciter_metadata> {
private:
    float freq_old, ceil_old;
    bool ceil_active_old;
    stereo_in_out_metering<exciter_metadata> meters;
    float meter_drive;
    dsp::biquad_d2<float> hp[2][4];
    dsp::biquad_d2<float> lp[2][2];
    dsp::tap_distortion dist[2];
public:
    uint32_t srate;
    bool is_active;
    exciter_audio_module();
    void activate();
    void deactivate();
    void params_changed();
    void set_sample_rate(uint32_t sr);
    uint32_t process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask);
};

/// Bass Enhancer by Markus Schmidt (based on Krzysztof's filters and Tom's distortion algorythm)
class bassenhancer_audio_module: public audio_module<bassenhancer_metadata> {
private:
    float freq_old, floor_old;
    bool floor_active_old;
    stereo_in_out_metering<exciter_metadata> meters;
    float meter_drive;
    dsp::biquad_d2<float> lp[2][4];
    dsp::biquad_d2<float> hp[2][2];
    dsp::tap_distortion dist[2];
public:
    uint32_t srate;
    bool is_active;
    bassenhancer_audio_module();
    void activate();
    void deactivate();
    void params_changed();
    void set_sample_rate(uint32_t sr);
    uint32_t process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask);
};

};

#endif
