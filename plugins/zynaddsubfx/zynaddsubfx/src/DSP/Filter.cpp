/*
  ZynAddSubFX - a software synthesizer

  Filter.cpp - Filters, uses analog,formant,etc. filters
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

#include <math.h>
#include <stdio.h>

#include "Filter.h"
#include "AnalogFilter.h"
#include "FormantFilter.h"
#include "SVFilter.h"
#include "../Params/FilterParams.h"

Filter::Filter(unsigned int srate, int bufsize)
    : outgain(1.0f),
      samplerate(srate),
      buffersize(bufsize)
{
    alias();
}

Filter *Filter::generate(FilterParams *pars, unsigned int srate, int bufsize)
{
    if (srate == 0)
        srate = synth->samplerate;
    if (bufsize == 0)
        bufsize = synth->buffersize;

    unsigned char Ftype   = pars->Ptype;
    unsigned char Fstages = pars->Pstages;

    Filter *filter;
    switch(pars->Pcategory) {
        case 1:
            filter = new FormantFilter(pars, srate, bufsize);
            break;
        case 2:
            filter = new SVFilter(Ftype, 1000.0f, pars->getq(), Fstages, srate, bufsize);
            filter->outgain = dB2rap(pars->getgain());
            if(filter->outgain > 1.0f)
                filter->outgain = sqrt(filter->outgain);
            break;
        default:
            filter = new AnalogFilter(Ftype, 1000.0f, pars->getq(), Fstages, srate, bufsize);
            if((Ftype >= 6) && (Ftype <= 8))
                filter->setgain(pars->getgain());
            else
                filter->outgain = dB2rap(pars->getgain());
            break;
    }
    return filter;
}

float Filter::getrealfreq(float freqpitch)
{
    return powf(2.0f, freqpitch + 9.96578428f); //log2(1000)=9.95748f
}
