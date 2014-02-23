/* Calf DSP Library
 * Example audio modules - organ
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
#include <config.h>

#include <calf/giface.h>
#include <calf/organ.h>
#include <iostream>

using namespace std;
using namespace dsp;
using namespace calf_plugins;

//////////////////////////////////////////////////////////////////////////////////////////////////////////

organ_audio_module::organ_audio_module()
: drawbar_organ(&par_values)
{
    var_map_curve = "2\n0 1\n1 1\n"; // XXXKF hacky bugfix
}

void organ_audio_module::activate()
{
    setup(srate);
    panic_flag = false;
}

void organ_audio_module::post_instantiate()
{
    dsp::organ_voice_base::precalculate_waves(progress_report);
}


uint32_t organ_audio_module::process(uint32_t offset, uint32_t nsamples, uint32_t inputs_mask, uint32_t outputs_mask)
{
    float *o[2] = { outs[0] + offset, outs[1] + offset };
    if (panic_flag)
    {
        control_change(120, 0); // stop all sounds
        control_change(121, 0); // reset all controllers
        panic_flag = false;
    }
    render_separate(o, nsamples);
    return 3;
}

void organ_audio_module::params_changed() {
    for (int i = 0; i < param_count; i++)
        ((float *)&par_values)[i] = *params[i];

    unsigned int old_poly = polyphony_limit;
    polyphony_limit = dsp::clip(dsp::fastf2i_drm(*params[par_polyphony]), 1, 32);
    if (polyphony_limit < old_poly)
        trim_voices();
    
    update_params();
}

bool organ_audio_module::get_graph(int index, int subindex, float *data, int points, cairo_iface *context) const
{
    if (index == par_master) {
        organ_voice_base::precalculate_waves(progress_report);
        if (subindex)
            return false;
        float *waveforms[9];
        int S[9], S2[9];
        enum { small_waves = organ_voice_base::wave_count_small};
        for (int i = 0; i < 9; i++)
        {
            int wave = dsp::clip((int)(parameters->waveforms[i]), 0, (int)organ_voice_base::wave_count - 1);
            if (wave >= small_waves)
            {
                waveforms[i] = organ_voice_base::get_big_wave(wave - small_waves).original;
                S[i] = ORGAN_BIG_WAVE_SIZE;
                S2[i] = ORGAN_WAVE_SIZE / 64;
            }
            else
            {
                waveforms[i] = organ_voice_base::get_wave(wave).original;
                S[i] = S2[i] = ORGAN_WAVE_SIZE;
            }
        }
        for (int i = 0; i < points; i++)
        {
            float sum = 0.f;
            for (int j = 0; j < 9; j++)
            {
                float shift = parameters->phase[j] * S[j] / 360.0;
                sum += parameters->drawbars[j] * waveforms[j][int(parameters->harmonics[j] * i * S2[j] / points + shift) & (S[j] - 1)];
            }
            data[i] = sum * 2 / (9 * 8);
        }
        return true;
    }
    return false;
}

uint32_t organ_audio_module::message_run(const void *valid_inputs, void *output_ports)
{ 
    // silence a default printf (which is kind of a warning about unhandled message_run)
    return 0;
}


////////////////////////////////////////////////////////////////////////////

organ_voice_base::small_wave_family (*organ_voice_base::waves)[organ_voice_base::wave_count_small];
organ_voice_base::big_wave_family (*organ_voice_base::big_waves)[organ_voice_base::wave_count_big];

static void smoothen(bandlimiter<ORGAN_WAVE_BITS> &bl, float tmp[ORGAN_WAVE_SIZE])
{
    bl.compute_spectrum(tmp);
    for (int i = 1; i <= ORGAN_WAVE_SIZE / 2; i++) {
        bl.spectrum[i] *= 1.0 / sqrt(i);
        bl.spectrum[ORGAN_WAVE_SIZE - i] *= 1.0 / sqrt(i);
    }
    bl.compute_waveform(tmp);
    normalize_waveform(tmp, ORGAN_WAVE_SIZE);
}

static void phaseshift(bandlimiter<ORGAN_WAVE_BITS> &bl, float tmp[ORGAN_WAVE_SIZE])
{
    bl.compute_spectrum(tmp);
    for (int i = 1; i <= ORGAN_WAVE_SIZE / 2; i++) {
        float frac = i * 2.0 / ORGAN_WAVE_SIZE;
        float phase = M_PI / sqrt(frac) ;
        complex<float> shift = complex<float>(cos(phase), sin(phase));
        bl.spectrum[i] *= shift;
        bl.spectrum[ORGAN_WAVE_SIZE - i] *= conj(shift);
    }
    bl.compute_waveform(tmp);
    normalize_waveform(tmp, ORGAN_WAVE_SIZE);
}

static void padsynth(bandlimiter<ORGAN_WAVE_BITS> blSrc, bandlimiter<ORGAN_BIG_WAVE_BITS> &blDest, organ_voice_base::big_wave_family &result, int bwscale = 20, float bell_factor = 0, bool foldover = false)
{
    // kept in a vector to avoid putting large arrays on stack
    vector<complex<float> >orig_spectrum;
    orig_spectrum.resize(ORGAN_WAVE_SIZE / 2);
    for (int i = 0; i < ORGAN_WAVE_SIZE / 2; i++) 
    {
        orig_spectrum[i] = blSrc.spectrum[i];
//        printf("@%d = %f\n", i, abs(orig_spectrum[i]));
    }
    
    int periods = (1 << ORGAN_BIG_WAVE_SHIFT) * ORGAN_BIG_WAVE_SIZE / ORGAN_WAVE_SIZE;
    for (int i = 0; i <= ORGAN_BIG_WAVE_SIZE / 2; i++) {
        blDest.spectrum[i] = 0;
    }
    int MAXHARM = (ORGAN_WAVE_SIZE >> (1 + ORGAN_BIG_WAVE_SHIFT));
    for (int i = 1; i <= MAXHARM; i++) {
        //float esc = 0.25 * (1 + 0.5 * log(i));
        float esc = 0.5;
        float amp = abs(blSrc.spectrum[i]);
        // fade out starting from half
        if (i >= MAXHARM / 2) {
            float pos = (i - MAXHARM/2) * 1.0 / (MAXHARM / 2);
            amp *= 1.0 - pos;
            amp *= 1.0 - pos;
        }
        int bw = 1 + 20 * i;
        float sum = 1;
        int delta = 1;
        if (bw > 20) delta = bw / 20;
        for (int j = delta; j <= bw; j+=delta)
        {
            float p = j * 1.0 / bw;
            sum += 2 * exp(-p * p * esc);
        }
        if (sum < 0.0001)
            continue;
        amp *= (ORGAN_BIG_WAVE_SIZE / ORGAN_WAVE_SIZE);
        amp /= sum;
        int orig = i * periods + bell_factor * cos(i);
        if (orig > 0 && orig < ORGAN_BIG_WAVE_SIZE / 2)
            blDest.spectrum[orig] += amp;
        for (int j = delta; j <= bw; j += delta)
        {
            float p = j * 1.0 / bw;
            float val = amp * exp(-p * p * esc);
            int dist = j * bwscale / 40;
            int pos = orig + dist;
            if (pos < 1 || pos >= ORGAN_BIG_WAVE_SIZE / 2)
                continue;
            int pos2 = orig - dist;
            if (pos2 < 1 || pos2 >= ORGAN_BIG_WAVE_SIZE / 2)
                continue;
            blDest.spectrum[pos] += val;
            if (j)
                blDest.spectrum[pos2] += val;
        }
    }
    for (int i = 1; i <= ORGAN_BIG_WAVE_SIZE / 2; i++) {
        float phase = M_PI * 2 * (rand() & 255) / 256;
        complex<float> shift = complex<float>(cos(phase), sin(phase));
        blDest.spectrum[i] *= shift;        
//      printf("@%d = %f\n", i, abs(blDest.spectrum[i]));
        
        blDest.spectrum[ORGAN_BIG_WAVE_SIZE - i] = conj(blDest.spectrum[i]);
    }
    // same as above - put large array on heap to avoid stack overflow in ingen
    vector<float> tmp;
    tmp.resize(ORGAN_BIG_WAVE_SIZE);
    float *ptmp = &tmp.front();
    blDest.compute_waveform(ptmp);
    normalize_waveform(ptmp, ORGAN_BIG_WAVE_SIZE);
    blDest.compute_spectrum(ptmp);
    
    // limit is 1/2 of the number of harmonics of the original wave
    result.make_from_spectrum(blDest, foldover, ORGAN_WAVE_SIZE >> (1 + ORGAN_BIG_WAVE_SHIFT));
    memcpy(result.original, result.begin()->second, sizeof(result.original));
    #if 0
    blDest.compute_waveform(result);
    normalize_waveform(result, ORGAN_BIG_WAVE_SIZE);
    result[ORGAN_BIG_WAVE_SIZE] = result[0];
    for (int i =0 ; i<ORGAN_BIG_WAVE_SIZE + 1; i++)
        printf("%f\n", result[i]);
    #endif
}

#define LARGE_WAVEFORM_PROGRESS() do { if (reporter) { progress += 100; reporter->report_progress(floor(progress / totalwaves), "Precalculating large waveforms"); } } while(0)

void organ_voice_base::update_pitch()
{
    float phase = dsp::midi_note_to_phase(note, 100 * parameters->global_transpose + parameters->global_detune, sample_rate_ref);
    dpphase.set((long int) (phase * parameters->percussion_harmonic * parameters->pitch_bend));
    moddphase.set((long int) (phase * parameters->percussion_fm_harmonic * parameters->pitch_bend));
}

void organ_voice_base::precalculate_waves(progress_report_iface *reporter)
{
    static bool inited = false;
    if (!inited)
    {
        static organ_voice_base::small_wave_family waves[organ_voice_base::wave_count_small];
        static organ_voice_base::big_wave_family big_waves[organ_voice_base::wave_count_big];
        organ_voice_base::waves = &waves;
        organ_voice_base::big_waves = &big_waves;
        
        float progress = 0.0;
        int totalwaves = 1 + wave_count_big;
        if (reporter)
            reporter->report_progress(0, "Precalculating small waveforms");
        float tmp[ORGAN_WAVE_SIZE];
        static bandlimiter<ORGAN_WAVE_BITS> bl;
        static bandlimiter<ORGAN_BIG_WAVE_BITS> blBig;
        for (int i = 0; i < ORGAN_WAVE_SIZE; i++)
            tmp[i] = sin(i * 2 * M_PI / ORGAN_WAVE_SIZE);
        waves[wave_sine].make(bl, tmp);
        for (int i = 0; i < ORGAN_WAVE_SIZE; i++)
            tmp[i] = (i < (ORGAN_WAVE_SIZE / 16)) ? 1 : 0;
        normalize_waveform(tmp, ORGAN_WAVE_SIZE);
        waves[wave_pulse].make(bl, tmp);
        for (int i = 0; i < ORGAN_WAVE_SIZE; i++)
            tmp[i] = i < (ORGAN_WAVE_SIZE / 2) ? sin(i * 2 * 2 * M_PI / ORGAN_WAVE_SIZE) : 0;
        waves[wave_sinepl1].make(bl, tmp);
        for (int i = 0; i < ORGAN_WAVE_SIZE; i++)
            tmp[i] = i < (ORGAN_WAVE_SIZE / 3) ? sin(i * 3 * 2 * M_PI / ORGAN_WAVE_SIZE) : 0;
        waves[wave_sinepl2].make(bl, tmp);
        for (int i = 0; i < ORGAN_WAVE_SIZE; i++)
            tmp[i] = i < (ORGAN_WAVE_SIZE / 4) ? sin(i * 4 * 2 * M_PI / ORGAN_WAVE_SIZE) : 0;
        waves[wave_sinepl3].make(bl, tmp);
        for (int i = 0; i < ORGAN_WAVE_SIZE; i++)
            tmp[i] = (i < (ORGAN_WAVE_SIZE / 2)) ? 1 : -1;
        normalize_waveform(tmp, ORGAN_WAVE_SIZE);
        waves[wave_sqr].make(bl, tmp);
        for (int i = 0; i < ORGAN_WAVE_SIZE; i++)
            tmp[i] = -1 + (i * 2.0 / ORGAN_WAVE_SIZE);
        normalize_waveform(tmp, ORGAN_WAVE_SIZE);
        waves[wave_saw].make(bl, tmp);
        
        for (int i = 0; i < ORGAN_WAVE_SIZE; i++)
            tmp[i] = (i < (ORGAN_WAVE_SIZE / 2)) ? 1 : -1;
        normalize_waveform(tmp, ORGAN_WAVE_SIZE);
        smoothen(bl, tmp);
        waves[wave_ssqr].make(bl, tmp);
        
        for (int i = 0; i < ORGAN_WAVE_SIZE; i++)
            tmp[i] = -1 + (i * 2.0 / ORGAN_WAVE_SIZE);
        normalize_waveform(tmp, ORGAN_WAVE_SIZE);
        smoothen(bl, tmp);
        waves[wave_ssaw].make(bl, tmp);

        for (int i = 0; i < ORGAN_WAVE_SIZE; i++)
            tmp[i] = (i < (ORGAN_WAVE_SIZE / 16)) ? 1 : 0;
        normalize_waveform(tmp, ORGAN_WAVE_SIZE);
        smoothen(bl, tmp);
        waves[wave_spls].make(bl, tmp);
        for (int i = 0; i < ORGAN_WAVE_SIZE; i++)
            tmp[i] = i < (ORGAN_WAVE_SIZE / 1.5) ? sin(i * 1.5 * 2 * M_PI / ORGAN_WAVE_SIZE) : 0;
        normalize_waveform(tmp, ORGAN_WAVE_SIZE);
        waves[wave_sinepl05].make(bl, tmp);
        for (int i = 0; i < ORGAN_WAVE_SIZE; i++)
            tmp[i] = i < (ORGAN_WAVE_SIZE / 1.5) ? (i < ORGAN_WAVE_SIZE / 3 ? -1 : +1) : 0;
        normalize_waveform(tmp, ORGAN_WAVE_SIZE);
        waves[wave_sqr05].make(bl, tmp);
        for (int i = 0; i < ORGAN_WAVE_SIZE; i++)
            tmp[i] = sin(i * M_PI / ORGAN_WAVE_SIZE);
        normalize_waveform(tmp, ORGAN_WAVE_SIZE);
        waves[wave_halfsin].make(bl, tmp);
        for (int i = 0; i < ORGAN_WAVE_SIZE; i++)
            tmp[i] = sin(i * 3 * M_PI / ORGAN_WAVE_SIZE);
        normalize_waveform(tmp, ORGAN_WAVE_SIZE);
        waves[wave_clvg].make(bl, tmp);
        for (int i = 0; i < ORGAN_WAVE_SIZE; i++)
        {
            float ph = i * 2 * M_PI / ORGAN_WAVE_SIZE;
            float fm = 0.3 * sin(6*ph) + 0.2 * sin(11*ph) + 0.2 * cos(17*ph) - 0.2 * cos(19*ph);
            tmp[i] = sin(5*ph + fm) + 0.7 * cos(7*ph - fm);
        }
        normalize_waveform(tmp, ORGAN_WAVE_SIZE);
        waves[wave_bell].make(bl, tmp, true);
        for (int i = 0; i < ORGAN_WAVE_SIZE; i++)
        {
            float ph = i * 2 * M_PI / ORGAN_WAVE_SIZE;
            float fm = 0.3 * sin(3*ph) + 0.3 * sin(11*ph) + 0.3 * cos(17*ph) - 0.3 * cos(19*ph)  + 0.3 * cos(25*ph)  - 0.3 * cos(31*ph) + 0.3 * cos(37*ph);
            tmp[i] = sin(3*ph + fm) + cos(7*ph - fm);
        }
        normalize_waveform(tmp, ORGAN_WAVE_SIZE);
        waves[wave_bell2].make(bl, tmp, true);
        for (int i = 0; i < ORGAN_WAVE_SIZE; i++)
        {
            float ph = i * 2 * M_PI / ORGAN_WAVE_SIZE;
            float fm = 0.5 * sin(3*ph) + 0.3 * sin(5*ph) + 0.3 * cos(6*ph) - 0.3 * cos(9*ph);
            tmp[i] = sin(4*ph + fm) + cos(ph - fm);
        }
        normalize_waveform(tmp, ORGAN_WAVE_SIZE);
        waves[wave_w1].make(bl, tmp);
        for (int i = 0; i < ORGAN_WAVE_SIZE; i++)
        {
            float ph = i * 2 * M_PI / ORGAN_WAVE_SIZE;
            tmp[i] = sin(ph) * sin(2 * ph) * sin(4 * ph) * sin(8 * ph);
        }
        normalize_waveform(tmp, ORGAN_WAVE_SIZE);
        waves[wave_w2].make(bl, tmp);
        for (int i = 0; i < ORGAN_WAVE_SIZE; i++)
        {
            float ph = i * 2 * M_PI / ORGAN_WAVE_SIZE;
            tmp[i] = sin(ph) * sin(3 * ph) * sin(5 * ph) * sin(7 * ph) * sin(9 * ph);
        }
        normalize_waveform(tmp, ORGAN_WAVE_SIZE);
        waves[wave_w3].make(bl, tmp);
        for (int i = 0; i < ORGAN_WAVE_SIZE; i++)
        {
            float ph = i * 2 * M_PI / ORGAN_WAVE_SIZE;
            tmp[i] = sin(ph + 2 * sin(ph + 2 * sin(ph)));
        }
        normalize_waveform(tmp, ORGAN_WAVE_SIZE);
        waves[wave_w4].make(bl, tmp);
        for (int i = 0; i < ORGAN_WAVE_SIZE; i++)
        {
            float ph = i * 2 * M_PI / ORGAN_WAVE_SIZE;
            tmp[i] = ph * sin(ph);
        }
        normalize_waveform(tmp, ORGAN_WAVE_SIZE);
        waves[wave_w5].make(bl, tmp);
        for (int i = 0; i < ORGAN_WAVE_SIZE; i++)
        {
            float ph = i * 2 * M_PI / ORGAN_WAVE_SIZE;
            tmp[i] = ph * sin(ph) + (2 * M_PI - ph) * sin(2 * ph);
        }
        normalize_waveform(tmp, ORGAN_WAVE_SIZE);
        waves[wave_w6].make(bl, tmp);
        for (int i = 0; i < ORGAN_WAVE_SIZE; i++)
        {
            float ph = i * 1.0 / ORGAN_WAVE_SIZE;
            tmp[i] = exp(-ph * ph);
        }
        normalize_waveform(tmp, ORGAN_WAVE_SIZE);
        waves[wave_w7].make(bl, tmp);
        for (int i = 0; i < ORGAN_WAVE_SIZE; i++)
        {
            float ph = i * 1.0 / ORGAN_WAVE_SIZE;
            tmp[i] = exp(-ph * sin(2 * M_PI * ph));
        }
        normalize_waveform(tmp, ORGAN_WAVE_SIZE);
        waves[wave_w8].make(bl, tmp);
        for (int i = 0; i < ORGAN_WAVE_SIZE; i++)
        {
            float ph = i * 1.0 / ORGAN_WAVE_SIZE;
            tmp[i] = sin(2 * M_PI * ph * ph);
        }
        normalize_waveform(tmp, ORGAN_WAVE_SIZE);
        waves[wave_w9].make(bl, tmp);

        for (int i = 0; i < ORGAN_WAVE_SIZE; i++)
            tmp[i] = -1 + (i * 2.0 / ORGAN_WAVE_SIZE);
        normalize_waveform(tmp, ORGAN_WAVE_SIZE);
        phaseshift(bl, tmp);
        waves[wave_dsaw].make(bl, tmp);

        for (int i = 0; i < ORGAN_WAVE_SIZE; i++)
            tmp[i] = (i < (ORGAN_WAVE_SIZE / 2)) ? 1 : -1;
        normalize_waveform(tmp, ORGAN_WAVE_SIZE);
        phaseshift(bl, tmp);
        waves[wave_dsqr].make(bl, tmp);

        for (int i = 0; i < ORGAN_WAVE_SIZE; i++)
            tmp[i] = (i < (ORGAN_WAVE_SIZE / 16)) ? 1 : 0;
        normalize_waveform(tmp, ORGAN_WAVE_SIZE);
        phaseshift(bl, tmp);
        waves[wave_dpls].make(bl, tmp);

        LARGE_WAVEFORM_PROGRESS();
        for (int i = 0; i < ORGAN_WAVE_SIZE; i++)
            tmp[i] = -1 + (i * 2.0 / ORGAN_WAVE_SIZE);
        normalize_waveform(tmp, ORGAN_WAVE_SIZE);
        bl.compute_spectrum(tmp);
        padsynth(bl, blBig, big_waves[wave_strings - wave_count_small], 15);

        LARGE_WAVEFORM_PROGRESS();
        for (int i = 0; i < ORGAN_WAVE_SIZE; i++)
            tmp[i] = -1 + (i * 2.0 / ORGAN_WAVE_SIZE);
        normalize_waveform(tmp, ORGAN_WAVE_SIZE);
        bl.compute_spectrum(tmp);
        padsynth(bl, blBig, big_waves[wave_strings2 - wave_count_small], 40);

        LARGE_WAVEFORM_PROGRESS();
        for (int i = 0; i < ORGAN_WAVE_SIZE; i++)
            tmp[i] = sin(i * 2 * M_PI / ORGAN_WAVE_SIZE);
        normalize_waveform(tmp, ORGAN_WAVE_SIZE);
        bl.compute_spectrum(tmp);
        padsynth(bl, blBig, big_waves[wave_sinepad - wave_count_small], 20);

        LARGE_WAVEFORM_PROGRESS();
        for (int i = 0; i < ORGAN_WAVE_SIZE; i++)
        {
            float ph = i * 2 * M_PI / ORGAN_WAVE_SIZE;
            float fm = 0.3 * sin(6*ph) + 0.2 * sin(11*ph) + 0.2 * cos(17*ph) - 0.2 * cos(19*ph);
            tmp[i] = sin(5*ph + fm) + 0.7 * cos(7*ph - fm);
        }
        normalize_waveform(tmp, ORGAN_WAVE_SIZE);
        bl.compute_spectrum(tmp);
        padsynth(bl, blBig, big_waves[wave_bellpad - wave_count_small], 30, 30, true);

        LARGE_WAVEFORM_PROGRESS();
        for (int i = 0; i < ORGAN_WAVE_SIZE; i++)
        {
            float ph = i * 2 * M_PI / ORGAN_WAVE_SIZE;
            float fm = 0.3 * sin(3*ph) + 0.2 * sin(4*ph) + 0.2 * cos(5*ph) - 0.2 * cos(6*ph);
            tmp[i] = sin(2*ph + fm) + 0.7 * cos(3*ph - fm);
        }
        normalize_waveform(tmp, ORGAN_WAVE_SIZE);
        bl.compute_spectrum(tmp);
        padsynth(bl, blBig, big_waves[wave_space - wave_count_small], 30, 30);

        LARGE_WAVEFORM_PROGRESS();
        for (int i = 0; i < ORGAN_WAVE_SIZE; i++)
        {
            float ph = i * 2 * M_PI / ORGAN_WAVE_SIZE;
            float fm = 0.5 * sin(ph) + 0.5 * sin(2*ph) + 0.5 * sin(3*ph);
            tmp[i] = sin(ph + fm) + 0.5 * cos(7*ph - 2 * fm) + 0.25 * cos(13*ph - 4 * fm);
        }
        normalize_waveform(tmp, ORGAN_WAVE_SIZE);
        bl.compute_spectrum(tmp);
        padsynth(bl, blBig, big_waves[wave_choir - wave_count_small], 50, 10);

        LARGE_WAVEFORM_PROGRESS();
        for (int i = 0; i < ORGAN_WAVE_SIZE; i++)
        {
            float ph = i * 2 * M_PI / ORGAN_WAVE_SIZE;
            float fm = sin(ph) ;
            tmp[i] = sin(ph + fm) + 0.25 * cos(11*ph - 2 * fm) + 0.125 * cos(23*ph - 2 * fm) + 0.0625 * cos(49*ph - 2 * fm);
        }
        normalize_waveform(tmp, ORGAN_WAVE_SIZE);
        bl.compute_spectrum(tmp);
        padsynth(bl, blBig, big_waves[wave_choir2 - wave_count_small], 50, 10);

        LARGE_WAVEFORM_PROGRESS();
        for (int i = 0; i < ORGAN_WAVE_SIZE; i++)
        {
            float ph = i * 2 * M_PI / ORGAN_WAVE_SIZE;
            float fm = sin(ph) ;
            tmp[i] = sin(ph + 4 * fm) + 0.5 * sin(2 * ph + 4 * ph);
        }
        normalize_waveform(tmp, ORGAN_WAVE_SIZE);
        bl.compute_spectrum(tmp);
        padsynth(bl, blBig, big_waves[wave_choir3 - wave_count_small], 50, 10);
        LARGE_WAVEFORM_PROGRESS();
        
        inited = true;
    }
}

organ_voice_base::organ_voice_base(organ_parameters *_parameters, int &_sample_rate_ref, bool &_released_ref)
: parameters(_parameters)
, sample_rate_ref(_sample_rate_ref)
, released_ref(_released_ref)
{
    note = -1;
}

void organ_voice_base::render_percussion_to(float (*buf)[2], int nsamples)
{
    if (note == -1)
        return;

    if (!pamp.get_active())
        return;
    if (parameters->percussion_level < small_value<float>())
        return;
    float level = parameters->percussion_level * 9;
    static float zeros[ORGAN_WAVE_SIZE];
    // XXXKF the decay needs work!
    double age_const = parameters->perc_decay_const;
    double fm_age_const = parameters->perc_fm_decay_const;
    int timbre = parameters->get_percussion_wave();
    if (timbre < 0 || timbre >= wave_count_small)
        return;
    int timbre2 = parameters->get_percussion_fm_wave();
    if (timbre2 < 0 || timbre2 >= wave_count_small)
        timbre2 = wave_sine;
    float *fmdata = (*waves)[timbre2].get_level(moddphase.get());
    if (!fmdata)
        fmdata = zeros;
    float *data = (*waves)[timbre].get_level(dpphase.get());
    if (!data) {
        pamp.deactivate();
        return;
    }
    float s = parameters->percussion_stereo * ORGAN_WAVE_SIZE * (0.5 / 360.0);
    for (int i = 0; i < nsamples; i++) {
        float fm = wave(fmdata, modphase);
        fm *= ORGAN_WAVE_SIZE * parameters->percussion_fm_depth * fm_amp.get();
        modphase += moddphase;
        fm_amp.age_exp(fm_age_const, 1.0 / 32768.0);

        float lamp = level * pamp.get();
        buf[i][0] += lamp * wave(data, pphase + dsp::fixed_point<int64_t, 52>(fm - s));
        buf[i][1] += lamp * wave(data, pphase + dsp::fixed_point<int64_t, 52>(fm + s));
        if (released_ref)
            pamp.age_lin(rel_age_const,0.0);
        else
            pamp.age_exp(age_const, 1.0 / 32768.0);
        pphase += dpphase;
    }
}

void organ_voice_base::perc_reset()
{
    pphase = 0;
    modphase = 0;
    dpphase = 0;
    moddphase = 0;
    note = -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////

void organ_vibrato::reset()
{
    for (int i = 0; i < VibratoSize; i++)
        vibrato_x1[i][0] = vibrato_y1[i][0] = vibrato_x1[i][1] = vibrato_y1[i][1] = 0.f;
    vibrato[0].a0 = vibrato[1].a0 = 0;
    lfo_phase = 0.f;
}

void organ_vibrato::process(organ_parameters *parameters, float (*data)[2], unsigned int len, float sample_rate)
{
    float lfo1 = lfo_phase < 0.5 ? 2 * lfo_phase : 2 - 2 * lfo_phase;
    float lfo_phase2 = lfo_phase + parameters->lfo_phase * (1.0 / 360.0);
    if (lfo_phase2 >= 1.0)
        lfo_phase2 -= 1.0;
    float lfo2 = lfo_phase2 < 0.5 ? 2 * lfo_phase2 : 2 - 2 * lfo_phase2;
    lfo_phase += parameters->lfo_rate * len / sample_rate;
    if (lfo_phase >= 1.0)
        lfo_phase -= 1.0;
    if (!len)
        return;
    float olda0[2] = {vibrato[0].a0, vibrato[1].a0};
    vibrato[0].set_ap(3000 + 7000 * parameters->lfo_amt * lfo1 * lfo1, sample_rate);
    vibrato[1].set_ap(3000 + 7000 * parameters->lfo_amt * lfo2 * lfo2, sample_rate);
    float ilen = 1.0 / len;
    float deltaa0[2] = {(vibrato[0].a0 - olda0[0])*ilen, (vibrato[1].a0 - olda0[1])*ilen};
    
    float vib_wet = parameters->lfo_wet;
    for (int c = 0; c < 2; c++)
    {
        for (unsigned int i = 0; i < len; i++)
        {
            float v = data[i][c];
            float v0 = v;
            float coeff = olda0[c] + deltaa0[c] * i;
            for (int t = 0; t < VibratoSize; t++)
                v = vibrato[c].process_ap(v, vibrato_x1[t][c], vibrato_y1[t][c], coeff);
            
            data[i][c] += (v - v0) * vib_wet;
        }
        for (int t = 0; t < VibratoSize; t++)
        {
            sanitize(vibrato_x1[t][c]);
            sanitize(vibrato_y1[t][c]);
        }
    }
}

void scanner_vibrato::reset()
{
    legacy.reset();
    for (int i = 0; i < ScannerSize; i++)
        scanner[i].reset();
    lfo_phase = 0.f;
}

void scanner_vibrato::process(organ_parameters *parameters, float (*data)[2], unsigned int len, float sample_rate)
{
    if (!len)
        return;
    
    int vtype = (int)parameters->lfo_type;
    if (!vtype || vtype > organ_enums::lfotype_cvfull)
    {
        legacy.process(parameters, data, len, sample_rate);
        return;
    }
    
    // I bet the original components of the line box had some tolerance,
    // hence two different values of cutoff frequency
    scanner[0].set_lp_rbj(4000, 0.707, sample_rate);
    scanner[1].set_lp_rbj(4200, 0.707, sample_rate);
    for (int t = 2; t < ScannerSize; t ++)
    {
        scanner[t].copy_coeffs(scanner[t & 1]);
    }
    
    float lfo_phase2 = lfo_phase + parameters->lfo_phase * (1.0 / 360.0);
    if (lfo_phase2 >= 1.0)
        lfo_phase2 -= 1.0;
    float vib_wet = parameters->lfo_wet;
    float dphase = parameters->lfo_rate / sample_rate;
    static const int v1[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 8, 8 };
    static const int v2[] = { 0, 1, 2, 4, 6, 8, 9, 10, 12 };
    static const int v3[] = { 0, 1, 3, 6, 11, 12, 15, 17, 18, 18, 18 };
    static const int vfull[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 18 };
    static const int *vtypes[] = { NULL, v1, v2, v3, vfull };
    const int *vib = vtypes[vtype];
    
    float vibamt = 8 * parameters->lfo_amt;
    if (vtype == organ_enums::lfotype_cvfull)
        vibamt = 17 * parameters->lfo_amt;
    for (unsigned int i = 0; i < len; i++)
    {
        float line[ScannerSize + 1];
        float v0 = (data[i][0] + data[i][1]) * 0.5;
        
        line[0] = v0;
        for (int t = 0; t < ScannerSize; t++)
            line[t + 1] = scanner[t].process(line[t]) * 1.03;
        
        float lfo1 = lfo_phase < 0.5 ? 2 * lfo_phase : 2 - 2 * lfo_phase;
        float lfo2 = lfo_phase2 < 0.5 ? 2 * lfo_phase2 : 2 - 2 * lfo_phase2;
        
        float pos = vibamt * lfo1;
        int ipos = (int)pos;
        float vl = lerp(line[vib[ipos]], line[vib[ipos + 1]], pos - ipos);
        
        pos = vibamt * lfo2;
        ipos = (int)pos;
        float vr = lerp(line[vib[ipos]], line[vib[ipos + 1]], pos - ipos);
        
        lfo_phase += dphase;
        if (lfo_phase >= 1.0)
            lfo_phase -= 1.0;
        lfo_phase2 += dphase;
        if (lfo_phase2 >= 1.0)
            lfo_phase2 -= 1.0;
        
        data[i][0] += (vl - v0) * vib_wet;
        data[i][1] += (vr - v0) * vib_wet;
    }
    for (int t = 0; t < ScannerSize; t++)
    {
        scanner[t].sanitize();
    }
}
//////////////////////////////////////////////////////////////////////////////////////////////////////

void organ_voice::update_pitch()
{
    organ_voice_base::update_pitch();
    dphase.set(dsp::midi_note_to_phase(note, 100 * parameters->global_transpose + parameters->global_detune, sample_rate) * inertia_pitchbend.get_last());
}

void organ_voice::render_block() {
    if (note == -1)
        return;

    dsp::zero(&output_buffer[0][0], Channels * BlockSize);
    dsp::zero(&aux_buffers[1][0][0], 2 * Channels * BlockSize);
    if (!amp.get_active())
    {
        if (use_percussion())
            render_percussion_to(output_buffer, BlockSize);
        return;
    }

    inertia_pitchbend.set_inertia(parameters->pitch_bend);
    inertia_pitchbend.step();
    update_pitch();
    dsp::fixed_point<int, 20> tphase, tdphase;
    unsigned int foldvalue = parameters->foldvalue * inertia_pitchbend.get_last();
    int vibrato_mode = fastf2i_drm(parameters->lfo_mode);
    for (int h = 0; h < 9; h++)
    {
        float amp = parameters->drawbars[h];
        if (amp < small_value<float>())
            continue;
        float *data;
        dsp::fixed_point<int, 24> hm = dsp::fixed_point<int, 24>(parameters->multiplier[h]);
        int waveid = (int)parameters->waveforms[h];
        if (waveid < 0 || waveid >= wave_count)
            waveid = 0;

        uint32_t rate = (dphase * hm).get();
        if (waveid >= wave_count_small)
        {
            float *data = (*big_waves)[waveid - wave_count_small].get_level(rate >> (ORGAN_BIG_WAVE_BITS - ORGAN_WAVE_BITS + ORGAN_BIG_WAVE_SHIFT));
            if (!data)
                continue;
            hm.set(hm.get() >> ORGAN_BIG_WAVE_SHIFT);
            dsp::fixed_point<int64_t, 20> tphase, tdphase;
            tphase.set(((phase * hm).get()) + parameters->phaseshift[h]);
            tdphase.set(rate >> ORGAN_BIG_WAVE_SHIFT);
            float ampl = amp * 0.5f * (1 - parameters->pan[h]);
            float ampr = amp * 0.5f * (1 + parameters->pan[h]);
            float (*out)[Channels] = aux_buffers[dsp::fastf2i_drm(parameters->routing[h])];
            
            for (int i=0; i < (int)BlockSize; i++) {
                float wv = big_wave(data, tphase);
                out[i][0] += wv * ampl;
                out[i][1] += wv * ampr;
                tphase += tdphase;
            }
        }
        else
        {
            unsigned int foldback = 0;
            while (rate > foldvalue)
            {
                rate >>= 1;
                foldback++;
            }
            hm.set(hm.get() >> foldback);
            data = (*waves)[waveid].get_level(rate);
            if (!data)
                continue;
            tphase.set((uint32_t)((phase * hm).get()) + parameters->phaseshift[h]);
            tdphase.set((uint32_t)rate);
            float ampl = amp * 0.5f * (1 - parameters->pan[h]);
            float ampr = amp * 0.5f * (1 + parameters->pan[h]);
            float (*out)[Channels] = aux_buffers[dsp::fastf2i_drm(parameters->routing[h])];
            
            for (int i=0; i < (int)BlockSize; i++) {
                float wv = wave(data, tphase);
                out[i][0] += wv * ampl;
                out[i][1] += wv * ampr;
                tphase += tdphase;
            }
        }
    }
    
    bool is_quad = parameters->quad_env >= 0.5f;
    
    expression.set_inertia(parameters->cutoff);
    phase += dphase * BlockSize;
    float escl[EnvCount], eval[EnvCount];
    for (int i = 0; i < EnvCount; i++)
        escl[i] = (1.f + parameters->envs[i].velscale * (velocity - 1.f));
    
    if (is_quad)
    {
        for (int i = 0; i < EnvCount; i++)
            eval[i] = envs[i].value * envs[i].value * escl[i];
    }
    else
    {
        for (int i = 0; i < EnvCount; i++)
            eval[i] = envs[i].value * escl[i];
    }
    for (int i = 0; i < FilterCount; i++)
    {
        float mod = parameters->filters[i].envmod[0] * eval[0] ;
        mod += parameters->filters[i].keyf * 100 * (note - 60);
        for (int j = 1; j < EnvCount; j++)
        {
            mod += parameters->filters[i].envmod[j] * eval[j];
        }
        if (i) mod += expression.get() * 1200 * 4;
        float fc = parameters->filters[i].cutoff * pow(2.0f, mod * (1.f / 1200.f));
        if (i == 0 && parameters->filter1_type >= 0.5f)
            filterL[i].set_hp_rbj(dsp::clip<float>(fc, 10, 18000), parameters->filters[i].resonance, sample_rate);
        else
            filterL[i].set_lp_rbj(dsp::clip<float>(fc, 10, 18000), parameters->filters[i].resonance, sample_rate);
        filterR[i].copy_coeffs(filterL[i]);
    }
    float amp_pre[ampctl_count - 1], amp_post[ampctl_count - 1];
    for (int i = 0; i < ampctl_count - 1; i++)
    {
        amp_pre[i] = 1.f;
        amp_post[i] = 1.f;
    }
    bool any_running = false;
    for (int i = 0; i < EnvCount; i++)
    {
        float pre = eval[i];
        envs[i].advance();
        int mode = fastf2i_drm(parameters->envs[i].ampctl);
        if (!envs[i].stopped())
            any_running = true;
        if (mode == ampctl_none)
            continue;
        float post = (is_quad ? envs[i].value : 1) * envs[i].value * escl[i];
        amp_pre[mode - 1] *= pre;
        amp_post[mode - 1] *= post;
    }
    if (vibrato_mode >= lfomode_direct && vibrato_mode <= lfomode_filter2)
        vibrato.process(parameters, aux_buffers[vibrato_mode - lfomode_direct], BlockSize, sample_rate);
    if (!any_running)
        finishing = true;
    // calculate delta from pre and post
    for (int i = 0; i < ampctl_count - 1; i++)
        amp_post[i] = (amp_post[i] - amp_pre[i]) * (1.0 / BlockSize);
    float a0 = amp_pre[0], a1 = amp_pre[1], a2 = amp_pre[2], a3 = amp_pre[3];
    float d0 = amp_post[0], d1 = amp_post[1], d2 = amp_post[2], d3 = amp_post[3];
    if (parameters->filter_chain >= 0.5f)
    {
        for (int i=0; i < (int) BlockSize; i++) {
            output_buffer[i][0] = a3 * (a0 * output_buffer[i][0] + a2 * filterL[1].process(a1 * filterL[0].process(aux_buffers[1][i][0]) + aux_buffers[2][i][0]));
            output_buffer[i][1] = a3 * (a0 * output_buffer[i][1] + a2 * filterR[1].process(a1 * filterR[0].process(aux_buffers[1][i][1]) + aux_buffers[2][i][1]));
            a0 += d0, a1 += d1, a2 += d2, a3 += d3;
        }
    }
    else
    {
        for (int i=0; i < (int) BlockSize; i++) {
            output_buffer[i][0] = a3 * (a0 * output_buffer[i][0] + a1 * filterL[0].process(aux_buffers[1][i][0]) + a2 * filterL[1].process(aux_buffers[2][i][0]));
            output_buffer[i][1] = a3 * (a0 * output_buffer[i][1] + a1 * filterR[0].process(aux_buffers[1][i][1]) + a2 * filterR[1].process(aux_buffers[2][i][1]));
            a0 += d0, a1 += d1, a2 += d2, a3 += d3;
        }
    }
    filterL[0].sanitize();
    filterR[0].sanitize();
    filterL[1].sanitize();
    filterR[1].sanitize();
    if (vibrato_mode == lfomode_voice)
        vibrato.process(parameters, output_buffer, BlockSize, sample_rate);

    if (finishing)
    {
        for (int i = 0; i < (int) BlockSize; i++) {
            output_buffer[i][0] *= amp.get();
            output_buffer[i][1] *= amp.get();
            amp.age_lin((1.0/44100.0)/0.03,0.0);
        }
    }
    
    if (use_percussion())
        render_percussion_to(output_buffer, BlockSize);

}

void organ_voice::note_on(int note, int vel)
{
    stolen = false;
    finishing = false;
    perc_released = false;
    released = false;
    reset();
    this->note = note;
    const float sf = 0.001f;
    for (int i = 0; i < EnvCount; i++)
    {
        organ_parameters::organ_env_parameters &p = parameters->envs[i];
        envs[i].set(sf * p.attack, sf * p.decay, p.sustain, sf * p.release, sample_rate / BlockSize);
        envs[i].note_on();
    }
    update_pitch();
    velocity = vel * 1.0 / 127.0;
    amp.set(1.0f);
    perc_note_on(note, vel);
}

void organ_voice::note_off(int /* vel */)
{
    // reset age to 0 (because decay will turn from exponential to linear, necessary because of error cumulation prevention)
    perc_released = true;
    if (pamp.get_active())
    {
        pamp.reinit();
    }
    rel_age_const = pamp.get() * ((1.0/44100.0)/0.03);
    for (int i = 0; i < EnvCount; i++)
        envs[i].note_off();
}

