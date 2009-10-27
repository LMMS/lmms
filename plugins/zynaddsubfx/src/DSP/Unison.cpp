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

#include <math.h>
#include <stdio.h>
#include "Unison.h"

Unison::Unison(int update_period_samples_, REALTYPE max_delay_sec_) {
    update_period_samples = update_period_samples_;
    max_delay    = (int)(max_delay_sec_ * (REALTYPE)SAMPLE_RATE + 1);
    if(max_delay < 10)
        max_delay = 10;
    delay_buffer = new REALTYPE[max_delay];
    delay_k      = 0;
    base_freq    = 1.0;
    unison_bandwidth_cents = 10.0;

    ZERO_REALTYPE(delay_buffer, max_delay);

    uv = NULL;
    update_period_sample_k = 0;
    first_time = 0;

    set_size(1);
}

Unison::~Unison() {
    delete [] delay_buffer;
    if(uv)
        delete [] uv;
}

void Unison::set_size(int new_size) {
    if(new_size < 1)
        new_size = 1;
    unison_size = new_size;
    if(uv)
        delete [] uv;
    uv = new UnisonVoice[unison_size];
    first_time = true;
    update_parameters();
}

void Unison::set_base_frequency(REALTYPE freq) {
    base_freq = freq;
    update_parameters();
}

void Unison::set_bandwidth(REALTYPE bandwidth) {
    if(bandwidth < 0)
        bandwidth = 0.0;
    if(bandwidth > 1200.0)
        bandwidth = 1200.0;

    printf("bandwidth %g\n", bandwidth);
#warning \
    : todo: if bandwidth is too small the audio will be self canceled (because of the sign change of the outputs)
    unison_bandwidth_cents = bandwidth;
    update_parameters();
}

void Unison::update_parameters() {
    if(!uv)
        return;
    REALTYPE increments_per_second = SAMPLE_RATE
                                     / (REALTYPE) update_period_samples;
//	printf("#%g, %g\n",increments_per_second,base_freq);
    for(int i = 0; i < unison_size; i++) {
        REALTYPE base   = pow(UNISON_FREQ_SPAN, RND * 2.0 - 1.0);
        uv[i].relative_amplitude = base;
        REALTYPE period = base / base_freq;
        REALTYPE m      = 4.0 / (period * increments_per_second);
        if(RND < 0.5)
            m = -m;
        uv[i].step = m;
//		printf("%g %g\n",uv[i].relative_amplitude,period);
    }

    REALTYPE max_speed = pow(2.0, unison_bandwidth_cents / 1200.0);
    unison_amplitude_samples = 0.125
                               * (max_speed - 1.0) * SAMPLE_RATE / base_freq;
    printf("unison_amplitude_samples %g\n", unison_amplitude_samples);

#warning \
    todo: test if unison_amplitude_samples is to big and reallocate bigger memory
    if(unison_amplitude_samples >= max_delay - 1)
        unison_amplitude_samples = max_delay - 2;

    update_unison_data();
}

void Unison::process(int bufsize, REALTYPE *inbuf, REALTYPE *outbuf) {
    if(!uv)
        return;
    if(!outbuf)
        outbuf = inbuf;

    REALTYPE volume    = 1.0 / sqrt(unison_size);
    REALTYPE xpos_step = 1.0 / (REALTYPE) update_period_samples;
    REALTYPE xpos      = (REALTYPE) update_period_sample_k * xpos_step;
    for(int i = 0; i < bufsize; i++) {
        if((update_period_sample_k++) >= update_period_samples) {
            update_unison_data();
            update_period_sample_k = 0;
            xpos = 0.0;
        }
        xpos += xpos_step;
        REALTYPE in   = inbuf[i], out = 0.0;

        REALTYPE sign = 1.0;
        for(int k = 0; k < unison_size; k++) {
            REALTYPE vpos = uv[k].realpos1
                            * (1.0 - xpos) + uv[k].realpos2 * xpos;     //optimize
            REALTYPE pos  = delay_k + max_delay - vpos - 1.0; //optimize
            int      posi;
            REALTYPE posf;
            F2I(pos, posi); //optimize!
            if(posi >= max_delay)
                posi -= max_delay;
            posf = pos - floor(pos);
            out +=
                ((1.0
                  - posf) * delay_buffer[posi] + posf
                 * delay_buffer[posi + 1]) * sign;
            sign = -sign;
        }
        outbuf[i] = out * volume;
//		printf("%d %g\n",i,outbuf[i]);
        delay_buffer[delay_k] = in;
        if((++delay_k) >= max_delay)
            delay_k = 0;
    }
}

void Unison::update_unison_data() {
    if(!uv)
        return;

    for(int k = 0; k < unison_size; k++) {
        REALTYPE pos  = uv[k].position;
        REALTYPE step = uv[k].step;
        pos += step;
        if(pos <= -1.0) {
            pos  = -1.0;
            step = -step;
        }
        if(pos >= 1.0) {
            pos  = 1.0;
            step = -step;
        }
        REALTYPE vibratto_val = (pos - 0.333333333 * pos * pos * pos) * 1.5; //make the vibratto lfo smoother
#warning \
        I will use relative amplitude, so the delay might be bigger than the whole buffer
#warning \
        I have to enlarge (reallocate) the buffer to make place for the whole delay
        REALTYPE newval = 1.0 + 0.5
                          * (vibratto_val
                             + 1.0) * unison_amplitude_samples
                          * uv[k].relative_amplitude;

        if(first_time)
            uv[k].realpos1 = uv[k].realpos2 = newval;
        else{
            uv[k].realpos1 = uv[k].realpos2;
            uv[k].realpos2 = newval;
        }

        uv[k].position = pos;
        uv[k].step     = step;
    }
    if(first_time)
        first_time = false;
}

