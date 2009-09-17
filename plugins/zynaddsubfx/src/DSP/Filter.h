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

#include "Filter_.h"
#include "AnalogFilter.h"
#include "FormantFilter.h"
#include "SVFilter.h"
#include "../Params/FilterParams.h"

class Filter
{
public:
    Filter(FilterParams *pars);
    ~Filter();
    void filterout(REALTYPE *smp);
    void setfreq(REALTYPE frequency);
    void setfreq_and_q(REALTYPE frequency,REALTYPE q_);
    void setq(REALTYPE q_);

    REALTYPE getrealfreq(REALTYPE freqpitch);
private:
    Filter_ *filter;
    unsigned char category;
};


#endif

