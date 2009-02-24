/*
  ZynAddSubFX - a software synthesizer
 
  Chorus.h - Chorus and Flange effects
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

#ifndef CHORUS_H
#define CHORUS_H
#include "../globals.h"
#include "Effect.h"
#include "EffectLFO.h"

#define MAX_CHORUS_DELAY 250.0 //ms

class Chorus:public Effect {
    public:
	Chorus(int insetion_,REALTYPE *efxoutl_,REALTYPE *efxoutr_);
	~Chorus();
	void out(REALTYPE *smpsl,REALTYPE *smpsr);
        void setpreset(unsigned char npreset);
	void changepar(int npar,unsigned char value);
	unsigned char getpar(int npar);
	void cleanup();
		
    private:
	//Parametrii Chorus
	EffectLFO lfo;//lfo-ul chorus
	unsigned char Pvolume;
	unsigned char Ppanning;
	unsigned char Pdepth;//the depth of the Chorus(ms)
	unsigned char Pdelay;//the delay (ms)
	unsigned char Pfb;//feedback
	unsigned char Plrcross;//feedback
	unsigned char Pflangemode;//how the LFO is scaled, to result chorus or flange
	unsigned char Poutsub;//if I wish to substract the output instead of the adding it

	
	//Control Parametrii
	void setvolume(unsigned char Pvolume);
	void setpanning(unsigned char Ppanning);
	void setdepth(unsigned char Pdepth);
	void setdelay(unsigned char Pdelay);
	void setfb(unsigned char Pfb);
	void setlrcross(unsigned char Plrcross);

	//Valorile interne
	REALTYPE depth,delay,fb,lrcross,panning;
	REALTYPE dl1,dl2,dr1,dr2,lfol,lfor;
	int insertion,maxdelay;
	REALTYPE *delayl,*delayr;
	int dlk,drk,dlhi,dlhi2;
	REALTYPE getdelay(REALTYPE xlfo);
	REALTYPE dllo,mdel;
};

#endif

