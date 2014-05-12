/*
  ZynAddSubFX - a software synthesizer

  Unison.cpp - Unison effect (multivoice chorus)
  Copyright (C) 2002-2009 Nasca Octavian Paul
  Author: Nasca Octavian Paul

  This program is free software; you can redistribute it and/or modify
  it under the terms of version 2 of the GNU General Public License
  as published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License (version 2 or later) for more details.

  You should have received a copy of the GNU General Public License (version 2)
  along with this program; if not, write to the Free Software Foundation,
  Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
*/

#include <cmath>
#include <cstring>
#ifndef WIN32
#include <err.h>
#endif

#include "Unison.h"

Unison::Unison(int update_period_samples_, float max_delay_sec_, float srate_f)
    :unison_size(0),
      base_freq(1.0f),
      uv(NULL),
      update_period_samples(update_period_samples_),
      update_period_sample_k(0),
      max_delay((int)(srate_f * max_delay_sec_) + 1),
      delay_k(0),
      first_time(false),
      delay_buffer(NULL),
      unison_amplitude_samples(0.0f),
      unison_bandwidth_cents(10.0f),
      samplerate_f(srate_f)
{
    if(max_delay < 10)
        max_delay = 10;
    delay_buffer = new float[max_delay];
    memset(delay_buffer, 0, max_delay * sizeof(float));
    setSize(1);
}

Unison::~Unison() {
    delete [] delay_buffer;
    delete [] uv;
}

void Unison::setSize(int new_size)
{
    if(new_size < 1)
        new_size = 1;
    unison_size = new_size;
    if(uv)
        delete [] uv;
    uv = new UnisonVoice[unison_size];
    first_time = true;
    updateParameters();
}

void Unison::setBaseFrequency(float freq)
{
    base_freq = freq;
    updateParameters();
}

void Unison::setBandwidth(float bandwidth)
{
    if(bandwidth < 0)
        bandwidth = 0.0f;
    if(bandwidth > 1200.0f)
        bandwidth = 1200.0f;

    /* If the bandwidth is too small, the audio may cancel itself out
     * (due to the sign change of the outputs)
     * TODO figure out the acceptable lower bound and codify it
     */
    unison_bandwidth_cents = bandwidth;
    updateParameters();
}

void Unison::updateParameters(void)
{
    if(!uv)
        return;
    float increments_per_second = samplerate_f
                                  / (float) update_period_samples;
//	printf("#%g, %g\n",increments_per_second,base_freq);
    for(int i = 0; i < unison_size; ++i) {
        float base = powf(UNISON_FREQ_SPAN, SYNTH_T::numRandom() * 2.0f - 1.0f);
        uv[i].relative_amplitude = base;
        float period = base / base_freq;
        float m      = 4.0f / (period * increments_per_second);
        if(SYNTH_T::numRandom() < 0.5f)
            m = -m;
        uv[i].step = m;
//		printf("%g %g\n",uv[i].relative_amplitude,period);
    }

    float max_speed = powf(2.0f, unison_bandwidth_cents / 1200.0f);
    unison_amplitude_samples = 0.125f * (max_speed - 1.0f)
                               * samplerate_f / base_freq;

    //If functions exceed this limit, they should have requested a bigguer delay
    //and thus are buggy
    if(unison_amplitude_samples >= max_delay - 1) {
#ifndef WIN32
        warnx("BUG: Unison amplitude samples too big");
        warnx("Unision max_delay should be larger");
#endif
        unison_amplitude_samples = max_delay - 2;
    }

    updateUnisonData();
}

void Unison::process(int bufsize, float *inbuf, float *outbuf)
{
    if(!uv)
        return;
    if(!outbuf)
        outbuf = inbuf;

    float volume    = 1.0f / sqrtf(unison_size);
    float xpos_step = 1.0f / (float) update_period_samples;
    float xpos      = (float) update_period_sample_k * xpos_step;
    for(int i = 0; i < bufsize; ++i) {
        if(update_period_sample_k++ >= update_period_samples) {
            updateUnisonData();
            update_period_sample_k = 0;
            xpos = 0.0f;
        }
        xpos += xpos_step;
        float in   = inbuf[i], out = 0.0f;
        float sign = 1.0f;
        for(int k = 0; k < unison_size; ++k) {
            float vpos = uv[k].realpos1 * (1.0f - xpos) + uv[k].realpos2 * xpos;        //optimize
            float pos  = (float)(delay_k + max_delay) - vpos - 1.0f;
            int   posi;
            F2I(pos, posi); //optimize!
            int posi_next = posi + 1;
            if(posi >= max_delay)
                posi -= max_delay;
            if(posi_next >= max_delay)
                posi_next -= max_delay;
            float posf = pos - floorf(pos);
            out += ((1.0f - posf) * delay_buffer[posi] + posf
                 * delay_buffer[posi_next]) * sign;
            sign = -sign;
        }
        outbuf[i] = out * volume;
//		printf("%d %g\n",i,outbuf[i]);
        delay_buffer[delay_k] = in;
        delay_k = (++delay_k < max_delay) ? delay_k : 0;
    }
}

void Unison::updateUnisonData()
{
    if(!uv)
        return;

    for(int k = 0; k < unison_size; ++k) {
        float pos  = uv[k].position;
        float step = uv[k].step;
        pos += step;
        if(pos <= -1.0f) {
            pos  = -1.0f;
            step = -step;
        }
        else
        if(pos >= 1.0f) {
            pos  = 1.0f;
            step = -step;
        }
        float vibratto_val = (pos - 0.333333333f * pos * pos * pos) * 1.5f; //make the vibratto lfo smoother

        //Relative amplitude is utilized, so the delay may be larger than the
        //whole buffer, if the buffer is too small, this indicates a buggy call
        //to Unison()
        float newval = 1.0f + 0.5f
                       * (vibratto_val + 1.0f) * unison_amplitude_samples
                       * uv[k].relative_amplitude;

        if(first_time)
            uv[k].realpos1 = uv[k].realpos2 = newval;
        else {
            uv[k].realpos1 = uv[k].realpos2;
            uv[k].realpos2 = newval;
        }

        uv[k].position = pos;
        uv[k].step     = step;
    }
    first_time = false;
}
