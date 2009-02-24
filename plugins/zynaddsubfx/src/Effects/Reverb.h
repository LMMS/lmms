/*
  ZynAddSubFX - a software synthesizer
 
  Reverb.h - Reverberation effect
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

#ifndef REVERB_H
#define REVERB_H


#include "../globals.h"
#include "../DSP/AnalogFilter.h"
#include "Effect.h"

#define REV_COMBS 8
#define REV_APS 4

class Reverb:public Effect {
    public:
	Reverb(int insertion,REALTYPE *efxoutl_,REALTYPE *efxoutr_);
	~Reverb();
	void out(REALTYPE *smps_l,REALTYPE *smps_r);
	void cleanup();

        void setpreset(unsigned char npreset);
	void changepar(int npar,unsigned char value);
	unsigned char getpar(int npar);

    private:
	//Parametrii
	//Amount of the reverb,
	unsigned char Pvolume;

	//LefT/Right Panning
	unsigned char Ppan;

	//duration of reverb
	unsigned char Ptime;

	//Initial delay 
	unsigned char Pidelay;

	//Initial delay feedback
	unsigned char Pidelayfb;

	//delay between ER/Reverbs
	unsigned char Prdelay;

	//EarlyReflections/Reverb Balance
	unsigned char Perbalance;

	//HighPassFilter 
	unsigned char Plpf;

	//LowPassFilter
	unsigned char Phpf;

	//Low/HighFrequency Damping
	unsigned char Plohidamp;// 0..63 lpf,64=off,65..127=hpf(TODO)

	//Reverb type
	unsigned char Ptype;

	//Room Size
	unsigned char Proomsize;

	//parameter control
	void setvolume(unsigned char Pvolume);
	void setpan(unsigned char Ppan);
	void settime(unsigned char Ptime);
	void setlohidamp(unsigned char Plohidamp);
	void setidelay(unsigned char Pidelay);
	void setidelayfb(unsigned char Pidelayfb);
	void sethpf(unsigned char Phpf);
	void setlpf(unsigned char Plpf);
	void settype(unsigned char Ptype);
	void setroomsize(unsigned char Proomsize);
	
	REALTYPE pan,erbalance;
	//Parametrii 2	
	int lohidamptype;//0=disable,1=highdamp(lowpass),2=lowdamp(highpass)
	int idelaylen,rdelaylen;
	int idelayk;
	REALTYPE lohifb,idelayfb,roomsize,rs;//rs is used to "normalise" the volume according to the roomsize
	int comblen[REV_COMBS*2];		
	int aplen[REV_APS*2];
	
	//Valorile interne
	
	REALTYPE *comb[REV_COMBS*2];
	
	int combk[REV_COMBS*2];
	REALTYPE combfb[REV_COMBS*2];//feedback-ul fiecarui filtru "comb"
	REALTYPE lpcomb[REV_COMBS*2];//pentru Filtrul LowPass

	REALTYPE *ap[REV_APS*2];
	
	int apk[REV_APS*2];
	
	REALTYPE *idelay;
	AnalogFilter *lpf,*hpf;//filters
	REALTYPE *inputbuf;
	
	void processmono(int ch,REALTYPE *output);
};




#endif

