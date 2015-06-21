/*
  ZynAddSubFX - a software synthesizer

  Resonance.h - Resonance
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
#ifndef RESONANCE_H
#define RESONANCE_H

#include "../globals.h"
#include "../Misc/Util.h"
#include "../Misc/XMLwrapper.h"
#include "../Params/Presets.h"
#include "../DSP/FFTwrapper.h"

#define N_RES_POINTS 256

class Resonance:public Presets
{
    public:
        Resonance(void);
        ~Resonance(void);
        void setpoint(int n, unsigned char p);
        void applyres(int n, fft_t *fftdata, float freq) const;
        void smooth(void);
        void interpolatepeaks(int type);
        void randomize(int type);
        void zero(void);

        void paste(Resonance &r);
        void add2XML(XMLwrapper *xml);
        void defaults(void);
        void getfromXML(XMLwrapper *xml);


        //TODO remove unused methods
        float getfreqpos(float freq) const;
        float getfreqx(float x) const;
        float getfreqresponse(float freq) const;
        float getcenterfreq(void) const;
        float getoctavesfreq(void) const;
        void sendcontroller(MidiControllers ctl, float par);

        //parameters
        unsigned char Penabled;     //if the ressonance is enabled
        unsigned char Prespoints[N_RES_POINTS]; //how many points define the resonance function
        unsigned char PmaxdB;       //how many dB the signal may be amplified
        unsigned char Pcenterfreq, Poctavesfreq; //the center frequency of the res. func., and the number of octaves
        unsigned char Pprotectthefundamental;   //the fundamental (1-st harmonic) is not damped, even it resonance function is low

        //controllers
        float ctlcenter; //center frequency(relative)
        float ctlbw; //bandwidth(relative)

        static rtosc::Ports ports;
};

#endif
