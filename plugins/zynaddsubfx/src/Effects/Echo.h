/*
  ZynAddSubFX - a software synthesizer
 
  Echo.h - Echo Effect
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

#ifndef ECHO_H
#define ECHO_H

#include "../globals.h"
#include "Effect.h"

class Echo:public Effect{
    public:
	Echo(int insertion,REALTYPE *efxoutl_,REALTYPE *efxoutr_);
	~Echo();
	void out(REALTYPE *smpsl,REALTYPE *smpr);
	void setpreset(unsigned char npreset);
	void changepar(int npar,unsigned char value);
	unsigned char getpar(int npar);
	void cleanup();

	void setdryonly();
    private:
	//Parametrii
	unsigned char Pvolume;//Volumul or E/R
	unsigned char Ppanning;//Panning
	unsigned char Pdelay;
	unsigned char Plrdelay;// L/R delay difference
	unsigned char Plrcross;// L/R Mixing
	unsigned char Pfb;//Feed-back-ul
	unsigned char Phidamp;
	
	void setvolume(unsigned char Pvolume);
	void setpanning(unsigned char Ppanning);
	void setdelay(unsigned char Pdelay);
	void setlrdelay(unsigned char Plrdelay);
	void setlrcross(unsigned char Plrcross);
	void setfb(unsigned char Pfb);
	void sethidamp(unsigned char Phidamp);
	
	//Parametrii reali
	REALTYPE panning,lrcross,fb,hidamp;
	int dl,dr,delay,lrdelay;
	
	void initdelays();
	REALTYPE *ldelay,*rdelay;
	REALTYPE oldl,oldr;//pt. lpf
	int kl,kr;
};


#endif


