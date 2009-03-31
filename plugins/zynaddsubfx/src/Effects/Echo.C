/*
  ZynAddSubFX - a software synthesizer
 
  Echo.C - Echo effect
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

#include <cmath>
#include "Echo.h"

Echo::Echo(const int & insertion_,REALTYPE *const efxoutl_,REALTYPE *const efxoutr_)
  : Effect(insertion_,efxoutl_,efxoutr_,NULL,0),
    Pvolume(50),Ppanning(64),Pdelay(60),
    Plrdelay(100),Plrcross(100),Pfb(40),Phidamp(60),
    lrdelay(0),ldelay(NULL),rdelay(NULL)
{
    setpreset(Ppreset);    	   
    cleanup();
}

Echo::~Echo(){
    delete[] ldelay;
    delete[] rdelay;
}

/*
 * Cleanup the effect
 */
void Echo::cleanup(){
    int i;
    for (i=0;i<dl;i++) ldelay[i]=0.0;
    for (i=0;i<dr;i++) rdelay[i]=0.0;
    oldl=0.0;
    oldr=0.0;
}


/*
 * Initialize the delays
 */
void Echo::initdelays(){
    kl=0;kr=0;
    dl=delay-lrdelay;if (dl<1) dl=1;
    dr=delay+lrdelay;if (dr<1) dr=1;
    
    if (ldelay!=NULL) delete [] ldelay;
    if (rdelay!=NULL) delete [] rdelay;
    ldelay=new REALTYPE[dl];
    rdelay=new REALTYPE[dr];

    cleanup();
}

/*
 * Effect output
 */
void Echo::out(REALTYPE *const smpsl,REALTYPE *const smpsr){
    int i;
    REALTYPE l,r,ldl,rdl;

    for (i=0;i<SOUND_BUFFER_SIZE;i++){
        ldl=ldelay[kl];
        rdl=rdelay[kr];
        l=ldl*(1.0-lrcross)+rdl*lrcross;
        r=rdl*(1.0-lrcross)+ldl*lrcross;
        ldl=l;rdl=r;
        
        efxoutl[i]=ldl*2.0;
        efxoutr[i]=rdl*2.0;
        ldl=smpsl[i]*panning-ldl*fb;
        rdl=smpsr[i]*(1.0-panning)-rdl*fb;

        //LowPass Filter
        ldelay[kl]=ldl=ldl*hidamp+oldl*(1.0-hidamp);
        rdelay[kr]=rdl=rdl*hidamp+oldr*(1.0-hidamp);
        oldl=ldl;
        oldr=rdl;

        if (++kl>=dl) kl=0;
        if (++kr>=dr) kr=0;
    };
    
}


/*
 * Parameter control
 */
void Echo::setvolume(const unsigned char & Pvolume){
    this->Pvolume=Pvolume;

    if (insertion==0) {
        outvolume=pow(0.01,(1.0-Pvolume/127.0))*4.0;
        volume=1.0;
    } else {
        volume=outvolume=Pvolume/127.0;
    };
    if (Pvolume==0) cleanup();

}

void Echo::setpanning(const unsigned char & Ppanning){
    this->Ppanning=Ppanning;
    panning=(Ppanning+0.5)/127.0;
}

void Echo::setdelay(const unsigned char & Pdelay){
    this->Pdelay=Pdelay;
    delay=1+(int)(Pdelay/127.0*SAMPLE_RATE*1.5);//0 .. 1.5 sec
    initdelays();
}

void Echo::setlrdelay(const unsigned char & Plrdelay){
    REALTYPE tmp;
    this->Plrdelay=Plrdelay;    
    tmp=(pow(2,fabs(Plrdelay-64.0)/64.0*9)-1.0)/1000.0*SAMPLE_RATE;
    if (Plrdelay<64.0) tmp=-tmp;
    lrdelay=(int) tmp;
    initdelays();
}

void Echo::setlrcross(const unsigned char & Plrcross){
    this->Plrcross=Plrcross;
    lrcross=Plrcross/127.0*1.0;
}

void Echo::setfb(const unsigned char & Pfb){
    this->Pfb=Pfb;
    fb=Pfb/128.0;
}

void Echo::sethidamp(const unsigned char & Phidamp){
    this->Phidamp=Phidamp;
    hidamp=1.0-Phidamp/127.0;
}

void Echo::setpreset(unsigned char npreset){
    const int PRESET_SIZE=7;
    const int NUM_PRESETS=9;
    unsigned char presets[NUM_PRESETS][PRESET_SIZE]={
        //Echo 1
        {67,64,35,64,30,59,0},
        //Echo 2
        {67,64,21,64,30,59,0},
        //Echo 3
        {67,75,60,64,30,59,10},
        //Simple Echo
        {67,60,44,64,30,0,0},
        //Canyon
        {67,60,102,50,30,82,48},
        //Panning Echo 1
        {67,64,44,17,0,82,24},
        //Panning Echo 2
        {81,60,46,118,100,68,18},
        //Panning Echo 3
        {81,60,26,100,127,67,36},
        //Feedback Echo
        {62,64,28,64,100,90,55}};


    if (npreset>=NUM_PRESETS) npreset=NUM_PRESETS-1;
    for (int n=0;n<PRESET_SIZE;n++) changepar(n,presets[npreset][n]);
    if (insertion!=0) changepar(0,presets[npreset][0]/2);//lower the volume if this is insertion effect
    Ppreset=npreset;
}


void Echo::changepar(const int & npar,const unsigned char & value){
    switch (npar){
        case 0: setvolume(value);
                break;
        case 1: setpanning(value);
                break;
        case 2: setdelay(value);
                break;
        case 3: setlrdelay(value);
                break;
        case 4: setlrcross(value);
                break;
        case 5: setfb(value);
                break;
        case 6: sethidamp(value);
                break;
    };
    /**\todo in case of bogus parameter number print error to find depreciated
     *calls*/
}

unsigned char Echo::getpar(const int & npar)const{
    switch (npar){
        case 0: return(Pvolume);
                break;
        case 1: return(Ppanning);
                break;
        case 2: return(Pdelay);
                break;
        case 3: return(Plrdelay);
                break;
        case 4: return(Plrcross);
                break;
        case 5: return(Pfb);
                break;
        case 6: return(Phidamp);
                break;
    };
    return(0);// in case of bogus parameter number
}

