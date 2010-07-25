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
 * Boston, MA 02111-1307, USA.
 */
#ifndef CALF_MODULES_COMP_H
#define CALF_MODULES_COMP_H

#include <assert.h>
#include <limits.h>
#include "biquad.h"
#include "inertia.h"
#include "audio_fx.h"
#include "giface.h"
#include "loudness.h"
#include "metadata.h"

namespace calf_plugins {

/// Not a true _audio_module style class, just pretends to be one!
class gain_reduction_audio_module
{
private:
    float linSlope, detected, kneeSqrt, kneeStart, linKneeStart, kneeStop;
    float compressedKneeStop, adjKneeStart, thres;
    float attack, release, threshold, ratio, knee, makeup, detection, stereo_link, bypass, mute, meter_out, meter_comp;
    mutable float old_threshold, old_ratio, old_knee, old_makeup, old_bypass, old_mute, old_detection, old_stereo_link;
    mutable volatile int last_generation;
    uint32_t srate;
    bool is_active;
    inline float output_level(float slope) const;
    inline float output_gain(float linSlope, bool rms) const;
public:
    gain_reduction_audio_module();
    void set_params(float att, float rel, float thr, float rat, float kn, float mak, float det, float stl, float byp, float mu);
    void update_curve();
    void process(float &left, float &right, const float *det_left = NULL, const float *det_right = NULL);
    void activate();
    void deactivate();
    int id;
    void set_sample_rate(uint32_t sr);
    float get_output_level();
    float get_comp_level();
    bool get_graph(int subindex, float *data, int points, cairo_iface *context) const;
    bool get_dot(int subindex, float &x, float &y, int &size, cairo_iface *context) const;
    bool get_gridline(int subindex, float &pos, bool &vertical, std::string &legend, cairo_iface *context) const;
    int  get_changed_offsets(int generation, int &subindex_graph, int &subindex_dot, int &subindex_gridline) const;
};

/// Compressor by Thor
class compressor_audio_module: public audio_module<compressor_metadata>, public line_graph_iface  {
private:
    typedef compressor_audio_module AM;
    uint32_t clip_in, clip_out;
    float meter_in, meter_out;
    gain_reduction_audio_module compressor;
public:
    typedef std::complex<double> cfloat;
    uint32_t srate;
    bool is_active;
    mutable volatile int last_generation, last_calculated_generation;
    compressor_audio_module();
    void activate();
    void deactivate();
    void params_changed();
    void set_sample_rate(uint32_t sr);
    uint32_t process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask);
    bool get_graph(int index, int subindex, float *data, int points, cairo_iface *context) const;
    bool get_dot(int index, int subindex, float &x, float &y, int &size, cairo_iface *context) const;
    bool get_gridline(int index, int subindex, float &pos, bool &vertical, std::string &legend, cairo_iface *context) const;
    int  get_changed_offsets(int index, int generation, int &subindex_graph, int &subindex_dot, int &subindex_gridline) const;
};

/// Sidecain Compressor by Markus Schmidt (based on Thor's compressor and Krzysztof's filters)
class sidechaincompressor_audio_module: public audio_module<sidechaincompressor_metadata>, public frequency_response_line_graph  {
private:
    typedef sidechaincompressor_audio_module AM;
    enum CalfScModes {
        WIDEBAND,
        DEESSER_WIDE,
        DEESSER_SPLIT,
        DERUMBLER_WIDE,
        DERUMBLER_SPLIT,
        WEIGHTED_1,
        WEIGHTED_2,
        WEIGHTED_3,
        BANDPASS_1,
        BANDPASS_2
    };
    mutable float f1_freq_old, f2_freq_old, f1_level_old, f2_level_old;
    mutable float f1_freq_old1, f2_freq_old1, f1_level_old1, f2_level_old1;
    CalfScModes sc_mode;
    mutable CalfScModes sc_mode_old, sc_mode_old1;
    float f1_active, f2_active;
    uint32_t clip_in, clip_out;
    float meter_in, meter_out;
    gain_reduction_audio_module compressor;
    dsp::biquad_d2<float> f1L, f1R, f2L, f2R;
public:
    typedef std::complex<double> cfloat;
    uint32_t srate;
    bool is_active;
    mutable volatile int last_generation, last_calculated_generation;
    sidechaincompressor_audio_module();
    void activate();
    void deactivate();
    void params_changed();
    cfloat h_z(const cfloat &z) const;
    float freq_gain(int index, double freq, uint32_t sr) const;
    void set_sample_rate(uint32_t sr);
    uint32_t process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask);
    bool get_graph(int index, int subindex, float *data, int points, cairo_iface *context) const;
    bool get_dot(int index, int subindex, float &x, float &y, int &size, cairo_iface *context) const;
    bool get_gridline(int index, int subindex, float &pos, bool &vertical, std::string &legend, cairo_iface *context) const;
    int  get_changed_offsets(int index, int generation, int &subindex_graph, int &subindex_dot, int &subindex_gridline) const;
};