void organ_voice::steal()
{
    perc_released = true;
    finishing = true;
    stolen = true;
}

void organ_voice::reset()
{
    inertia_pitchbend.ramp.set_length(sample_rate / (BlockSize * 30)); // 1/30s    
    vibrato.reset();
    phase = 0;
    for (int i = 0; i < FilterCount; i++)
    {
        filterL[i].reset();
        filterR[i].reset();
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void drawbar_organ::update_params()
{
    parameters->perc_decay_const = dsp::decay::calc_exp_constant(1.0 / 1024.0, 0.001 * parameters->percussion_time * sample_rate);
    parameters->perc_fm_decay_const = dsp::decay::calc_exp_constant(1.0 / 1024.0, 0.001 * parameters->percussion_fm_time * sample_rate);
    for (int i = 0; i < 9; i++)
    {
        parameters->multiplier[i] = parameters->harmonics[i] * pow(2.0, parameters->detune[i] * (1.0 / 1200.0));
        parameters->phaseshift[i] = int(parameters->phase[i] * 65536 / 360) << 16;
    }
    double dphase = dsp::midi_note_to_phase((int)parameters->foldover, 0, sample_rate);
    parameters->foldvalue = (int)(dphase);
}

dsp::voice *drawbar_organ::alloc_voice()
{
    block_voice<organ_voice> *v = new block_voice<organ_voice>();
    v->parameters = parameters;
    return v;
}

void drawbar_organ::percussion_note_on(int note, int vel)
{
    percussion.perc_note_on(note, vel);
}

void drawbar_organ::setup(int sr)
{
    basic_synth::setup(sr);
    percussion.setup(sr);
    parameters->cutoff = 0;
    params_changed();
    global_vibrato.reset();
}

bool drawbar_organ::check_percussion() { 
    switch(dsp::fastf2i_drm(parameters->percussion_trigger))
    {        
        case organ_voice_base::perctrig_first:
            return active_voices.empty();
        case organ_voice_base::perctrig_each: 
        default:
            return true;
        case organ_voice_base::perctrig_eachplus:
            return !percussion.get_noticable();
        case organ_voice_base::perctrig_polyphonic:
            return false;
    }
}

void drawbar_organ::pitch_bend(int amt)
{
    parameters->pitch_bend = pow(2.0, (amt * parameters->pitch_bend_range) / (1200.0 * 8192.0));
    for (list<voice *>::iterator i = active_voices.begin(); i != active_voices.end(); i++)
    {
        organ_voice *v = dynamic_cast<organ_voice *>(*i);
        v->update_pitch();
    }
    percussion.update_pitch();
}

void organ_audio_module::execute(int cmd_no)
{
    switch(cmd_no)
    {
        case 0:
            panic_flag = true;
            break;
    }
}

void organ_voice_base::perc_note_on(int note, int vel)
{
    perc_reset();
    released_ref = false;
    this->note = note;
    if (parameters->percussion_level > 0)
        pamp.set(1.0f + (vel - 127) * parameters->percussion_vel2amp / 127.0);
    update_pitch();
    float (*kt)[2] = parameters->percussion_keytrack;
    // assume last point (will be put there by padding)
    fm_keytrack = kt[ORGAN_KEYTRACK_POINTS - 1][1];
    // yes binary search would be nice if we had more than those crappy 4 points
    for (int i = 0; i < ORGAN_KEYTRACK_POINTS - 1; i++)
    {
        float &lower = kt[i][0], upper = kt[i + 1][0];
        if (note >= lower && note < upper)
        {
            fm_keytrack = kt[i][1] + (note - lower) * (kt[i + 1][1] - kt[i][1]) / (upper - lower);
            break;
        }
    }
    fm_amp.set(fm_keytrack * (1.0f + (vel - 127) * parameters->percussion_vel2fm / 127.0));
}

char *organ_audio_module::configure(const char *key, const char *value)
{
    if (!strcmp(key, "map_curve"))
    {
        if (!value)
            value = "2\n0 1\n1 1\n";
        var_map_curve = value;
        stringstream ss(value);
        int i = 0;
        float x = 0, y = 1;
        if (*value)
        {
            int points;
            ss >> points;
            for (i = 0; i < points; i++)
            {
                static const int whites[] = { 0, 2, 4, 5, 7, 9, 11 };
                ss >> x >> y;
                int wkey = (int)(x * 71);
                x = whites[wkey % 7] + 12 * (wkey / 7);
                parameters->percussion_keytrack[i][0] = x;
                parameters->percussion_keytrack[i][1] = y;
                // cout << "(" << x << ", " << y << ")" << endl;
            }
        }
        // pad with constant Y
        for (; i < ORGAN_KEYTRACK_POINTS; i++) {
            parameters->percussion_keytrack[i][0] = x;
            parameters->percussion_keytrack[i][1] = y;
        }
        return NULL;
    }
    cout << "Set unknown configure value " << key << " to " << value << endl;
    return NULL;
}

void organ_audio_module::send_configures(send_configure_iface *sci)
{
    sci->send_configure("map_curve", var_map_curve.c_str());
}

void organ_audio_module::deactivate()
{
    
}

void drawbar_organ::render_separate(float *output[], int nsamples)
{
    float buf[MAX_SAMPLE_RUN][2];
    dsp::zero(&buf[0][0], 2 * nsamples);
    basic_synth::render_to(buf, nsamples);
    if (dsp::fastf2i_drm(parameters->lfo_mode) == organ_voice_base::lfomode_global)
    {
        for (int i = 0; i < nsamples; i += 64)
            global_vibrato.process(parameters, buf + i, std::min(64, nsamples - i), sample_rate);
    }
    if (percussion.get_active())
        percussion.render_percussion_to(buf, nsamples);
    float gain = parameters->master * (1.0 / 8);
    eq_l.set(parameters->bass_freq, parameters->bass_gain, parameters->treble_freq, parameters->treble_gain, sample_rate);
    eq_r.copy_coeffs(eq_l);
    for (int i=0; i<nsamples; i++) {
        output[0][i] = gain*eq_l.process(buf[i][0]);
        output[1][i] = gain*eq_r.process(buf[i][1]);
    }
    eq_l.sanitize();
    eq_r.sanitize();
}
