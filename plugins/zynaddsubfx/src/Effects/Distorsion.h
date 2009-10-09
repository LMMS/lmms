/*
  ZynAddSubFX - a software synthesizer

  Distorsion.h - Distorsion Effect
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

#ifndef DISTORSION_H
#define DISTORSION_H

#include "../globals.h"
#include "../DSP/AnalogFilter.h"
#include "Effect.h"

//Waveshaping(called by Distorsion effect and waveshape from OscilGen)
void waveshapesmps(int n,
                   REALTYPE *smps,
                   unsigned char type,
                   unsigned char drive);
/**Distortion Effect*/
class Distorsion:public Effect
{
    public:
        Distorsion(const int &insertion, REALTYPE *efxoutl_, REALTYPE *efxoutr_);
        ~Distorsion();
        void out(REALTYPE *smpsl, REALTYPE *smpr);
        void setpreset(unsigned char npreset);
        void changepar(const int &npar, const unsigned char &value);
        unsigned char getpar(const int &npar) const;
        void cleanup();
        void applyfilters(REALTYPE *efxoutl, REALTYPE *efxoutr);

    private:
        //Parametrii
        unsigned char Pvolume; //Volumul or E/R
        unsigned char Ppanning; //Panning
        unsigned char Plrcross; // L/R Mixing
        unsigned char Pdrive; //the input amplification
        unsigned char Plevel; //the output amplification
        unsigned char Ptype; //Distorsion type
        unsigned char Pnegate; //if the input is negated
        unsigned char Plpf; //lowpass filter
        unsigned char Phpf; //highpass filter
        unsigned char Pstereo; //0=mono,1=stereo
        unsigned char Pprefiltering; //if you want to do the filtering before the distorsion

        void setvolume(const unsigned char &Pvolume);
        void setpanning(const unsigned char &Ppanning);
        void setlrcross(const unsigned char &Plrcross);
        void setlpf(const unsigned char &Plpf);
        void sethpf(const unsigned char &Phpf);

        //Real Parameters
        REALTYPE      panning, lrcross;
        AnalogFilter *lpfl, *lpfr, *hpfl, *hpfr;
};


#endif

