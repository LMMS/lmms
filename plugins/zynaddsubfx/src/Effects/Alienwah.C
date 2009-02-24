/*
  ZynAddSubFX - a software synthesizer
 
  Alienwah.C - "AlienWah" effect
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
#include "Alienwah.h"
#include <stdio.h>

Alienwah::Alienwah(int insertion_,REALTYPE *efxoutl_,REALTYPE *efxoutr_){
    efxoutl=efxoutl_;
    efxoutr=efxoutr_;
        
    oldl=NULL;
    oldr=NULL;
    filterpars=NULL;
    insertion=insertion_;

    Ppreset=0;
    setpreset(Ppreset);
    cleanup();
    oldclfol.a=fb;oldclfol.b=0.0;
    oldclfor.a=fb;oldclfor.b=0.0;
};

Alienwah::~Alienwah(){
    if (oldl!=NULL) delete [] oldl;
    if (oldr!=NULL) delete [] oldr ;
};


/*
 * Apply the effect
 */
void Alienwah::out(REALTYPE *smpsl,REALTYPE *smpsr){
    int i;
    REALTYPE lfol,lfor;
    COMPLEXTYPE clfol,clfor,out,tmp;

    lfo.effectlfoout(&lfol,&lfor);
    lfol*=depth*PI*2.0;lfor*=depth*PI*2.0;
    clfol.a=cos(lfol+phase)*fb;clfol.b=sin(lfol+phase)*fb;
    clfor.a=cos(lfor+phase)*fb;clfor.b=sin(lfor+phase)*fb;

    for (i=0;i<SOUND_BUFFER_SIZE;i++){	
	REALTYPE x=((REALTYPE) i)/SOUND_BUFFER_SIZE;
	REALTYPE x1=1.0-x;
	//left	
	tmp.a=clfol.a*x+oldclfol.a*x1;
	tmp.b=clfol.b*x+oldclfol.b*x1;
	
	out.a=tmp.a*oldl[oldk].a-tmp.b*oldl[oldk].b
	     +(1-fabs(fb))*smpsl[i]*panning;
	out.b=tmp.a*oldl[oldk].b+tmp.b*oldl[oldk].a;
	oldl[oldk].a=out.a;
	oldl[oldk].b=out.b;
	REALTYPE l=out.a*10.0*(fb+0.1);
	
	//right
	tmp.a=clfor.a*x+oldclfor.a*x1;
	tmp.b=clfor.b*x+oldclfor.b*x1;
	
	out.a=tmp.a*oldr[oldk].a-tmp.b*oldr[oldk].b
	     +(1-fabs(fb))*smpsr[i]*(1.0-panning);
	out.b=tmp.a*oldr[oldk].b+tmp.b*oldr[oldk].a;
	oldr[oldk].a=out.a;
	oldr[oldk].b=out.b;
	REALTYPE r=out.a*10.0*(fb+0.1);


	if (++oldk>=Pdelay) oldk=0;
	//LRcross
	efxoutl[i]=l*(1.0-lrcross)+r*lrcross;
	efxoutr[i]=r*(1.0-lrcross)+l*lrcross;
    };

    oldclfol.a=clfol.a;oldclfol.b=clfol.b;
    oldclfor.a=clfor.a;oldclfor.b=clfor.b;

};

/*
 * Cleanup the effect
 */
void Alienwah::cleanup(){
    for (int i=0;i<Pdelay;i++) {
	oldl[i].a=0.0;
	oldl[i].b=0.0;
	oldr[i].a=0.0;
	oldr[i].b=0.0;
    };
    oldk=0;
};


/*
 * Parameter control
 */

void Alienwah::setdepth(unsigned char Pdepth){
    this->Pdepth=Pdepth;
    depth=(Pdepth/127.0);
};

void Alienwah::setfb(unsigned char Pfb){
    this->Pfb=Pfb;
    fb=fabs((Pfb-64.0)/64.1);
    fb=sqrt(fb);
    if (fb<0.4) fb=0.4;
    if (Pfb<64) fb=-fb;
};

void Alienwah::setvolume(unsigned char Pvolume){
    this->Pvolume=Pvolume;
    outvolume=Pvolume/127.0;
    if (insertion==0) volume=1.0;
	else volume=outvolume;
};

void Alienwah::setpanning(unsigned char Ppanning){
    this->Ppanning=Ppanning;
    panning=Ppanning/127.0;
};

void Alienwah::setlrcross(unsigned char Plrcross){
    this->Plrcross=Plrcross;
    lrcross=Plrcross/127.0;
};

void Alienwah::setphase(unsigned char Pphase){
    this->Pphase=Pphase;
    phase=(Pphase-64.0)/64.0*PI;
};

void Alienwah::setdelay(unsigned char Pdelay){
    if (oldl!=NULL) delete [] oldl;
    if (oldr!=NULL) delete [] oldr;
    if (Pdelay>=MAX_ALIENWAH_DELAY) Pdelay=MAX_ALIENWAH_DELAY;
    this->Pdelay=Pdelay;
    oldl=new COMPLEXTYPE[Pdelay];
    oldr=new COMPLEXTYPE[Pdelay];
    cleanup();
};

void Alienwah::setpreset(unsigned char npreset){
    const int PRESET_SIZE=11;
    const int NUM_PRESETS=4;
    unsigned char presets[NUM_PRESETS][PRESET_SIZE]={
	//AlienWah1
	{127,64,70,0,0,62,60,105,25,0,64},
	//AlienWah2
	{127,64,73,106,0,101,60,105,17,0,64},
	//AlienWah3
	{127,64,63,0,1,100,112,105,31,0,42},
	//AlienWah4
	{93,64,25,0,1,66,101,11,47,0,86}};
	
    if (npreset>=NUM_PRESETS) npreset=NUM_PRESETS-1;
    for (int n=0;n<PRESET_SIZE;n++) changepar(n,presets[npreset][n]);
    if (insertion==0) changepar(0,presets[npreset][0]/2);//lower the volume if this is system effect
    Ppreset=npreset;
};


void Alienwah::changepar(int npar,unsigned char value){
    switch(npar){
	case 0:	setvolume(value);
	        break;
	case 1:	setpanning(value);
	        break;
	case 2:	lfo.Pfreq=value;
	        lfo.updateparams();
		break;	
	case 3:	lfo.Prandomness=value;
	        lfo.updateparams();
		break;	
	case 4:	lfo.PLFOtype=value;
	        lfo.updateparams();
		break;	
	case 5:	lfo.Pstereo=value;
	        lfo.updateparams();
		break;	
	case 6:	setdepth(value);
	        break;
	case 7:	setfb(value);
	        break;
	case 8:	setdelay(value);
	        break;
	case 9:	setlrcross(value);
	        break;
	case 10:setphase(value);
	        break;
    };
};

unsigned char Alienwah::getpar(int npar){
    switch (npar){
	case 0:	return(Pvolume);
		break;
	case 1:	return(Ppanning);
		break;
	case 2:	return(lfo.Pfreq);
		break;
	case 3:	return(lfo.Prandomness);
		break;
	case 4:	return(lfo.PLFOtype);
		break;
	case 5:	return(lfo.Pstereo);
		break;
	case 6:	return(Pdepth);
		break;
	case 7:	return(Pfb);
		break;
	case 8:	return(Pdelay);
		break;
	case 9:	return(Plrcross);
		break;
	case 10:return(Pphase);
		break;
	default:return (0);
    };
    
};