/// Multibandcompressor by Markus Schmidt
class multibandcompressor_audio_module: public audio_module<multibandcompressor_metadata>, public line_graph_iface {
private:
    typedef multibandcompressor_audio_module AM;
    static const int strips = 4;
    bool mute[strips];
    uint32_t clip_inL, clip_inR, clip_outL, clip_outR;
    float meter_inL, meter_inR, meter_outL, meter_outR;
    gain_reduction_audio_module strip[strips];
    dsp::biquad_d2<float> lpL0, lpR0, lpL1, lpR1, lpL2, lpR2, hpL0, hpR0, hpL1, hpR1, hpL2, hpR2;
    float freq_old[strips - 1], sep_old[strips - 1], q_old[strips - 1];
public:
    uint32_t srate;
    bool is_active;
    multibandcompressor_audio_module();
    void activate();
    void deactivate();
    void params_changed();
    uint32_t process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask);
    void set_sample_rate(uint32_t sr);
    const gain_reduction_audio_module *get_strip_by_param_index(int index) const;
    virtual bool get_graph(int index, int subindex, float *data, int points, cairo_iface *context) const;
    virtual bool get_dot(int index, int subindex, float &x, float &y, int &size, cairo_iface *context) const;
    virtual bool get_gridline(int index, int subindex, float &pos, bool &vertical, std::string &legend, cairo_iface *context) const;
    virtual int  get_changed_offsets(int index, int generation, int &subindex_graph, int &subindex_dot, int &subindex_gridline) const;
};

/// Deesser by Markus Schmidt (based on Thor's compressor and Krzysztof's filters)
class deesser_audio_module: public audio_module<deesser_metadata>, public frequency_response_line_graph  {
private:
    enum CalfDeessModes {
        WIDE,
        SPLIT
    };
    mutable float f1_freq_old, f2_freq_old, f1_level_old, f2_level_old, f2_q_old;
    mutable float f1_freq_old1, f2_freq_old1, f1_level_old1, f2_level_old1, f2_q_old1;
    uint32_t detected_led;
    float detected, clip_out;
    uint32_t clip_led;
    gain_reduction_audio_module compressor;
    dsp::biquad_d2<float> hpL, hpR, lpL, lpR, pL, pR;
public:
    uint32_t srate;
    bool is_active;
    mutable volatile int last_generation, last_calculated_generation;
    deesser_audio_module();
    void activate();
    void deactivate();
    void params_changed();
    float freq_gain(int index, double freq, uint32_t sr) const
    {
        return hpL.freq_gain(freq, sr) * pL.freq_gain(freq, sr);
    }
    void set_sample_rate(uint32_t sr);
    uint32_t process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask);
    bool get_graph(int index, int subindex, float *data, int points, cairo_iface *context) const;
    bool get_gridline(int index, int subindex, float &pos, bool &vertical, std::string &legend, cairo_iface *context) const;
    int  get_changed_offsets(int index, int generation, int &subindex_graph, int &subindex_dot, int &subindex_gridline) const;
};


class gate_audio_module: public audio_module<gate_metadata>, public line_graph_iface {
private:
    float linSlope, peak, detected, kneeSqrt, kneeStart, linKneeStart, kneeStop, linKneeStop, threshold, ratio, knee, makeup, compressedKneeStop, adjKneeStart, range;
    mutable float old_threshold, old_ratio, old_knee, old_makeup, old_bypass, old_range, old_trigger, old_mono;
    mutable volatile int last_generation;
    uint32_t clip;
    dsp::aweighter awL, awR;
    dsp::biquad_d2<float> bpL, bpR;
public:
    uint32_t srate;
    bool is_active;
    gate_audio_module();
    void activate();
    void deactivate();
    uint32_t process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask);

    inline float output_level(float slope) const {
        bool rms = *params[param_detection] == 0;
        return slope * output_gain(rms ? slope*slope : slope, rms) * makeup;
    }

    inline float output_gain(float linSlope, bool rms) const {
        if(linSlope < linKneeStop) {
            float slope = log(linSlope);

            //float tratio = rms ? sqrt(ratio) : ratio;
            float tratio = ratio;
            float gain = 0.f;
            float delta = 0.f;
	    if(IS_FAKE_INFINITY(ratio))
	        tratio = 1000.f;
            gain = (slope-threshold) * tratio + threshold;
            delta = tratio;

            if(knee > 1.f && slope > kneeStart ) {
		gain = dsp::hermite_interpolation(slope, kneeStart, kneeStop, ((kneeStart - threshold) * tratio  + threshold), kneeStop, delta,1.f);
	    }
	    return std::max(range, expf(gain-slope));
	}


        return 1.f;
    }

    void set_sample_rate(uint32_t sr);

    virtual bool get_graph(int index, int subindex, float *data, int points, cairo_iface *context) const;
    virtual bool get_dot(int index, int subindex, float &x, float &y, int &size, cairo_iface *context) const;
    virtual bool get_gridline(int index, int subindex, float &pos, bool &vertical, std::string &legend, cairo_iface *context) const;
    virtual int  get_changed_offsets(int generation, int &subindex_graph, int &subindex_dot, int &subindex_gridline) const;
};


};

#endif
