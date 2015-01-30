/*
  ZynAddSubFX - a software synthesizer

  Filter.h - Filters, uses analog,formant,etc. filters
  Copyright (C) 2002-2005 Nasca Octavian Paul
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

#ifndef FILTER_H
#define FILTER_H

#include "../globals.h"

class Filter
{
    public:
        static float getrealfreq(float freqpitch);
        static Filter *generate(class FilterParams * pars, unsigned int srate = 0, int bufsize = 0);

        Filter(unsigned int srate, int bufsize);
        virtual ~Filter() {}
        virtual void filterout(float *smp)    = 0;
        virtual void setfreq(float frequency) = 0;
        virtual void setfreq_and_q(float frequency, float q_) = 0;
        virtual void setq(float q_) = 0;
        virtual void setgain(float dBgain) = 0;

    protected:
        float outgain;

        // current setup
        unsigned int samplerate;
        int buffersize;

        // alias for above terms
        float samplerate_f;
        float halfsamplerate_f;
        float buffersize_f;
        int   bufferbytes;

        inline void alias()
        {
            samplerate_f     = samplerate;
            halfsamplerate_f = samplerate_f / 2.0f;
            buffersize_f     = buffersize;
            bufferbytes      = buffersize * sizeof(float);
        }
};

#endif
