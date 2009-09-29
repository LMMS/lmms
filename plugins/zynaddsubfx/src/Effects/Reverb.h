/*
  ZynAddSubFX - a software synthesizer

  Reverb.h - Reverberation effect
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

#ifndef REVERB_H
#define REVERB_H

#include <math.h>
#include "../globals.h"
#include "../DSP/AnalogFilter.h"
#include "../DSP/FFTwrapper.h"
#include "../DSP/Unison.h"
#include "Effect.h"

#define REV_COMBS 8
#define REV_APS 4

/**Creates Reverberation Effects*/

class Reverb:public Effect
{
public:
    Reverb(const int &insertion_,REALTYPE *efxoutl_,REALTYPE *efxoutr_);
    ~Reverb();
    void out(REALTYPE *smps_l,REALTYPE *smps_r);
    void cleanup();

    void setpreset(unsigned char npreset);
    void changepar(const int &npar,const unsigned char &value);
    unsigned char getpar(const int &npar)const;

private:
    //Parametrii
    /**Amount of the reverb*/
    unsigned char Pvolume;

    /**Left/Right Panning*/
    unsigned char Ppan;

    /**duration of reverb*/
    unsigned char Ptime;

    /**Initial delay*/
    unsigned char Pidelay;

    /**Initial delay feedback*/
    unsigned char Pidelayfb;

    /**delay between ER/Reverbs*/
    unsigned char Prdelay;

    /**EarlyReflections/Reverb Balance*/
    unsigned char Perbalance;

    /**HighPassFilter*/
    unsigned char Plpf;

    /**LowPassFilter*/
    unsigned char Phpf;

    /**Low/HighFrequency Damping
         * \todo 0..63 lpf,64=off,65..127=hpf(TODO)*/
    unsigned char Plohidamp;

    /**Reverb type*/
    unsigned char Ptype;

    /**Room Size*/
    unsigned char Proomsize;

	/**Bandwidth */
	unsigned char Pbandwidth;

    //parameter control
    void setvolume(const unsigned char &Pvolume);
    void setpan(const unsigned char &Ppan);
    void settime(const unsigned char &Ptime);
    void setlohidamp(unsigned char Plohidamp);
    void setidelay(const unsigned char &Pidelay);
    void setidelayfb(const unsigned char &Pidelayfb);
    void sethpf(const unsigned char &Phpf);
    void setlpf(const unsigned char &Plpf);
    void settype( unsigned char Ptype);
    void setroomsize(const unsigned char &Proomsize);
    void setbandwidth(const unsigned char &Pbandwidth);

    REALTYPE pan,erbalance;
    //Parameters 
    int lohidamptype;/**<0=disable,1=highdamp(lowpass),2=lowdamp(highpass)*/
    int idelaylen,rdelaylen;
    int idelayk;
    REALTYPE lohifb,idelayfb,roomsize,rs;//rs is used to "normalise" the volume according to the roomsize
    int comblen[REV_COMBS*2];
    int aplen[REV_APS*2];
	Unison *bandwidth;

    //Internal Variables

    REALTYPE *comb[REV_COMBS*2];

    int combk[REV_COMBS*2];
    REALTYPE combfb[REV_COMBS*2];/**<feedback-ul fiecarui filtru "comb"*/
    REALTYPE lpcomb[REV_COMBS*2];/**<pentru Filtrul LowPass*/

    REALTYPE *ap[REV_APS*2];

    int apk[REV_APS*2];

    REALTYPE *idelay;
    AnalogFilter *lpf,*hpf;//filters
    REALTYPE *inputbuf;

    void processmono(int ch,REALTYPE *output);
};

#endif

