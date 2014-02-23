/* Calf DSP Library
 * Example audio modules - monosynth
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
 * Boston, MA  02110-1301  USA
 */
#include <calf/giface.h>
#include <calf/modules_synths.h>

using namespace dsp;
using namespace calf_plugins;
using namespace std;

float silence[4097];

monosynth_audio_module::monosynth_audio_module()
: mod_matrix_impl(mod_matrix_data, &mm_metadata)
, inertia_cutoff(1)
, inertia_pitchbend(1)
, inertia_pressure(64)
{
}

void monosynth_audio_module::activate() {
    running = false;
    output_pos = 0;
    queue_note_on = -1;
    inertia_pitchbend.set_now(1.f);
    lfo_bend = 1.0;
    modwheel_value = 0.f;
    modwheel_value_int = 0;
    inertia_cutoff.set_now(*params[par_cutoff]);
    inertia_pressure.set_now(0);
    filter.reset();
    filter2.reset();
    stack.clear();
    last_pwshift1 = last_pwshift2 = 0;
    last_stretch1 = 65536;
    queue_note_on_and_off = false;
    prev_wave1 = -1;
    prev_wave2 = -1;
    wave1 = -1;
    wave2 = -1;
    queue_note_on = -1;
    last_filter_type = -1;
}

waveform_family<MONOSYNTH_WAVE_BITS> *monosynth_audio_module::waves;

