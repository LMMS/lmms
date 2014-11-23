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

#include "../Misc/Util.h"

//how much the unison frequencies varies (always >= 1.0)
#define UNISON_FREQ_SPAN 2.0f

class Unison
{
    public:
        Unison(int update_period_samples_, float max_delay_sec_, float srate_f);
        ~Unison();

        void setSize(int new_size);
        void setBaseFrequency(float freq);
        void setBandwidth(float bandwidth_cents);

        void process(int bufsize, float *inbuf, float *outbuf = NULL);

    private:
        void updateParameters(void);
        void updateUnisonData(void);

        int   unison_size;
        float base_freq;
        struct UnisonVoice {
            float step;     //base LFO
            float position;
            float realpos1; //the position regarding samples
            float realpos2;
            float relative_amplitude;
            float lin_fpos;
            float lin_ffreq;
            UnisonVoice() {
                position = RND * 1.8f - 0.9f;
                realpos1 = 0.0f;
                realpos2 = 0.0f;
                step     = 0.0f;
                relative_amplitude = 1.0f;
            }
        } *uv;

        int    update_period_samples;
        int    update_period_sample_k;
        int    max_delay, delay_k;
        bool   first_time;
        float *delay_buffer;
        float  unison_amplitude_samples;
        float  unison_bandwidth_cents;

        // current setup
        float samplerate_f;
};
#endif
