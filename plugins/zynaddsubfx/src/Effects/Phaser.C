/*
  ZynAddSubFX - a software synthesizer
 
  Phaser.C - Phaser effect
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
#include "Phaser.h"
#include <stdio.h>
#define PHASER_LFO_SHAPE 2

Phaser::Phaser(int insertion_,REALTYPE *efxoutl_,REALTYPE *efxoutr_){
    efxoutl=efxoutl_;
    efxoutr=efxoutr_;
    filterpars=NULL;
        
    oldl=NULL;
    oldr=NULL;
    insertion=insertion_;

    Ppreset=0;
    setpreset(Ppreset);
    cleanup();
};

Phaser::~Phaser(){
    if (oldl!=NULL) delete [] oldl;
    if (oldr!=NULL) delete [] oldr;
};


/*
 * Effect output
 */
void Phaser::out(REALTYPE *smpsl,REALTYPE *smpsr){
    int i,j;
    REALTYPE lfol,lfor,lgain,rgain,tmp;

    lfo.effectlfoout(&lfol,&lfor);
    lgain=lfol;
    rgain=lfor;
    lgain=(exp(lgain*PHASER_LFO_SHAPE)-1)/(exp(PHASER_LFO_SHAPE)-1.0);
    rgain=(exp(rgain*PHASER_LFO_SHAPE)-1)/(exp(PHASER_LFO_SHAPE)-1.0);


    lgain=1.0-phase*(1.0-depth)-(1.0-phase)*lgain*depth;
    rgain=1.0-phase*(1.0-depth)-(1.0-phase)*rgain*depth;
    
    if (lgain>1.0) lgain=1.0;else if (lgain<0.0) lgain=0.0;
    if (rgain>1.0) rgain=1.0;else if (rgain<0.0) rgain=0.0;

    for (i=0;i<SOUND_BUFFER_SIZE;i++){	
	REALTYPE x=(REALTYPE) i /SOUND_BUFFER_SIZE;
	REALTYPE x1=1.0-x;
	REALTYPE gl=lgain*x+oldlgain*x1;
	REALTYPE gr=rgain*x+oldrgain*x1;
	REALTYPE inl=smpsl[i]*panning+fbl;
	REALTYPE inr=smpsr[i]*(1.0-panning)+fbr;

	//Left channel
	for (j=0;j<Pstages*2;j++){//Phasing routine
	    tmp=oldl[j];
	    oldl[j]=gl*tmp+inl;
	    inl=tmp-gl*oldl[j];
	};
	//Right channel
	for (j=0;j<Pstages*2;j++){//Phasing routine
	    tmp=oldr[j];
	    oldr[j]=gr*tmp+inr;
	    inr=tmp-gr*oldr[j];
	};
	//Left/Right crossing
	REALTYPE l=inl;
	REALTYPE r=inr;
	inl=l*(1.0-lrcross)+r*lrcross;
	inr=r*(1.0-lrcross)+l*lrcross;
	
	fbl=inl*fb;
	fbr=inr*fb;
	efxoutl[i]=inl;
	efxoutr[i]=inr;

    };

    oldlgain=lgain; oldrgain=rgain;

    if (Poutsub!=0)
	for (i=0;i<SOUND_BUFFER_SIZE;i++){
    	    efxoutl[i]*= -1.0;
            efxoutr[i]*= -1.0;
	};

};

/*
 * Cleanup the effect
 */
void Phaser::cleanup(){
    fbl=0.0;fbr=0.0;
    oldlgain=0.0;
    oldrgain=0.0;
    for (int i=0;i<Pstages*2;i++) {
	oldl[i]=0.0;
	oldr[i]=0.0;
    };
};

/*
 * Parameter control
 */
void Phaser::setdepth(unsigned char Pdepth){
    this->Pdepth=Pdepth;
    depth=(Pdepth/127.0);
};


void Phaser::setfb(unsigned char Pfb){
    this->Pfb=Pfb;
    fb=(Pfb-64.0)/64.1;
};

void Phaser::setvolume(unsigned char Pvolume){
    this->Pvolume=Pvolume;
    outvolume=Pvolume/127.0;
    if (insertion==0) volume=1.0;
	else volume=outvolume;
};

void Phaser::setpanning(unsigned char Ppanning){
    this->Ppanning=Ppanning;
    panning=Ppanning/127.0;
};

void Phaser::setlrcross(unsigned char Plrcross){
    this->Plrcross=Plrcross;
    lrcross=Plrcross/127.0;
};

void Phaser::setstages(unsigned char Pstages){
    if (oldl!=NULL) delete [] oldl;
    if (oldr!=NULL) delete [] oldr;
    if (Pstages>=MAX_PHASER_STAGES) Pstages=MAX_PHASER_STAGES-1;
    this->Pstages=Pstages;
    oldl=new REALTYPE[Pstages*2];
    oldr=new REALTYPE[Pstages*2];
    cleanup();
};

void Phaser::setphase(unsigned char Pphase){
    this->Pphase=Pphase;
    phase=(Pphase/127.0);
};


void Phaser::setpreset(unsigned char npreset){
    const int PRESET_SIZE=12;
    const int NUM_PRESETS=6;
    unsigned char presets[NUM_PRESETS][PRESET_SIZE]={
	//Phaser1
	{64,64,36,0,0,64,110,64,1,0,0,20},
	//Phaser2
	{64,64,35,0,0,88,40,64,3,0,0,20},
	//Phaser3
	{64,64,31,0,0,66,68,107,2,0,0,20},
	//Phaser4
	{39,64,22,0,0,66,67,10,5,0,1,20},
	//Phaser5
	{64,64,20,0,1,110,67,78,10,0,0,20},
	//Phaser6
	{64,64,53,100,0,58,37,78,3,0,0,20}};
    if (npreset>=NUM_PRESETS) npreset=NUM_PRESETS-1;
    for (int n=0;n<PRESET_SIZE;n++) changepar(n,presets[npreset][n]);
    Ppreset=npreset;
};


void Phaser::changepar(int npar,unsigned char value){
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
	case 8:	setstages(value);
	        break;
	case 9:	setlrcross(value);
	        break;
	case 10:if (value>1) value=1;
		Poutsub=value;
		break;
	case 11:setphase(value);
	        break;
    };
};

unsigned char Phaser::getpar(int npar){
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
	case 8:	return(Pstages);
		break;
	case 9:	return(Plrcross);
		break;
	case 10:return(Poutsub);
		break;
	case 11:return(Pphase);
		break;
	default:return (0);
    };
    
};