void monosynth_audio_module::precalculate_waves(progress_report_iface *reporter)
{
    float data[1 << MONOSYNTH_WAVE_BITS];
    bandlimiter<MONOSYNTH_WAVE_BITS> bl;
    
    if (waves)
        return;
    
    static waveform_family<MONOSYNTH_WAVE_BITS> waves_data[wave_count];
    waves = waves_data;
    
    enum { S = 1 << MONOSYNTH_WAVE_BITS, HS = S / 2, QS = S / 4, QS3 = 3 * QS };
    float iQS = 1.0 / QS;
    
    if (reporter)
        reporter->report_progress(0, "Precalculating waveforms");
    
    // yes these waves don't have really perfect 1/x spectrum because of aliasing
    // (so what?)
    for (int i = 0 ; i < HS; i++)
        data[i] = (float)(i * 1.0 / HS),
        data[i + HS] = (float)(i * 1.0 / HS - 1.0f);
    waves[wave_saw].make(bl, data);

    // this one is dummy, fake and sham, we're using a difference of two sawtooths for square wave due to PWM
    for (int i = 0 ; i < S; i++)
        data[i] = (float)(i < HS ? -1.f : 1.f);
    waves[wave_sqr].make(bl, data, 4);

    for (int i = 0 ; i < S; i++)
        data[i] = (float)(i < (64 * S / 2048)? -1.f : 1.f);
    waves[wave_pulse].make(bl, data);

    for (int i = 0 ; i < S; i++)
        data[i] = (float)sin(i * M_PI / HS);
    waves[wave_sine].make(bl, data);

    for (int i = 0 ; i < QS; i++) {
        data[i] = i * iQS,
        data[i + QS] = 1 - i * iQS,
        data[i + HS] = - i * iQS,
        data[i + QS3] = -1 + i * iQS;
    }
    waves[wave_triangle].make(bl, data);
    
    for (int i = 0, j = 1; i < S; i++) {
        data[i] = -1 + j * 1.0 / HS;
        if (i == j)
            j *= 2;
    }
    waves[wave_varistep].make(bl, data);

    for (int i = 0; i < S; i++) {
        data[i] = (min(1.f, (float)(i / 64.f))) * (1.0 - i * 1.0 / S) * (-1 + fmod (i * i * 8/ (S * S * 1.0), 2.0));
    }
    waves[wave_skewsaw].make(bl, data);
    for (int i = 0; i < S; i++) {
        data[i] = (min(1.f, (float)(i / 64.f))) * (1.0 - i * 1.0 / S) * (fmod (i * i * 8/ (S * S * 1.0), 2.0) < 1.0 ? -1.0 : +1.0);
    }
    waves[wave_skewsqr].make(bl, data);

    if (reporter)
        reporter->report_progress(50, "Precalculating waveforms");
    
    for (int i = 0; i < S; i++) {
        if (i < QS3) {
            float p = i * 1.0 / QS3;
            data[i] = sin(M_PI * p * p * p);
        } else {
            float p = (i - QS3 * 1.0) / QS;
            data[i] = -0.5 * sin(3 * M_PI * p * p);
        }
    }
    waves[wave_test1].make(bl, data);
    for (int i = 0; i < S; i++) {
        data[i] = exp(-i * 1.0 / HS) * sin(i * M_PI / HS) * cos(2 * M_PI * i / HS);
    }
    normalize_waveform(data, S);
    waves[wave_test2].make(bl, data);
    for (int i = 0; i < S; i++) {
        //int ii = (i < HS) ? i : S - i;
        int ii = HS;
        data[i] = (ii * 1.0 / HS) * sin(i * 3 * M_PI / HS + 2 * M_PI * sin(M_PI / 4 + i * 4 * M_PI / HS)) * sin(i * 5 * M_PI / HS + 2 * M_PI * sin(M_PI / 8 + i * 6 * M_PI / HS));
    }
    waves[wave_test3].make(bl, data);
    for (int i = 0; i < S; i++) {
        data[i] = sin(i * 2 * M_PI / HS + sin(i * 2 * M_PI / HS + 0.5 * M_PI * sin(i * 18 * M_PI / HS)) * sin(i * 1 * M_PI / HS + 0.5 * M_PI * sin(i * 11 * M_PI / HS)));
    }
    waves[wave_test4].make(bl, data);
    for (int i = 0; i < S; i++) {
        data[i] = sin(i * 2 * M_PI / HS + 0.2 * M_PI * sin(i * 13 * M_PI / HS) + 0.1 * M_PI * sin(i * 37 * M_PI / HS)) * sin(i * M_PI / HS + 0.2 * M_PI * sin(i * 15 * M_PI / HS));
    }
    waves[wave_test5].make(bl, data);
    for (int i = 0; i < S; i++) {
        if (i < HS)
            data[i] = sin(i * 2 * M_PI / HS);
        else
        if (i < 3 * S / 4)
            data[i] = sin(i * 4 * M_PI / HS);
        else
        if (i < 7 * S / 8)
            data[i] = sin(i * 8 * M_PI / HS);
        else
            data[i] = sin(i * 8 * M_PI / HS) * (S - i) / (S / 8);
    }
    waves[wave_test6].make(bl, data);
    for (int i = 0; i < S; i++) {
        int j = i >> (MONOSYNTH_WAVE_BITS - 11);
        data[i] = (j ^ 0x1D0) * 1.0 / HS - 1;
    }
    waves[wave_test7].make(bl, data);
    for (int i = 0; i < S; i++) {
        int j = i >> (MONOSYNTH_WAVE_BITS - 11);
        data[i] = -1 + 0.66 * (3 & ((j >> 8) ^ (j >> 10) ^ (j >> 6)));
    }
    waves[wave_test8].make(bl, data);
    if (reporter)
        reporter->report_progress(100, "");
    
}

