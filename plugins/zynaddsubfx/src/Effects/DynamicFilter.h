/*
  ZynAddSubFX - a software synthesizer

  DynamicFilter.h - "WahWah" effect and others
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

#ifndef DYNAMICFILTER_H
#define DYNAMICFILTER_H
#include "../globals.h"
#include "Effect.h"
#include "EffectLFO.h"

#include "../DSP/Filter.h"
/**DynamicFilter Effect*/
class DynamicFilter:public Effect
{
    public:
        DynamicFilter(int insetion_, REALTYPE *efxoutl_, REALTYPE *efxoutr_);
        ~DynamicFilter();
        void out(REALTYPE *smpsl, REALTYPE *smpsr);

        void setpreset(unsigned char npreset);
        void changepar(const int &npar, const unsigned char &value);
        unsigned char getpar(const int &npar) const;
        void cleanup();

//	void setdryonly();

    private:
        //Parametrii DynamicFilter
        EffectLFO     lfo; //lfo-ul DynamicFilter
        unsigned char Pvolume;
        unsigned char Ppanning;
        unsigned char Pdepth; /**<the depth of the lfo of the DynamicFilter*/
        unsigned char Pampsns; /**<how the filter varies according to the input amplitude*/
        unsigned char Pampsnsinv; //if the filter freq is lowered if the input amplitude rises
        unsigned char Pampsmooth; //how smooth the input amplitude changes the filter

        //Parameter Control
        void setvolume(const unsigned char &Pvolume);
        void setpanning(const unsigned char &Ppanning);
        void setdepth(const unsigned char &Pdepth);
        void setampsns(const unsigned char &Pampsns);

        void reinitfilter();

        //Internal Values
        REALTYPE panning, depth, ampsns, ampsmooth;

        Filter *filterl, *filterr;

        REALTYPE ms1, ms2, ms3, ms4; //mean squares
};

#endif

