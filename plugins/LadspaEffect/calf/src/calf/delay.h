/* Calf DSP Library
 * Reusable audio effect classes.
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
#ifndef __CALF_DELAY_H
#define __CALF_DELAY_H

#include "primitives.h"
#include "buffer.h"
#include "onepole.h"

namespace dsp {

/**
 * Delay primitive. Can be used for most delay stuff, including
 * variable (modulated) delays like chorus/flanger. Note that
 * for modulated delay effects use of GetInterp is preferred,
 * because it handles fractional positions and uses linear
 * interpolation, which sounds better most of the time.
 *
 * @param N maximum length
 * @param C number of channels read/written for each sample (1 mono, 2 stereo etc)
 */
template<int N, class T> 
struct simple_delay {
    auto_buffer<N, T> data;
    int pos;

    simple_delay() {
        reset();
    }
    void reset() {
        pos = 0;
        for (int i=0; i<N; i++)
            zero(data[i]);
    }    
    /** Write one C-channel sample from idata[0], idata[1] etc into buffer */
    inline void put(T idata) {
        data[pos] = idata;
        pos = wrap_around<N>(pos+1);
    }

    /** 
     * Read one C-channel sample into odata[0], odata[1] etc into buffer.
     * Don't use for modulated delays (chorus/flanger etc) unless you
     * want them to crackle and generally sound ugly
     * @param odata pointer to write into
     * @param delay delay relative to current writing pos
     */
    template<class U>
    inline void get(U &odata, int delay) {
        assert(delay >= 0 && delay < N);
        int ppos = wrap_around<N>(pos + N - delay);
        odata = data[ppos];
    }
    
    /**
     * Read and write during the same function call
     */
    inline T process(T idata, int delay)
    {
        assert(delay >= 0 && delay < N);
        int ppos = wrap_around<N>(pos + N - delay);
        T odata = data[ppos];
        data[pos] = idata;
        pos = wrap_around<N>(pos+1);
        return odata;
    }

    /** Read one C-channel sample at fractional position.
     * This version can be used for modulated delays, because
     * it uses linear interpolation.
     * @param odata value to write into
     * @param delay delay relative to current writing pos
     * @param udelay fractional delay (0..1)
     */
    template<class U>
    inline void get_interp(U &odata, int delay, float udelay) {
//        assert(delay >= 0 && delay <= N-1);
        int ppos = wrap_around<N>(pos + N - delay);
        int pppos = wrap_around<N>(ppos + N - 1);
        odata = lerp(data[ppos], data[pppos], udelay);
    }
    
    /** Read one C-channel sample at fractional position.
     * This version can be used for modulated delays, because
     * it uses linear interpolation.
     * @param odata value to write into
     * @param delay delay relative to current writing pos
     * @param udelay fractional delay (0..1)
     */
    inline T get_interp_1616(unsigned int delay) {
        float udelay = (float)((delay & 0xFFFF) * (1.0 / 65536.0));
        delay = delay >> 16;
//        assert(delay >= 0 && delay < N-1);
        int ppos = wrap_around<N>(pos + N - delay);
        int pppos = wrap_around<N>(ppos + N - 1);
        return lerp(data[ppos], data[pppos], udelay);
    }
    
    /**
     * Comb filter. Feedback delay line with given delay and feedback values
     * @param in input signal
     * @param delay delay length (must be <N and integer)
     * @param fb feedback (must be <1 or it will be unstable)
     */
    inline T process_comb(T in, unsigned int delay, float fb)
    {
        T old, cur;
        get(old, delay);
        cur = in + fb*old;
        sanitize(cur);
        put(cur);
        return old;
    }
    
    /**
     * Comb filter with linear interpolation. Feedback delay line with given delay and feedback values
     * Note that linear interpolation introduces some weird effects in frequency response.
     * @param in input signal
     * @param delay fractional delay length (must be < 65536 * N)
     * @param fb feedback (must be <1 or it will be unstable)
     */
    inline T process_comb_lerp16(T in, unsigned int delay, float udelay, float fb)
    {
        T old, cur;
        get_interp(old, delay>>16, dsp::fract16(delay));
        cur = in + fb*old;
        sanitize(cur);
        put(cur);
        return old;
    }
    
    /**
     * Comb allpass filter. The comb filter with additional direct path, which is supposed to cancel the coloration.
     * @param in input signal
     * @param delay delay length (must be <N and integer)
     * @param fb feedback (must be <1 or it will be unstable)
     */
    inline T process_allpass_comb(T in, unsigned int delay, float fb)
    {
        T old, cur;
        get(old, delay);
        cur = in + fb*old;
        sanitize(cur);
        put(cur);
        return old - fb * cur;
    }

        /**
     * Comb allpass filter. The comb filter with additional direct path, which is supposed to cancel the coloration.
     * @param in input signal
     * @param delay fractional delay length (must be < 65536 * N)
     * @param fb feedback (must be <1 or it will be unstable)
     */
    inline T process_allpass_comb_lerp16(T in, unsigned int delay, float fb)
    {
        T old, cur;
        get_interp(old, delay>>16, dsp::fract16(delay));
        cur = in + fb*old;
        sanitize(cur);
        put(cur);
        return old - fb * cur;
    }
};

};

#endif