bool monosynth_audio_module::get_graph(int index, int subindex, float *data, int points, cairo_iface *context) const
{
    monosynth_audio_module::precalculate_waves(NULL);
    // printf("get_graph %d %p %d wave1=%d wave2=%d\n", index, data, points, wave1, wave2);
    if (index == par_wave1 || index == par_wave2) {
        if (subindex)
            return false;
        enum { S = 1 << MONOSYNTH_WAVE_BITS };
        float value = *params[index];
        int wave = dsp::clip(dsp::fastf2i_drm(value), 0, (int)wave_count - 1);

        uint32_t shift = index == par_wave1 ? last_pwshift1 : last_pwshift2;
        if (!running)
            shift = (int32_t)(0x78000000 * (*params[index == par_wave1 ? par_pw1 : par_pw2]));
        int flag = (wave == wave_sqr);
                
        shift = (flag ? S/2 : 0) + (shift >> (32 - MONOSYNTH_WAVE_BITS));
        int sign = flag ? -1 : 1;
        if (wave == wave_sqr)
            wave = wave_saw;
        float *waveform = waves[wave].original;
        float rnd_start = 1 - *params[par_window1] * 0.5f;
        float scl = rnd_start < 1.0 ? 1.f / (1 - rnd_start) : 0.f;
        for (int i = 0; i < points; i++)
        {
            int pos = i * S / points;
            float r = 1;
            if (index == par_wave1)
            {
                float ph = i * 1.0 / points;
                if (ph < 0.5f)
                    ph = 1.f - ph;
                ph = (ph - rnd_start) * scl;
                if (ph < 0)
                    ph = 0;
                r = 1.0 - ph * ph;
                pos = int(pos * 1.0 * last_stretch1 / 65536.0 ) % S;
            }
            data[i] = r * (sign * waveform[pos] + waveform[(pos + shift) & (S - 1)]) / (sign == -1 ? 1 : 2);
        }
        return true;
    }
    if (index == par_filtertype) {
        if (!running)
            return false;
        if (subindex > (is_stereo_filter() ? 1 : 0))
            return false;
        for (int i = 0; i < points; i++)
        {
            double freq = 20.0 * pow (20000.0 / 20.0, i * 1.0 / points);
            
            const dsp::biquad_d1_lerp<float> &f = subindex ? filter2 : filter;
            float level = f.freq_gain(freq, srate);
            if (!is_stereo_filter())
                level *= filter2.freq_gain(freq, srate);
            level *= fgain;
            
            data[i] = log(level) / log(1024.0) + 0.5;
        }
        return true;
    }
    return get_static_graph(index, subindex, *params[index], data, points, context);
}

void monosynth_audio_module::calculate_buffer_oscs(float lfo1)
{
    int flag1 = (wave1 == wave_sqr);
    int flag2 = (wave2 == wave_sqr);
    
    int32_t shift1 = last_pwshift1;
    int32_t shift2 = last_pwshift2;
    int32_t stretch1 = last_stretch1;
    int32_t shift_target1 = (int32_t)(0x78000000 * dsp::clip11(*params[par_pw1] + lfo1 * *params[par_lfopw] + 0.01f * moddest[moddest_o1pw]));
    int32_t shift_target2 = (int32_t)(0x78000000 * dsp::clip11(*params[par_pw2] + lfo1 * *params[par_lfopw] + 0.01f * moddest[moddest_o2pw]));
    int32_t stretch_target1 = (int32_t)(65536 * dsp::clip(*params[par_stretch1] + 0.01f * moddest[moddest_o1stretch], 1.f, 16.f));
    int32_t shift_delta1 = ((shift_target1 >> 1) - (last_pwshift1 >> 1)) >> (step_shift - 1);
    int32_t shift_delta2 = ((shift_target2 >> 1) - (last_pwshift2 >> 1)) >> (step_shift - 1);
    int32_t stretch_delta1 = ((stretch_target1 >> 1) - (last_stretch1 >> 1)) >> (step_shift - 1);
    last_pwshift1 = shift_target1;
    last_pwshift2 = shift_target2;
    last_stretch1 = stretch_target1;
    lookup_waveforms();
    
    shift1 += (flag1 << 31);
    shift2 += (flag2 << 31);
    float mix1 = 1 - 2 * flag1, mix2 = 1 - 2 * flag2;
    
    float new_xfade = dsp::clip<float>(xfade + 0.01f * moddest[moddest_oscmix], 0.f, 1.f);
    float cur_xfade = last_xfade;
    float xfade_step = (new_xfade - cur_xfade) * (1.0 / step_size);
    
    float rnd_start = 1 - *params[par_window1] * 0.5f;
    float scl = rnd_start < 1.0 ? 1.f / (1 - rnd_start) : 0.f;
    
    for (uint32_t i = 0; i < step_size; i++) 
    {
        //buffer[i] = lerp(osc1.get_phaseshifted(shift1, mix1), osc2.get_phaseshifted(shift2, mix2), cur_xfade);
        float o1phase = osc1.phase / (65536.0 * 65536.0);
        if (o1phase < 0.5)
            o1phase = 1 - o1phase;
        o1phase = (o1phase - rnd_start) * scl;
        if (o1phase < 0)
            o1phase = 0;
        float r = 1.0 - o1phase * o1phase;
        buffer[i] = lerp(r * osc1.get_phasedist(stretch1, shift1, mix1), osc2.get_phaseshifted(shift2, mix2), cur_xfade);
        osc1.advance();
        osc2.advance();
        shift1 += shift_delta1;
        shift2 += shift_delta2;
        stretch1 += stretch_delta1;
        cur_xfade += xfade_step;
    }
    last_xfade = new_xfade;
}

