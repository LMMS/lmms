/* Calf DSP Library
 * Oscillators
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
 * Boston, MA 02111-1307, USA.
 */

#ifndef CALF_OSC_H
#define CALF_OSC_H

#include "fft.h"
#include <map>

namespace dsp
{

/** Very simple, non-bandlimited saw oscillator. Should not be used for anything
 *  else than testing/prototyping. Unless get() function is replaced with something
 * with "proper" oscillator code, as the frequency setting function is fine.
 */
struct simple_oscillator
{
    /// Phase (from 0 to 0xFFFFFFFF)
    uint32_t phase;
    /// Per-sample phase delta (phase increment), equal to 2^32*freq/sr.
    uint32_t phasedelta;
    /// Reset oscillator phase to zero.
    void reset()
    {
        phase = 0;
    }
    /// Set phase delta based on oscillator frequency and sample rate.
    void set_freq(float freq, float sr)
    {
        phasedelta = (int)(freq * 65536.0 * 256.0 * 16.0 / sr) << 4;
    }
    /// Set phase delta based on oscillator frequency and inverse of sample rate.
    void set_freq_odsr(float freq, double odsr)
    {
        phasedelta = (int)(freq * 65536.0 * 256.0 * 16.0 * odsr) << 4;
    }
    inline float get()
    {
        float value = (phase >> 16 ) / 65535.0 - 0.5;
        phase += phasedelta;
        return value;
    }
};

/**
 * FFT-based bandlimiting helper class. Allows conversion between time and frequency domains and generating brickwall filtered
 * versions of a waveform given a pre-computed spectrum.
 * Waveform size must be a power of two, and template argument SIZE_BITS is log2 of waveform size.
 */
template<int SIZE_BITS>
struct bandlimiter
{
    enum { SIZE = 1 << SIZE_BITS };
    static dsp::fft<float, SIZE_BITS> &get_fft()
    {
        static dsp::fft<float, SIZE_BITS> fft;
        return fft;
    }
    
    std::complex<float> spectrum[SIZE];
    
    /// Import time domain waveform and calculate spectrum from it
    void compute_spectrum(float input[SIZE])
    {
        dsp::fft<float, SIZE_BITS> &fft = get_fft();
        std::complex<float> *data = new std::complex<float>[SIZE];
        for (int i = 0; i < SIZE; i++)
            data[i] = input[i];
        fft.calculate(data, spectrum, false);
        delete []data;
    }
    
    /// Generate the waveform from the contained spectrum.
    void compute_waveform(float output[SIZE])
    {
        dsp::fft<float, SIZE_BITS> &fft = get_fft();
        std::complex<float> *data = new std::complex<float>[SIZE];
        fft.calculate(spectrum, data, true);
        for (int i = 0; i < SIZE; i++)
            output[i] = data[i].real();
        delete []data;
    }
    
    /// remove DC offset of the spectrum (it usually does more harm than good!)
    void remove_dc()
    {
        spectrum[0] = 0.f;
    }
    
