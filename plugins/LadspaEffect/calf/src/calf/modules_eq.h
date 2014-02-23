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
 * Boston, MA 02111-1307, USA.
 */
#ifndef CALF_MODULES_EQ_H
#define CALF_MODULES_EQ_H

#include <assert.h>
#include <limits.h>
#include "biquad.h"
#include "inertia.h"
#include "audio_fx.h"
#include "giface.h"
#include "metadata.h"
#include "plugin_tools.h"

namespace calf_plugins {

/// Equalizer N Band by Markus Schmidt (based on Krzysztof's filters)
template<class BaseClass, bool has_lphp>
class equalizerNband_audio_module: public audio_module<BaseClass>, public frequency_response_line_graph {
public:
    typedef audio_module<BaseClass> AM;
    using AM::ins;
    using AM::outs;
    using AM::params;
    using AM::in_count;
    using AM::out_count;
    using AM::param_count;
    using AM::PeakBands;
private:
    enum { graph_param_count = BaseClass::last_graph_param - BaseClass::first_graph_param + 1, params_per_band = AM::param_p2_active - AM::param_p1_active };
    float hp_mode_old, hp_freq_old;
    float lp_mode_old, lp_freq_old;
    float ls_level_old, ls_freq_old;
    float hs_level_old, hs_freq_old;
    float p_level_old[PeakBands], p_freq_old[PeakBands], p_q_old[PeakBands];
    mutable float old_params_for_graph[graph_param_count];
    dual_in_out_metering<BaseClass> meters;
    CalfEqMode hp_mode, lp_mode;
    dsp::biquad_d2<float> hp[3][2], lp[3][2];
    dsp::biquad_d2<float> lsL, lsR, hsL, hsR;
    dsp::biquad_d2<float> pL[PeakBands], pR[PeakBands];
    
    inline void process_hplp(float &left, float &right);
public:
    typedef std::complex<double> cfloat;
    uint32_t srate;
    bool is_active;
    mutable volatile int last_generation, last_calculated_generation;
    equalizerNband_audio_module();
    void activate();
    void deactivate();

    void params_changed();
    float freq_gain(int index, double freq, uint32_t sr) const;
    void set_sample_rate(uint32_t sr)
    {
        srate = sr;
        meters.set_sample_rate(sr);
    }
    uint32_t process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask);
    bool get_graph(int index, int subindex, float *data, int points, cairo_iface *context) const;
    bool get_gridline(int index, int subindex, float &pos, bool &vertical, std::string &legend, cairo_iface *context) const;
    int  get_changed_offsets(int index, int generation, int &subindex_graph, int &subindex_dot, int &subindex_gridline) const;
};

typedef equalizerNband_audio_module<equalizer5band_metadata, false> equalizer5band_audio_module;
typedef equalizerNband_audio_module<equalizer8band_metadata, true> equalizer8band_audio_module;
typedef equalizerNband_audio_module<equalizer12band_metadata, true> equalizer12band_audio_module;

};

#endif