void monosynth_audio_module::calculate_buffer_ser()
{
    filter.big_step(1.0 / step_size);
    filter2.big_step(1.0 / step_size);
    for (uint32_t i = 0; i < step_size; i++) 
    {
        float wave = buffer[i] * fgain;
        wave = filter.process(wave);
        wave = filter2.process(wave);
        buffer[i] = wave;
        fgain += fgain_delta;
    }
}

void monosynth_audio_module::calculate_buffer_single()
{
    filter.big_step(1.0 / step_size);
    for (uint32_t i = 0; i < step_size; i++) 
    {
        float wave = buffer[i] * fgain;
        wave = filter.process(wave);
        buffer[i] = wave;
        fgain += fgain_delta;
    }
}

void monosynth_audio_module::calculate_buffer_stereo()
{
    filter.big_step(1.0 / step_size);
    filter2.big_step(1.0 / step_size);
    for (uint32_t i = 0; i < step_size; i++) 
    {
        float wave1 = buffer[i] * fgain;
        buffer[i] = fgain * filter.process(wave1);
        buffer2[i] = fgain * filter2.process(wave1);
        fgain += fgain_delta;
    }
}

void monosynth_audio_module::lookup_waveforms()
{
    osc1.waveform = waves[wave1 == wave_sqr ? wave_saw : wave1].get_level((uint32_t)(((uint64_t)osc1.phasedelta) * last_stretch1 >> 16));
    osc2.waveform = waves[wave2 == wave_sqr ? wave_saw : wave2].get_level(osc2.phasedelta);    
    if (!osc1.waveform) osc1.waveform = silence;
    if (!osc2.waveform) osc2.waveform = silence;
    prev_wave1 = wave1;
    prev_wave2 = wave2;
}