    /// Very basic bandlimiting (brickwall filter)
    /// might need to be improved much in future!
    void make_waveform(float output[SIZE], int cutoff, bool foldover = false)
    {
        dsp::fft<float, SIZE_BITS> &fft = get_fft();
        std::vector<std::complex<float> > new_spec, iffted;
        new_spec.resize(SIZE);
        iffted.resize(SIZE);
        // Copy original harmonics up to cutoff point
        new_spec[0] = spectrum[0];
        for (int i = 1; i < cutoff; i++)
            new_spec[i] = spectrum[i], 
            new_spec[SIZE - i] = spectrum[SIZE - i];
        // Fill the rest with zeros, optionally folding over harmonics over the
        // cutoff point into the lower octaves while halving the amplitude.
        // (I think it is almost nice for bell type waveforms when the original
        // waveform has few widely spread harmonics)
        if (foldover)
        {
            std::complex<float> fatt(0.5);
            cutoff /= 2;
            if (cutoff < 2)
                cutoff = 2;
            for (int i = SIZE / 2; i >= cutoff; i--)
            {
                new_spec[i / 2] += new_spec[i] * fatt;
                new_spec[SIZE - i / 2] += new_spec[SIZE - i] * fatt;
                new_spec[i] = 0.f,
                new_spec[SIZE - i] = 0.f;
            }
        }
        else
        {
            if (cutoff < 1)
                cutoff = 1;
            for (int i = cutoff; i < SIZE / 2; i++)
                new_spec[i] = 0.f,
                new_spec[SIZE - i] = 0.f;
        }
        // convert back to time domain (IFFT) and extract only real part
        fft.calculate(&new_spec.front(), &iffted.front(), true);
        for (int i = 0; i < SIZE; i++)
            output[i] = iffted[i].real();
    }
};

/// Set of bandlimited wavetables
template<int SIZE_BITS>
struct waveform_family: public std::map<uint32_t, float *>
{
    enum { SIZE = 1 << SIZE_BITS };
    using std::map<uint32_t, float *>::iterator;
    using std::map<uint32_t, float *>::end;
    using std::map<uint32_t, float *>::lower_bound;
    float original[SIZE];
    
    /// Fill the family using specified bandlimiter and original waveform. Optionally apply foldover. 
    /// Does not produce harmonics over specified limit (limit = (SIZE / 2) / min_number_of_harmonics)
    void make(bandlimiter<SIZE_BITS> &bl, float input[SIZE], bool foldover = false, uint32_t limit = SIZE / 2)
    {
        memcpy(original, input, sizeof(original));
        bl.compute_spectrum(input);
        make_from_spectrum(bl, foldover);
    }
    
    /// Fill the family using specified bandlimiter and spectrum contained within. Optionally apply foldover. 
    /// Does not produce harmonics over specified limit (limit = (SIZE / 2) / min_number_of_harmonics)
    void make_from_spectrum(bandlimiter<SIZE_BITS> &bl, bool foldover = false, uint32_t limit = SIZE / 2)
    {
        bl.remove_dc();
        
        uint32_t base = 1 << (32 - SIZE_BITS);
        uint32_t cutoff = SIZE / 2, top = SIZE / 2;
        float vmax = 0;
        for (unsigned int i = 0; i < cutoff; i++)
            vmax = std::max(vmax, abs(bl.spectrum[i]));
        float vthres = vmax / 1024.0;  // -60dB
        float cumul = 0.f;
        while(cutoff > (SIZE / limit)) {
            if (!foldover)
            {
                // skip harmonics too quiet to be heard, but measure their loudness cumulatively,
                // because even if a single harmonic is too quiet, a whole bunch of them may add up 
                // to considerable amount of space
                cumul = 0.f;
                while(cutoff > 1 && cumul + abs(bl.spectrum[cutoff - 1]) < vthres)
                {
                    cumul += abs(bl.spectrum[cutoff - 1]);
                    cutoff--;
                }
            }
            float *wf = new float[SIZE+1];
            bl.make_waveform(wf, cutoff, foldover);
            wf[SIZE] = wf[0];
            (*this)[base * (top / cutoff)] = wf;
            cutoff = (int)(0.75 * cutoff);
        }
    }
    
    /// Retrieve waveform pointer suitable for specified phase_delta
    inline float *get_level(uint32_t phase_delta)
    {
        iterator i = upper_bound(phase_delta);
        if (i == end())
            return NULL;
        // printf("Level = %08x\n", i->first);
        return i->second;
    }
    /// Destructor, deletes the waveforms and removes them from the map.
    ~waveform_family()
    {
        for (iterator i = begin(); i != end(); i++)
            delete []i->second;
        clear();
    }
};

#if 0
// cubic interpolation
static inline float cerp(float pm1, float p0, float p1, float p2, float t)
{
    return (-t*(t-1)*(t-2) * pm1 + 3*(t+1)*(t-1)*(t-2) * p0 - 3*(t+1)*t*(t-2) * p1 + (t+1)*t*(t-1) * p2) * (1.0 / 6.0);
}
#endif
/**
 * Simple table-based lerping oscillator. Uses waveform of size 2^SIZE_BITS.
 * Combine with waveform_family if bandlimited waveforms are needed. Because
 * of linear interpolation, it's usually a good idea to use large tables
 * (2048-4096 points), otherwise aliasing may be produced.
 */
template<int SIZE_BITS>
struct waveform_oscillator: public simple_oscillator
{
    enum { SIZE = 1 << SIZE_BITS, MASK = SIZE - 1, SCALE = 1 << (32 - SIZE_BITS) };
    float *waveform;
    waveform_oscillator()
    {
        waveform = NULL;
    }
    
