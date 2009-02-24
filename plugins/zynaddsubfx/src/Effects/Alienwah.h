/*
  ZynAddSubFX - a software synthesizer
 
  Alienwah.h - "AlienWah" effect
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

#ifndef ALIENWAH_H
#define ALIENWAH_H
#include "../globals.h"
#include "Effect.h"
#include "EffectLFO.h"


#define MAX_ALIENWAH_DELAY 100

struct COMPLEXTYPE {
    REALTYPE a,b;
};

class Alienwah:public Effect {
    public:
	Alienwah(int insetion_,REALTYPE *efxoutl_,REALTYPE *efxoutr_);
	~Alienwah();
	void out(REALTYPE *smpsl,REALTYPE *smpsr);

        void setpreset(unsigned char npreset);
	void changepar(int npar,unsigned char value);
	unsigned char getpar(int npar);
	void cleanup();
		
    private:
	//Parametrii Alienwah
	EffectLFO lfo;//lfo-ul Alienwah
	unsigned char Pvolume;
	unsigned char Ppanning;
	unsigned char Pdepth;//the depth of the Alienwah
	unsigned char Pfb;//feedback
	unsigned char Plrcross;//feedback
	unsigned char Pdelay;
	unsigned char Pphase;

	
	//Control Parametrii
	void setvolume(unsigned char Pvolume);
	void setpanning(unsigned char Ppanning);
	void setdepth(unsigned char Pdepth);
	void setfb(unsigned char Pfb);
	void setlrcross(unsigned char Plrcross);
	void setdelay(unsigned char Pdelay);
	void setphase(unsigned char Pphase);
	
	//Valorile interne
	int insertion;
	REALTYPE panning,fb,depth,lrcross,phase;
	COMPLEXTYPE *oldl,*oldr;
	COMPLEXTYPE oldclfol,oldclfor;
	int oldk;
};

#endif

