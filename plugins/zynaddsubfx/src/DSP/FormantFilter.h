/*
  ZynAddSubFX - a software synthesizer

  FormantFilter.h - formant filter
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

#ifndef FORMANT_FILTER_H
#define FORMANT_FILTER_H

#include "../globals.h"
#include "Filter_.h"
#include "AnalogFilter.h"
#include "../Params/FilterParams.h"


class FormantFilter:public Filter_
{
    public:
        FormantFilter(FilterParams *pars);
        ~FormantFilter();
        void filterout(REALTYPE *smp);
        void setfreq(REALTYPE frequency);
        void setfreq_and_q(REALTYPE frequency, REALTYPE q_);
        void setq(REALTYPE q_);

        void cleanup();
    private:
        AnalogFilter *formant[FF_MAX_FORMANTS];
        REALTYPE     *inbuffer, *tmpbuf;

        struct {
            REALTYPE freq, amp, q; //frequency,amplitude,Q
        } formantpar[FF_MAX_VOWELS][FF_MAX_FORMANTS],
          currentformants[FF_MAX_FORMANTS];

        struct {
            unsigned char nvowel;
        } sequence [FF_MAX_SEQUENCE];

        REALTYPE oldformantamp[FF_MAX_FORMANTS];

        int      sequencesize, numformants, firsttime;
        REALTYPE oldinput, slowinput;
        REALTYPE Qfactor, formantslowness, oldQfactor;
        REALTYPE vowelclearness, sequencestretch;

        void setpos(REALTYPE input);
};


#endif