void monosynth_audio_module::delayed_note_on()
{
    force_fadeout = false;
    fadeout.reset_soft();
    fadeout2.reset_soft();
    porta_time = 0.f;
    start_freq = freq;
    target_freq = freq = 440 * pow(2.0, (queue_note_on - 69) / 12.0);
    velocity = queue_vel;
    ampctl = 1.0 + (queue_vel - 1.0) * *params[par_vel2amp];
    fltctl = 1.0 + (queue_vel - 1.0) * *params[par_vel2filter];
    bool starting = false;

    if (!running)
    {
        starting = true;
        if (legato >= 2)
            porta_time = -1.f;
        last_xfade = xfade;
        osc1.reset();
        osc2.reset();
        filter.reset();
        filter2.reset();
        if (*params[par_lfo1trig] <= 0)
            lfo1.reset();
        if (*params[par_lfo2trig] <= 0)
            lfo2.reset();
        switch((int)*params[par_oscmode])
        {
        case 1:
            osc2.phase = 0x80000000;
            break;
        case 2:
            osc2.phase = 0x40000000;
            break;
        case 3:
            osc1.phase = osc2.phase = 0x40000000;
            break;
        case 4:
            osc1.phase = 0x40000000;
            osc2.phase = 0xC0000000;
            break;
        case 5:
            // rand() is crap, but I don't have any better RNG in Calf yet
            osc1.phase = rand() << 16;
            osc2.phase = rand() << 16;
            break;
        default:
            break;
        }
        running = true;
    }
    if (legato >= 2 && !gate)
        porta_time = -1.f;
    gate = true;
    stopping = false;
    if (starting || !(legato & 1) || envelope1.released())
        envelope1.note_on();
    if (starting || !(legato & 1) || envelope2.released())
        envelope2.note_on();
    envelope1.advance();
    envelope2.advance();
    queue_note_on = -1;
    float modsrc[modsrc_count] = {
        1,
        velocity,
        inertia_pressure.get_last(),
        modwheel_value,
        (float)envelope1.value,
        (float)envelope2.value,
        (float)(0.5+0.5*lfo1.last),
        (float)(0.5+0.5*lfo2.last)
    };
    calculate_modmatrix(moddest, moddest_count, modsrc);
    set_frequency();
    lookup_waveforms();
        
    if (queue_note_on_and_off)
    {
        end_note();
        queue_note_on_and_off = false;
    }
}

void monosynth_audio_module::set_sample_rate(uint32_t sr) {
    srate = sr;
    crate = sr / step_size;
    odcr = (float)(1.0 / crate);
    fgain = 0.f;
    fgain_delta = 0.f;
    inertia_cutoff.ramp.set_length(crate / 30); // 1/30s    
    inertia_pitchbend.ramp.set_length(crate / 30); // 1/30s    
    master.set_sample_rate(sr);
}