    /// Get the value from single oscillator at current position
    inline float get()
    {
        uint32_t wpos = phase >> (32 - SIZE_BITS);
        return dsp::lerp(waveform[wpos], waveform[(wpos + 1) & MASK], (phase & (SCALE - 1)) * (1.0f / SCALE));
    }
    /// Add/substract two phase-shifted values
    inline float get_phaseshifted(uint32_t shift, float mix)
    {
        uint32_t wpos = phase >> (32 - SIZE_BITS);
        float value1 = dsp::lerp(waveform[wpos], waveform[(wpos + 1) & MASK], (phase & (SCALE - 1)) * (1.0f / SCALE));
        wpos = (phase + shift) >> (32 - SIZE_BITS);
        float value2 = dsp::lerp(waveform[wpos], waveform[(wpos + 1) & MASK], ((phase + shift) & (SCALE - 1)) * (1.0f / SCALE));
        return value1 + mix * value2;
    }
    /// Get the value of a hard synced osc (65536 = 1:1 ratio)
    inline float get_phasedist(uint32_t sync, uint32_t shift, float mix)
    {
        uint32_t phase_mod = (uint64_t(phase) * sync >> 16);
        
        uint32_t wpos = phase_mod >> (32 - SIZE_BITS);
        float value1 = dsp::lerp(waveform[wpos], waveform[(wpos + 1) & MASK], (phase & (SCALE - 1)) * (1.0f / SCALE));
        wpos = (phase_mod + shift) >> (32 - SIZE_BITS);
        float value2 = dsp::lerp(waveform[wpos], waveform[(wpos + 1) & MASK], ((phase + shift) & (SCALE - 1)) * (1.0f / SCALE));
        return value1 + mix * value2;
    }
    /// One step
    inline void advance()
    {
        phase += phasedelta;
    }
};

/**
 * Simple triangle LFO without any smoothing or anything of this sort. 
 */
struct triangle_lfo: public simple_oscillator
{
    /// Previous value (not stored here, but may be used by calling code)
    float last;
    
    triangle_lfo()
    {
        reset();
    }
    void reset()
    {
        simple_oscillator::reset();
        last = 0;
    }
    inline float get()
    {
        uint32_t phase2 = phase;
        // start at 90 degrees point of the "/\" wave (-1 to +1)
        phase2 += 1<<30; 
        // if in second half, invert the wave (so it falls back into 0..0x7FFFFFFF)
        phase2 ^= ((int32_t)phase2)>>31;
        
        float value = (phase2 >> 6) / 16777216.0 - 1.0;
        phase += phasedelta;
        return value;
    }
};

/// Simple stupid inline function to normalize a waveform (by removing DC offset and ensuring max absolute value of 1).
static inline void normalize_waveform(float *table, unsigned int size)
{
    float dc = 0;
    for (unsigned int i = 0; i < size; i++)
        dc += table[i];
    dc /= size;
    for (unsigned int i = 0; i < size; i++)
        table[i] -= dc;
    float thismax = 0;
    for (unsigned int i = 0; i < size; i++)
        thismax = std::max(thismax, fabsf(table[i]));
    if (thismax < 0.000001f)
        return;
    double divv = 1.0 / thismax;
    for (unsigned int i = 0; i < size; i++)
        table[i] *= divv;
}



};

#endif
