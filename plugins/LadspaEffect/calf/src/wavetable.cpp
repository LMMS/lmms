/* Calf DSP Library
 * Example audio modules - wavetable synthesizer
 *
 * Copyright (C) 2009 Krzysztof Foltman
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

#include <config.h>

#if ENABLE_EXPERIMENTAL
    
#include <calf/giface.h>
#include <calf/modules_synths.h>
#include <iostream>

using namespace dsp;
using namespace calf_plugins;

wavetable_voice::wavetable_voice()
{
    sample_rate = -1;
}

void wavetable_voice::set_params_ptr(wavetable_audio_module *_parent, int _srate)
{
    parent = _parent;
    params = parent->params;
    sample_rate = _srate;
}

void wavetable_voice::reset()
{
    note = -1;
}

void wavetable_voice::note_on(int note, int vel)
{
    typedef wavetable_metadata md;
    this->note = note;
    velocity = vel / 127.0;
    amp.set(1.0);
    for (int i = 0; i < OscCount; i++) {
        oscs[i].reset();
        oscs[i].set_freq(note_to_hz(note, 0), sample_rate);
        last_oscshift[i] = 0;
    }
    int cr = sample_rate / BlockSize;
    for (int i = 0; i < EnvCount; i++) {
        envs[i].set(0.01, 0.1, 0.5, 1, cr);
        envs[i].note_on();
    }
    float modsrc[wavetable_metadata::modsrc_count] = { 1, velocity, parent->inertia_pressure.get_last(), parent->modwheel_value, envs[0].value, envs[1].value, envs[2].value};
    parent->calculate_modmatrix(moddest, md::moddest_count, modsrc);
    calc_derived_dests();

    float oscshift[2] = { moddest[md::moddest_o1shift], moddest[md::moddest_o2shift] };
    memcpy(last_oscshift, oscshift, sizeof(oscshift));
    memcpy(last_oscamp, cur_oscamp, sizeof(cur_oscamp));
}

void wavetable_voice::note_off(int vel)
{
    for (int i = 0; i < EnvCount; i++)
        envs[i].note_off();
}

void wavetable_voice::steal()
{
}

void wavetable_voice::render_block()
{
    typedef wavetable_metadata md;
    
    const float step = 1.f / BlockSize;

    float s = 0.001;
    float scl[EnvCount];
    int espc = md::par_eg2attack - md::par_eg1attack;
    for (int j = 0; j < EnvCount; j++) {
        int o = j*espc;
        envs[j].set(*params[md::par_eg1attack + o] * s, *params[md::par_eg1decay + o] * s, *params[md::par_eg1sustain + o], *params[md::par_eg1release + o] * s, sample_rate / BlockSize, *params[md::par_eg1fade + o] * s); 
        scl[j] = dsp::lerp(1.f, velocity, *params[md::par_eg1velscl + o]);; 
    }
    
    for (int i = 0; i < EnvCount; i++)
        envs[i].advance();    
    
    float modsrc[wavetable_metadata::modsrc_count] = { 1, velocity, parent->inertia_pressure.get_last(), parent->modwheel_value, envs[0].value, envs[1].value, envs[2].value};
    parent->calculate_modmatrix(moddest, md::moddest_count, modsrc);
    calc_derived_dests();

    int ospc = md::par_o2level - md::par_o1level;
    for (int j = 0; j < OscCount; j++) {
        oscs[j].tables = parent->tables[(int)*params[md::par_o1wave + j * ospc]];
        oscs[j].set_freq(note_to_hz(note, *params[md::par_o1transpose + j * ospc] * 100+ *params[md::par_o1detune + j * ospc] + moddest[md::moddest_o1detune]), sample_rate);
    }
        
    float oscshift[2] = { moddest[md::moddest_o1shift], moddest[md::moddest_o2shift] };
    float osstep[2] = { (oscshift[0] - last_oscshift[0]) * step, (oscshift[1] - last_oscshift[1]) * step };
    float oastep[2] = { (cur_oscamp[0] - last_oscamp[0]) * step, (cur_oscamp[1] - last_oscamp[1]) * step };
    for (int i = 0; i < BlockSize; i++) {        
        float value = 0.f;

        for (int j = 0; j < OscCount; j++) {
            float o = last_oscshift[j] * 0.01;
            value += last_oscamp[j] * oscs[j].get(dsp::clip(fastf2i_drm((o + *params[md::par_o1offset + j * ospc]) * 127.0 * 256), 0, 127 * 256));
            last_oscshift[j] += osstep[j];
            last_oscamp[j] += oastep[j];
        }
        
        output_buffer[i][0] = output_buffer[i][1] = value;
    }
    if (envs[0].stopped())
        released = true;
    memcpy(last_oscshift, oscshift, sizeof(oscshift));
    memcpy(last_oscamp, cur_oscamp, sizeof(cur_oscamp));
}

/////////////////////////////////////////////////////////////////////////////////////////////////////

static inline float sincl(float x, float clip)
{
    if (fabs(x) > clip)
        return 0;
    return sin(M_PI * x);
}

static inline float blip(float x, float center, float range)
{
    if (x < center - range || x > center + range)
        return 0;
    return 1 - fabs(x - center)/range;
}

static void interpolate_wt(int16_t table[129][256], int step)
{
    for (int i = 0; i < 128; i++)
    {
        if (!(i % step))
            continue;
        int prev = i - i % step;
        int next = prev + step;
        for (int j = 0; j < 256; j++)
        {
            table[i][j] = table[prev][j] + (i - prev) * (table[next][j] - table[prev][j]) / step;
        }
    }
}

wavetable_audio_module::wavetable_audio_module()
: mod_matrix_impl(mod_matrix_data, &mm_metadata)
, inertia_cutoff(1)
, inertia_pitchbend(1)
, inertia_pressure(64)
{
    panic_flag = false;
    modwheel_value = 0.;
    for (int i = 0; i < 129; i += 8)
    {
        for (int j = 0; j < 256; j++)
        {
            float ph = j * 2 * M_PI / 256;
            int harm = 1 + 2 * (i / 8);
            float ii = i / 128.0;
            float rezo1 = sin(harm * ph) * sin(ph);
            float rezo2 = sin((harm+1) * ph) * sin(ph * 2);
            float rezo3 = sin((harm+3) * ph) * sin(ph * 4);
            float rezo = (rezo1 + rezo2 + rezo3) / 3;
            float v = (sin (ph) + ii * ii * rezo) / 2;
            tables[0][i][j] = 32767 * v;
        }
    }
    interpolate_wt(tables[0], 8);
    for (int i = 0; i < 129; i += 4)
    {
        for (int j = 0; j < 256; j++)
        {
            float ph = j * 2 * M_PI / 256;
            int harm = 1 + (i / 4);
            float ii = i / 128.0;
            float h = sin(harm * ph);
            float rezo1 = h * sin(ph);
            float rezo2 = h * sin(ph * 2)/2;
            float rezo3 = h * sin(ph * 3)/3;
            float rezo4 = h * sin(ph * 4)/4;
            float rezo5 = h * sin(ph * 5)/5;
            float rezo = (rezo1 + rezo2 + rezo3 + rezo4 + rezo5) / 3;
            float v = sin (ph + ii * rezo);
            tables[1][i][j] = 32767 * v;
        }
    }
    interpolate_wt(tables[1], 4);
    for (int i = 0; i < 129; i++)
    {
        for (int j = 0; j < 256; j++)
        {
            float ph = j * 2 * M_PI / 256;
            float ii = (i & ~3) / 128.0;
            float ii2 = ((i & ~3) + 4) / 128.0;
            float peak = (32 * ii);
            float rezo1 = sin(floor(peak) * ph);
            float rezo2 = sin(floor(peak + 1) * ph);
            float widener = (0.5 + 0.3 * sin(ph) + 0.2 * sin (3 * ph));
            float v1 = 0.5 * sin (ph) + 0.5 * ii * ii * rezo1 * widener;
            float v2 = 0.5 * sin (ph) + 0.5 * ii2 * ii2 * rezo2 * widener;
            tables[wavetable_metadata::wt_rezo][i][j] = 32767 * lerp(v1, v2, (i & 3) / 4.0);
        }
    }
    for (int i = 0; i < 129; i++)
    {
        for (int j = 0; j < 256; j++)
        {
            float ph = j * 2 * M_PI / 256;
            float ii = i / 128.0;
            float v = (sin(ph) + ii * sin(ph + 2 * ii * sin(ph)) + ii * ii * sin(ph + 6 * ii * ii * sin(6 * ph)) + ii * ii * ii * ii * sin(ph + 11 * ii * ii * ii * ii * sin(11 * ph))) / 4;
            tables[wavetable_metadata::wt_metal][i][j] = 32767 * v;
        }
    }
    for (int i = 0; i < 129; i++)
    {
        for (int j = 0; j < 256; j++)
        {
            float ph = j * 2 * M_PI / 256;
            float ii = i / 128.0;
            float v = (sin(ph) + ii * sin(ph - 3 * ii * sin(ph)) + ii * ii * sin(5 * ph - 5 * ii * ii * ii * ii * sin(11 * ph))) / 3;
            tables[wavetable_metadata::wt_bell][i][j] = 32767 * v;
        }
    }
    for (int i = 0; i < 129; i++)
    {
        for (int j = 0; j < 256; j++)
        {
            float ph = j * 2 * M_PI / 256;
            float ii = i / 128.0;
            //float v = (sin(ph) + ii * sin(ph + 2 * ii * sin(ph)) + ii * ii * sin(ph + 3 * ii * ii * sin(3 * ph)) + ii * ii * ii * sin(ph + 5 * ii * ii * ii * sin(5 * ph))) / 4;
            float v = (sin(ph) + sin(ph - 3 * sin(ii * 5 - 2) * sin(ph)) + sin(ii * 4 - 1.3) * sin(5 * ph + 3 * ii * ii * sin(6 * ph))) / 3;
            tables[wavetable_metadata::wt_blah][i][j] = 32767 * v;
        }
    }
    for (int i = 0; i < 256; i++)
    {
        tables[wavetable_metadata::wt_pluck][128][i] = (i < 128) ? 32000 * fabs(sin(i / 32.0 * M_PI) * sin(i / 13.0 * M_PI) * sin(i / 19.0 * M_PI)) : 0;
    }
    for (int i = 127; i >= 0; i--)
    {
        int16_t *parent = tables[wavetable_metadata::wt_pluck][i + 1];
        float damp = 0.05;
        for (int j = 0; j < 256; j++)
        {
            tables[wavetable_metadata::wt_pluck][i][j] = (1 - 2*damp) * parent[j] + damp * parent[(j+1)&255] + damp * parent[(j+2)&255];// + 0.1 * parent[(j-1)&255]+ 0.1 * parent[(j-2)&255];
        }
    }
    for (int i = 0; i < 129; i++)
    {
        for (int j = 0; j < 256; j++)
        {
            float ph = j / 128.0 - 1.0;
            float ii = i / 128.0;
            float v = sincl(ph * (1 + 15 * ii), 1);
            tables[wavetable_metadata::wt_stretch][i][j] = 32767 * v;
        }
    }
    for (int i = 0; i < 129; i++)
    {
        for (int j = 0; j < 256; j++)
        {
            float ph = j / 128.0 - 1.0;
            float ii = i / 128.0;
            float v = sincl(ph * (1 + 15 * ii), 4) * sincl(j / 256.0, 1);
            tables[wavetable_metadata::wt_stretch2][i][j] = 32000 * v;
        }
    }
    for (int i = 0; i < 129; i++)
    {
        for (int j = 0; j < 256; j++)
        {
            float ph = j / 128.0 - 1.0;
            float ii = i / 128.0;
            float w = sincl(ph * (1 + 15 * ii), 4);
            float v = pow(w, 9) * sincl(j / 256.0, 1);
            tables[wavetable_metadata::wt_hardsync][i][j] = 32000 * v;
        }
    }
    for (int i = 0; i < 129; i++)
    {
        for (int j = 0; j < 256; j++)
        {
            float ph = j / 128.0 - 1.0;
            float ii = i / 128.0;
            float w = sincl(ph * (1 + 31 * ii), 3);
            float v = pow(w, 5) * sincl(j / 256.0, 1);
            tables[wavetable_metadata::wt_hardsync2][i][j] = 32000 * v;
        }
    }
    for (int i = 0; i < 129; i++)
    {
        for (int j = 0; j < 256; j++)
        {
            float ph = j / 128.0 - 1.0;
            float ii = i / 128.0;
            float w = sincl(ph * ph * (1 + 15 * ii), 2);
            float v = pow(w, 4) * sincl(j / 256.0, 1);
            tables[wavetable_metadata::wt_softsync][i][j] = 32000 * v;
        }
    }
    for (int i = 0; i < 129; i++)
    {
        for (int j = 0; j < 256; j++)
        {
            float ph = j * 2 * M_PI / 256;
            float ii = i / 128.0;
            float v = (sin(ph) + ii * sin(ph - 3 * ii * sin(ph)) + ii * ii * ii * sin(7 * ph - 2 * ii * ii * ii * ii * sin(13 * ph))) / 3;
            tables[wavetable_metadata::wt_bell2][i][j] = 32767 * v;
        }
    }
    for (int i = 0; i < 129; i++)
    {
        for (int j = 0; j < 256; j++)
        {
            float ph = j * 2 * M_PI / 256;
            float ii = i / 128.0;
            float v = (sin(ph) + ii * sin(ph - 3 * ii * sin(ph)) + ii * ii * ii * sin(9 * ph - ii * ii * sin(11 * ph))) / 3;
            tables[wavetable_metadata::wt_bell3][i][j] = 32767 * v;
        }
    }
    for (int i = 0; i < 129; i++)
    {
        for (int j = 0; j < 256; j++)
        {
            float ph = j * 2 * M_PI / 256;
            float ii = i / 128.0;
            float v = (sin(ph + ii * sin(ph - 3 * ii * sin(ph) + ii * ii * ii * sin(5 * ph - ii * ii * sin(7 * ph)))));
            tables[wavetable_metadata::wt_tine][i][j] = 32767 * v;
        }
    }
    for (int i = 0; i < 129; i++)
    {
        for (int j = 0; j < 256; j++)
        {
            float ph = j * 2 * M_PI / 256;
            float ii = i / 128.0;
            float v = (sin(ph + ii * sin(ph - 2 * ii * sin(ph) + ii * ii * ii * sin(3 * ph - ii * ii * sin(4 * ph)))));
            tables[wavetable_metadata::wt_tine2][i][j] = 32767 * v;
        }
    }
    for (int i = 0; i < 129; i++)
    {
        for (int j = 0; j < 256; j++)
        {
            float ph = j * 2 * M_PI / 256;
            float ph2 = j / 128.0 - 1;
            float ii = i / 128.0;
            float w = sincl(ph2 * (1 + 7 * ii * ii), 4) * pow(sincl(j / 256.0, 1), 2);
            float v = sin(ph + ii * sin(ph - 2 * ii * w));
            tables[wavetable_metadata::wt_clav][i][j] = 32767 * v;
        }
    }
    for (int i = 0; i < 129; i++)
    {
        for (int j = 0; j < 256; j++)
        {
            float ph = j * 2 * M_PI / 256;
            float ph2 = j / 128.0 - 1;
            float ii = i / 128.0;
            float w = sincl(ph2 * (1 + 7 * ii * ii), 6) * sincl(j / 256.0, 1);
            float v = sin(ph + ii * sin(3 * ph - 2 * ii * w));
            tables[wavetable_metadata::wt_clav2][i][j] = 32767 * v;
        }
    }
    /*
    for (int i = 0; i < 129; i++)
    {
        for (int j = 0; j < 256; j++)
        {
            float ph = j * 2 * M_PI / 256;
            float ph2 = j / 128.0 - 1;
            float ii = i / 128.0;
            float w = sincl(ph2 * (1 + 7 * ii * ii), 6) * pow(sincl(j / 256.0, 1), 1);
            float v = sin(ph + ii * ii * ii * sin(3 * ph - ii * ii * ii * w));
            tables[wavetable_metadata::wt_gtr][i][j] = 32767 * v;
        }
    }
    */
    for (int i = 0; i < 129; i++)
    {
        for (int j = 0; j < 256; j++)
        {
            float ph = j * 2 * M_PI / 256;
            float ii = i / 128.0;
            float ii2 = ii;
            float w = pow(sincl(j / 256.0, 1), 1);
            float v = sin(ph + ii2 * ii2 * ii2 * sin(3 * ph - ii2 * ii2 * ii2 * w * sin(ph + sin(3 * ph) + ii * sin(11 * ph) + ii * ii * sin(25 * ph))));
            tables[wavetable_metadata::wt_gtr][i][j] = 32767 * v;
        }
    }
    for (int i = 0; i < 129; i++)
    {
        for (int j = 0; j < 256; j++)
        {
            float ph = j * 2 * M_PI / 256;
            float ii = i / 128.0;
            float ii2 = dsp::clip(ii - 0.5, 0.0, 1.0);
            float w = pow(sincl(j / 256.0, 1), 1);
            float v = sin(ph + ii * ii * ii * sin(3 * ph - ii * ii * ii * w * sin(ph + sin(3 * ph + ii2 * sin(13 * ph)))));
            tables[wavetable_metadata::wt_gtr2][i][j] = 32767 * v;
        }
    }
    for (int i = 0; i < 129; i++)
    {
        for (int j = 0; j < 256; j++)
        {
            float ph = j * 2 * M_PI / 256;
            float ii = i / 128.0;
            float ii2 = dsp::clip(2 * (ii - 0.5), 0.0, 1.0);
            //float w = sincl(ph2 * (1 + 15 * ii2 * ii2), 4) * pow(sincl(j / 256.0, 1), 1);
            float w = pow(sincl(j / 256.0, 1), 1);
            float v = sin(ph + ii * sin(3 * ph - ii * w * sin(ph + sin(3 * ph + 0.5 * ii2 * sin(13 * ph + 0.5 * sin(4 * ph))))));
            tables[wavetable_metadata::wt_gtr3][i][j] = 32767 * v;
        }
    }
    for (int i = 0; i < 129; i++)
    {
        for (int j = 0; j < 256; j++)
        {
            float ph = j * 2 * M_PI / 256;
            float ii = i / 128.0;
            float ii2 = dsp::clip(2 * (ii - 0.5), 0.0, 1.0);
            //float w = sincl(ph2 * (1 + 15 * ii2 * ii2), 4) * pow(sincl(j / 256.0, 1), 1);
            float w = pow(sincl(j / 256.0, 1), 1);
            float v = sin(ph + ii * sin(3 * ph - ii * w * sin(2 * ph + sin(5 * ph + 0.5 * ii2 * sin(13 * ph + 0.5 * sin(4 * ph))))));
            tables[wavetable_metadata::wt_gtr4][i][j] = 32767 * v;
        }
    }
    for (int i = 0; i < 129; i++)
    {
        for (int j = 0; j < 256; j++)
        {
            float ph = j * 2 * M_PI / 256;
            float ii = i / 128.0;
            float ii2 = dsp::clip((ii - 0.25)/0.75, 0.0, 1.0);
            //float w = sincl(ph2 * (1 + 15 * ii2 * ii2), 4) * pow(sincl(j / 256.0, 1), 1);
            float w = pow(sincl(j / 256.0, 1), 3);
            float v = sin(ph + (ii + 0.05) * sin(3 * ph - 2 * ii * w * sin(5 * ph + sin(7 * ph + 0.5 * ii2 * sin(13 * ph + 0.5 * sin(11 * ph))))));
            tables[wavetable_metadata::wt_gtr5][i][j] = 32767 * v;
        }
    }
    for (int i = 0; i < 129; i++)
    {
        for (int j = 0; j < 256; j++)
        {
            float ph = j * 2 * M_PI / 256;
            float ii = i / 128.0;
            float w = pow(sincl(2 * (j / 256.0), 2), 3);
            float v = sin(ph + (ii + 0.05) * sin(7 * ph - 2 * ii * w * sin(11 * ph)));
            tables[wavetable_metadata::wt_reed][i][j] = 32767 * v;
        }
    }
    for (int i = 0; i < 129; i++)
    {
        for (int j = 0; j < 256; j++)
        {
            float ph = j * 2 * M_PI / 256;
            float ii = i / 128.0;
            float ii2 = dsp::clip((ii - 0.25)/0.75, 0.0, 1.0);
            float ii3 = dsp::clip((ii - 0.5)/0.5, 0.0, 1.0);
            float v = sin(ph + (ii + 0.05) * sin(ii * sin(2 * ph) - 2 * ii2 * sin(2 * ph + ii2 * sin(3 * ph)) + 3 * ii3 * sin(3 * ph)));
            tables[wavetable_metadata::wt_reed2][i][j] = 32767 * v;
        }
    }
    for (int i = 0; i < 129; i++)
    {
        for (int j = 0; j < 256; j++)
        {
            float ph = j * 2 * M_PI / 256;
            float ii = i / 128.0;
            float mod = 0;
            for (int k = 0; k < 13; k++)
            {
                mod += blip(i, k * 10, 30) * sin (ph * (5 + 3 * k) + ii * cos(ph * (2 + 2 * k)));
            }
            float v = sin(ph + ii * mod);
            tables[wavetable_metadata::wt_silver][i][j] = 32767 * v;
        }
    }
    for (int i = 0; i < 129; i++)
    {
        for (int j = 0; j < 256; j++)
        {
            float ph = j * 2 * M_PI / 256;
            float ii = i / 128.0;
            float mod = 0;
            for (int k = 0; k < 16; k++)
            {
                mod += 2 * blip(i, k * 8, k * 4 + 10) * cos (ph * (k + 1));
            }
            float v = sin(ph + ii * mod);
            tables[wavetable_metadata::wt_brass][i][j] = 32767 * v;
        }
    }
    for (int i = 0; i < 129; i++)
    {
        for (int j = 0; j < 256; j++)
        {
            float ph = j * 2 * M_PI / 256;
            float ii = i / 128.0;
            float mod = 0;
            for (int k = 0; k < 16; k++)
            {
                mod += 2 * blip(i, k * 8, 16) * cos (ph * (2 * k + 1));
            }
            float v = (sin(ph + ii * mod) + ii * sin(2 * ph + ii * mod)) / 2;
            tables[wavetable_metadata::wt_multi][i][j] = 32767 * v;
        }
    }
    for (int i = 0; i < 129; i ++)
    {
        float h = 1 + i / 16.0;
        for (int j = 0; j < 256; j++)
        {
            float ph = j * 2 * M_PI / 256;
            float v = sin(ph), tv = 1;
            for (int k = 1; k < 24; k++) {
                float amp = blip(i, k * 6, 20) / k;
                v += amp * sin((k + 1) * ph + h * sin(ph));
                tv += amp;
            }
            tables[wavetable_metadata::wt_multi2][i][j] = 32767 * v / tv;
        }
    }
}

void wavetable_audio_module::channel_pressure(int /*channel*/, int value)
{
    inertia_pressure.set_inertia(value * (1.0 / 127.0));
}

#endif