void monosynth_audio_module::calculate_step()
{
    if (queue_note_on != -1)
        delayed_note_on();
    else
    if (stopping || !running)
    {
        running = false;
        envelope1.advance();
        envelope2.advance();
        lfo1.get();
        lfo2.get();
        float modsrc[modsrc_count] = {
            1,
            velocity,
            inertia_pressure.get_last(),
            modwheel_value,
            (float)envelope1.value,
            (float)envelope2.value,
            (float)(0.5+0.5*lfo1.last),
            (float)(0.5+0.5*lfo2.last)
        };
        calculate_modmatrix(moddest, moddest_count, modsrc);
        last_stretch1 = (int32_t)(65536 * dsp::clip(*params[par_stretch1] + 0.01f * moddest[moddest_o1stretch], 1.f, 16.f));
        return;
    }
    lfo1.set_freq(*params[par_lforate], crate);
    lfo2.set_freq(*params[par_lfo2rate], crate);
    float porta_total_time = *params[par_portamento] * 0.001f;
    
    if (porta_total_time >= 0.00101f && porta_time >= 0) {
        // XXXKF this is criminal, optimize!
        float point = porta_time / porta_total_time;
        if (point >= 1.0f) {
            freq = target_freq;
            porta_time = -1;
        } else {
            freq = start_freq + (target_freq - start_freq) * point;
            // freq = start_freq * pow(target_freq / start_freq, point);
            porta_time += odcr;
        }
    }
    float lfov1 = lfo1.get() * std::min(1.0f, lfo_clock / *params[par_lfodelay]);
    lfov1 = lfov1 * dsp::lerp(1.f, modwheel_value, *params[par_mwhl_lfo]);
    float lfov2 = lfo2.get() * std::min(1.0f, lfo_clock / *params[par_lfo2delay]);
    lfo_clock += odcr;
    if (fabs(*params[par_lfopitch]) > small_value<float>())
        lfo_bend = pow(2.0f, *params[par_lfopitch] * lfov1 * (1.f / 1200.0f));
    inertia_pitchbend.step();
    envelope1.advance();
    envelope2.advance();
    float env1 = envelope1.value, env2 = envelope2.value;
    float aenv1 = envelope1.get_amp_value(), aenv2 = envelope2.get_amp_value();
    
    // mod matrix
    // this should be optimized heavily; I think I'll do it when MIDI in Ardour 3 gets stable :>
    float modsrc[modsrc_count] = {
        1,
        velocity,
        inertia_pressure.get(),
        modwheel_value,
        (float)env1,
        (float)env2,
        (float)(0.5+0.5*lfov1),
        (float)(0.5+0.5*lfov2)
    };
    calculate_modmatrix(moddest, moddest_count, modsrc);
    
    set_frequency();
    inertia_cutoff.set_inertia(*params[par_cutoff]);
    cutoff = inertia_cutoff.get() * pow(2.0f, (lfov1 * *params[par_lfofilter] + env1 * fltctl * *params[par_env1tocutoff] + env2 * fltctl * *params[par_env2tocutoff] + moddest[moddest_cutoff]) * (1.f / 1200.f));
    if (*params[par_keyfollow] > 0.01f)
        cutoff *= pow(freq / 264.f, *params[par_keyfollow]);
    cutoff = dsp::clip(cutoff , 10.f, 18000.f);
    float resonance = *params[par_resonance];
    float e2r1 = *params[par_env1tores];
    resonance = resonance * (1 - e2r1) + (0.7 + (resonance - 0.7) * env1 * env1) * e2r1;
    float e2r2 = *params[par_env2tores];
    resonance = resonance * (1 - e2r2) + (0.7 + (resonance - 0.7) * env2 * env2) * e2r2 + moddest[moddest_resonance];
    float cutoff2 = dsp::clip(cutoff * separation, 10.f, 18000.f);
    float newfgain = 0.f;
    if (filter_type != last_filter_type)
    {
        filter.y2 = filter.y1 = filter.x2 = filter.x1 = filter.y1;
        filter2.y2 = filter2.y1 = filter2.x2 = filter2.x1 = filter2.y1;
        last_filter_type = filter_type;
    }
    switch(filter_type)
    {
    case flt_lp12:
        filter.set_lp_rbj(cutoff, resonance, srate);
        filter2.set_null();
        newfgain = min(0.7f, 0.7f / resonance) * ampctl;
        break;
    case flt_hp12:
        filter.set_hp_rbj(cutoff, resonance, srate);
        filter2.set_null();
        newfgain = min(0.7f, 0.7f / resonance) * ampctl;
        break;
    case flt_lp24:
        filter.set_lp_rbj(cutoff, resonance, srate);
        filter2.set_lp_rbj(cutoff2, resonance, srate);
        newfgain = min(0.5f, 0.5f / resonance) * ampctl;
        break;
    case flt_lpbr:
        filter.set_lp_rbj(cutoff, resonance, srate);
        filter2.set_br_rbj(cutoff2, 1.0 / resonance, srate);
        newfgain = min(0.5f, 0.5f / resonance) * ampctl;        
        break;
    case flt_hpbr:
        filter.set_hp_rbj(cutoff, resonance, srate);
        filter2.set_br_rbj(cutoff2, 1.0 / resonance, srate);
        newfgain = min(0.5f, 0.5f / resonance) * ampctl;        
        break;
    case flt_2lp12:
        filter.set_lp_rbj(cutoff, resonance, srate);
        filter2.set_lp_rbj(cutoff2, resonance, srate);
        newfgain = min(0.7f, 0.7f / resonance) * ampctl;
        break;
    case flt_bp6:
        filter.set_bp_rbj(cutoff, resonance, srate);
        filter2.set_null();
        newfgain = ampctl;
        break;
    case flt_2bp6:
        filter.set_bp_rbj(cutoff, resonance, srate);
        filter2.set_bp_rbj(cutoff2, resonance, srate);
        newfgain = ampctl;        
        break;
    }
    float e2a1 = *params[par_env1toamp];
    float e2a2 = *params[par_env2toamp];
    if (e2a1 > 0.f)
        newfgain *= aenv1;
    if (e2a2 > 0.f)
        newfgain *= aenv2;
    if (moddest[moddest_attenuation] != 0.f)
        newfgain *= dsp::clip<float>(1 - moddest[moddest_attenuation] * moddest[moddest_attenuation], 0.f, 1.f);
    fgain_delta = (newfgain - fgain) * (1.0 / step_size);
    calculate_buffer_oscs(lfov1);
    lfo1.last = lfov1;
    lfo2.last = lfov2;
    switch(filter_type)
    {
    case flt_lp24:
    case flt_lpbr:
    case flt_hpbr: // Oomek's wish
        calculate_buffer_ser();
        break;
    case flt_lp12:
    case flt_hp12:
    case flt_bp6:
        calculate_buffer_single();
        break;
    case flt_2lp12:
    case flt_2bp6:
        calculate_buffer_stereo();
        break;
    }
    apply_fadeout();
}

