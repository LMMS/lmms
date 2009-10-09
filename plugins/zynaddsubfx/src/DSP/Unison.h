/*
  ZynAddSubFX - a software synthesizer

  Unison.h - Unison effect (multivoice chorus)
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

#ifndef UNISON_H
#define UNISON_H
#include <stdlib.h>
#include "../globals.h"

#define UNISON_FREQ_SPAN 2.0
//how much the unison frequencies varies (always >= 1.0)

class Unison
{
    public:
        Unison(int update_period_samples_, REALTYPE max_delay_sec_);
        ~Unison();

        void set_size(int new_size);
        void set_base_frequency(REALTYPE freq);
        void set_bandwidth(REALTYPE bandwidth_cents);

        void process(int bufsize, REALTYPE *inbuf, REALTYPE *outbuf = NULL);

    private:
        void update_parameters();
        void update_unison_data();

        int      unison_size;
        REALTYPE base_freq;
        struct UnisonVoice {
            REALTYPE step, position; //base LFO
            REALTYPE realpos1, realpos2; //the position regarding samples
            REALTYPE relative_amplitude;
            REALTYPE lin_fpos, lin_ffreq;
            UnisonVoice() {
                position = RND * 1.8 - 0.9;
                realpos1 = 0.0;
                realpos2 = 0.0;
                step     = 0.0;
                relative_amplitude = 1.0;
            }
        } *uv;
        int       update_period_samples, update_period_sample_k;
        int       max_delay, delay_k;
        bool      first_time;
        REALTYPE *delay_buffer;
        REALTYPE  unison_amplitude_samples;
        REALTYPE  unison_bandwidth_cents;
};
#endif