void monosynth_audio_module::apply_fadeout()
{
    if (fadeout.undoing)
    {
        fadeout.process(buffer2, step_size);
        if (is_stereo_filter())
            fadeout2.process(buffer2, step_size);
    }
    else
    {
        // stop the sound if the amplitude envelope is not running (if there's any)
        bool aenv1_on = *params[par_env1toamp] > 0.f, aenv2_on = *params[par_env2toamp] > 0.f;
        
        bool do_fadeout = force_fadeout;
        
        // if there's no amplitude envelope at all, the fadeout starts at key release
        if (!aenv1_on && !aenv2_on && !gate)
            do_fadeout = true;
        // if ENV1 modulates amplitude, the fadeout will start on ENV1 end too
        if (aenv1_on && envelope1.state == adsr::STOP)
            do_fadeout = true;
        // if ENV2 modulates amplitude, the fadeout will start on ENV2 end too
        if (aenv2_on && envelope2.state == adsr::STOP)
            do_fadeout = true;
        
        if (do_fadeout || fadeout.undoing || fadeout2.undoing)
        {
            fadeout.process(buffer, step_size);
            if (is_stereo_filter())
                fadeout2.process(buffer2, step_size);
            if (fadeout.done)
                stopping = true;
        }
    }
}

void monosynth_audio_module::note_on(int /*channel*/, int note, int vel)
{
    queue_note_on = note;
    queue_note_on_and_off = false;
    last_key = note;
    queue_vel = vel / 127.f;
    stack.push(note);
}

void monosynth_audio_module::note_off(int /*channel*/, int note, int vel)
{
    stack.pop(note);
    if (note == queue_note_on)
    {
        queue_note_on_and_off = true;
        return;
    }
    // If releasing the currently played note, try to get another one from note stack.
    if (note == last_key) {
        end_note();
    }
}

void monosynth_audio_module::end_note()
{
    if (stack.count())
    {
        int note;
        last_key = note = stack.nth(stack.count() - 1);
        start_freq = freq;
        target_freq = freq = dsp::note_to_hz(note);
        porta_time = 0;
        set_frequency();
        if (!(legato & 1)) {
            envelope1.note_on();
            envelope2.note_on();
            stopping = false;
            running = true;
        }
        return;
    }
    gate = false;
    envelope1.note_off();
    envelope2.note_off();
}

void monosynth_audio_module::channel_pressure(int /*channel*/, int value)
{
    inertia_pressure.set_inertia(value * (1.0 / 127.0));
}

void monosynth_audio_module::control_change(int /*channel*/, int controller, int value)
{
    switch(controller)
    {
        case 1:
            modwheel_value_int = (modwheel_value_int & 127) | (value << 7);
            modwheel_value = modwheel_value_int / 16383.0;
            break;
        case 33:
            modwheel_value_int = (modwheel_value_int & (127 << 7)) | value;
            modwheel_value = modwheel_value_int / 16383.0;
            break;
        case 120: // all sounds off
            force_fadeout = true;
            // fall through
        case 123: // all notes off
            gate = false;
            queue_note_on = -1;
            envelope1.note_off();
            envelope2.note_off();
            stack.clear();
            break;
    }
}

void monosynth_audio_module::deactivate()
{
    gate = false;
    running = false;
    stopping = false;
    envelope1.reset();
    envelope2.reset();
    stack.clear();
}

void monosynth_audio_module::set_frequency()
{
    float detune_scaled = (detune - 1); // * log(freq / 440);
    if (*params[par_scaledetune] > 0)
        detune_scaled *= pow(20.0 / freq, (double)*params[par_scaledetune]);
    float p1 = 1, p2 = 1;
    if (moddest[moddest_o1detune] != 0)
        p1 = pow(2.0, moddest[moddest_o1detune] * (1.0 / 1200.0));
    if (moddest[moddest_o2detune] != 0)
        p2 = pow(2.0, moddest[moddest_o2detune] * (1.0 / 1200.0));
    osc1.set_freq(freq * (1 - detune_scaled) * p1 * inertia_pitchbend.get_last() * lfo_bend, srate);
    osc2.set_freq(freq * (1 + detune_scaled) * p2 * inertia_pitchbend.get_last() * lfo_bend * xpose, srate);
}


void monosynth_audio_module::params_changed()
{
    float sf = 0.001f;
    envelope1.set(*params[par_env1attack] * sf, *params[par_env1decay] * sf, std::min(0.999f, *params[par_env1sustain]), *params[par_env1release] * sf, srate / step_size, *params[par_env1fade] * sf);
    envelope2.set(*params[par_env2attack] * sf, *params[par_env2decay] * sf, std::min(0.999f, *params[par_env2sustain]), *params[par_env2release] * sf, srate / step_size, *params[par_env2fade] * sf);
    filter_type = dsp::fastf2i_drm(*params[par_filtertype]);
    separation = pow(2.0, *params[par_cutoffsep] / 1200.0);
    wave1 = dsp::clip(dsp::fastf2i_drm(*params[par_wave1]), 0, (int)wave_count - 1);
    wave2 = dsp::clip(dsp::fastf2i_drm(*params[par_wave2]), 0, (int)wave_count - 1);
    detune = pow(2.0, *params[par_detune] / 1200.0);
    xpose = pow(2.0, *params[par_osc2xpose] / 12.0);
    xfade = *params[par_oscmix];
    legato = dsp::fastf2i_drm(*params[par_legato]);
    master.set_inertia(*params[par_master]);
    if (running)
        set_frequency();
    if (wave1 != prev_wave1 || wave2 != prev_wave2)
        lookup_waveforms();
}


uint32_t monosynth_audio_module::process(uint32_t offset, uint32_t nsamples, uint32_t inputs_mask, uint32_t outputs_mask)
{
    uint32_t op = offset;
    uint32_t op_end = offset + nsamples;
    int had_data = 0;
    while(op < op_end) {
        if (output_pos == 0) 
            calculate_step();
        if(op < op_end) {
            uint32_t ip = output_pos;
            uint32_t len = std::min(step_size - output_pos, op_end - op);
            if (running)
            {
                had_data = 3;
                if (is_stereo_filter())
                    for(uint32_t i = 0 ; i < len; i++) {
                        float vol = master.get();
                        outs[0][op + i] = buffer[ip + i] * vol;
                        outs[1][op + i] = buffer2[ip + i] * vol;
                    }
                else
                    for(uint32_t i = 0 ; i < len; i++)
                        outs[0][op + i] = outs[1][op + i] = buffer[ip + i] * master.get();
            }
            else 
            {
                dsp::zero(&outs[0][op], len);
                dsp::zero(&outs[1][op], len);
            }
            op += len;
            output_pos += len;
            if (output_pos == step_size)
                output_pos = 0;
        }
    }
        
    return had_data;
}

