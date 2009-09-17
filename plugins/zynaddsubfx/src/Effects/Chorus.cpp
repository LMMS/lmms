/*
  ZynAddSubFX - a software synthesizer

  Chorus.C - Chorus and Flange effects
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
#include "Chorus.h"
#include <iostream>

using namespace std;

Chorus::Chorus(const int &insertion_,REALTYPE *const efxoutl_,REALTYPE *const efxoutr_)
        :Effect(insertion_,efxoutl_,efxoutr_,NULL,0),
        maxdelay((int)(MAX_CHORUS_DELAY/1000.0*SAMPLE_RATE)),
        delaySample(maxdelay)
{
    dlk=0;
    drk=0;
    //maxdelay=(int)(MAX_CHORUS_DELAY/1000.0*SAMPLE_RATE);
    //delayl=new REALTYPE[maxdelay];
    //delayr=new REALTYPE[maxdelay];

    setpreset(Ppreset);

    lfo.effectlfoout(&lfol,&lfor);
    dl2=getdelay(lfol);
    dr2=getdelay(lfor);
    cleanup();
};

Chorus::~Chorus() {};

/*
 * get the delay value in samples; xlfo is the current lfo value
 */
REALTYPE Chorus::getdelay(REALTYPE xlfo)
{
    REALTYPE result;
    if (Pflangemode==0) {
        result=(delay+xlfo*depth)*SAMPLE_RATE;
    } else result=0;

    //check if it is too big delay(caused bu errornous setdelay() and setdepth()
    /**\todo fix setdelay() and setdepth(), so this error cannot occur*/
    if ((result+0.5)>=maxdelay) {
        cerr << "WARNING: Chorus.C::getdelay(..) too big delay (see setdelay and setdepth funcs.)\n";
        result=maxdelay-1.0;
    };
    return(result);
};

/*
 * Apply the effect
 */
void Chorus::out(REALTYPE *smpsl,REALTYPE *smpsr)
{
    const Stereo<AuSample> input(AuSample(SOUND_BUFFER_SIZE,smpsl),AuSample(SOUND_BUFFER_SIZE,smpsr));
    out(input);
}

void Chorus::out(const Stereo<AuSample> &input)
{
    const REALTYPE one=1.0;
    dl1=dl2;
    dr1=dr2;
    lfo.effectlfoout(&lfol,&lfor);

    dl2=getdelay(lfol);
    dr2=getdelay(lfor);

    for (int i=0;i<input.l().size();i++) {
        REALTYPE inl=input.l()[i];
        REALTYPE inr=input.r()[i];
        //LRcross
        Stereo<REALTYPE> tmpc(inl,inr);
        //REALTYPE r=inr;
        inl=tmpc.l()*(1.0-lrcross)+tmpc.r()*lrcross;
        inr=tmpc.r()*(1.0-lrcross)+tmpc.l()*lrcross;

        //Left channel

        //compute the delay in samples using linear interpolation between the lfo delays
        mdel=(dl1*(SOUND_BUFFER_SIZE-i)+dl2*i)/SOUND_BUFFER_SIZE;
        if (++dlk>=maxdelay) dlk=0;
        REALTYPE tmp=dlk-mdel+maxdelay*2.0;//where should I get the sample from

        F2I(tmp,dlhi);
        dlhi%=maxdelay;

        dlhi2=(dlhi-1+maxdelay)%maxdelay;
        dllo=1.0-fmod(tmp,one);
        efxoutl[i]=delaySample.l()[dlhi2]*dllo+delaySample.l()[dlhi]*(1.0-dllo);
        delaySample.l()[dlk]=inl+efxoutl[i]*fb;

        //Right channel

        //compute the delay in samples using linear interpolation between the lfo delays
        mdel=(dr1*(SOUND_BUFFER_SIZE-i)+dr2*i)/SOUND_BUFFER_SIZE;
        if (++drk>=maxdelay) drk=0;
        tmp=drk*1.0-mdel+maxdelay*2.0;//where should I get the sample from

        F2I(tmp,dlhi);
        dlhi%=maxdelay;

        dlhi2=(dlhi-1+maxdelay)%maxdelay;
        dllo=1.0-fmod(tmp,one);
        efxoutr[i]=delaySample.r()[dlhi2]*dllo+delaySample.r()[dlhi]*(1.0-dllo);
        delaySample.r()[dlk]=inr+efxoutr[i]*fb;

    };

    if (Poutsub!=0)
        for (int i=0;i<input.l().size();i++) {
            efxoutl[i] *= -1.0;
            efxoutr[i] *= -1.0;
        };


    for (int i=0;i<input.l().size();i++) {
        efxoutl[i]*=panning;
        efxoutr[i]*=(1.0-panning);
    };
};

/*
 * Cleanup the effect
 */
void Chorus::cleanup()
{
    delaySample.l().clear();
    delaySample.r().clear();
};

/*
 * Parameter control
 */
void Chorus::setdepth(const unsigned char &Pdepth)
{
    this->Pdepth=Pdepth;
    depth=(pow(8.0,(Pdepth/127.0)*2.0)-1.0)/1000.0;//seconds
};

void Chorus::setdelay(const unsigned char &Pdelay)
{
    this->Pdelay=Pdelay;
    delay=(pow(10.0,(Pdelay/127.0)*2.0)-1.0)/1000.0;//seconds
};

void Chorus::setfb(const unsigned char &Pfb)
{
    this->Pfb=Pfb;
    fb=(Pfb-64.0)/64.1;
};
void Chorus::setvolume(const unsigned char &Pvolume)
{
    this->Pvolume=Pvolume;
    outvolume=Pvolume/127.0;
    if (insertion==0) volume=1.0;
    else volume=outvolume;
};

void Chorus::setpanning(const unsigned char &Ppanning)
{
    this->Ppanning=Ppanning;
    panning=Ppanning/127.0;
};

void Chorus::setlrcross(const unsigned char &Plrcross)
{
    this->Plrcross=Plrcross;
    lrcross=Plrcross/127.0;
};

void Chorus::setpreset(unsigned char npreset)
{
    const int PRESET_SIZE=12;
    const int NUM_PRESETS=10;
    unsigned char presets[NUM_PRESETS][PRESET_SIZE]={
        //Chorus1
        {64,64,50,0,0,90,40,85,64,119,0,0},
        //Chorus2
        {64,64,45,0,0,98,56,90,64,19,0,0},
        //Chorus3
        {64,64,29,0,1,42,97,95,90,127,0,0},
        //Celeste1
        {64,64,26,0,0,42,115,18,90,127,0,0},
        //Celeste2
        {64,64,29,117,0,50,115,9,31,127,0,1},
        //Flange1
        {64,64,57,0,0,60,23,3,62,0,0,0},
        //Flange2
        {64,64,33,34,1,40,35,3,109,0,0,0},
        //Flange3
        {64,64,53,34,1,94,35,3,54,0,0,1},
        //Flange4
        {64,64,40,0,1,62,12,19,97,0,0,0},
        //Flange5
        {64,64,55,105,0,24,39,19,17,0,0,1}
    };

    if (npreset>=NUM_PRESETS) npreset=NUM_PRESETS-1;
    for (int n=0;n<PRESET_SIZE;n++) changepar(n,presets[npreset][n]);
    Ppreset=npreset;
};


void Chorus::changepar(const int &npar,const unsigned char &value)
{
    switch (npar) {
    case 0:
        setvolume(value);
        break;
    case 1:
        setpanning(value);
        break;
    case 2:
        lfo.Pfreq=value;
        lfo.updateparams();
        break;
    case 3:
        lfo.Prandomness=value;
        lfo.updateparams();
        break;
    case 4:
        lfo.PLFOtype=value;
        lfo.updateparams();
        break;
    case 5:
        lfo.Pstereo=value;
        lfo.updateparams();
        break;
    case 6:
        setdepth(value);
        break;
    case 7:
        setdelay(value);
        break;
    case 8:
        setfb(value);
        break;
    case 9:
        setlrcross(value);
        break;
    case 10:
        if (value>1) Pflangemode=1;
        else Pflangemode=value;
        break;
    case 11:
        if (value>1) Poutsub=1;
        else Poutsub=value;
        break;
    };
};

unsigned char Chorus::getpar(const int &npar)const
{
    switch (npar) {
    case 0:
        return(Pvolume);
        break;
    case 1:
        return(Ppanning);
        break;
    case 2:
        return(lfo.Pfreq);
        break;
    case 3:
        return(lfo.Prandomness);
        break;
    case 4:
        return(lfo.PLFOtype);
        break;
    case 5:
        return(lfo.Pstereo);
        break;
    case 6:
        return(Pdepth);
        break;
    case 7:
        return(Pdelay);
        break;
    case 8:
        return(Pfb);
        break;
    case 9:
        return(Plrcross);
        break;
    case 10:
        return(Pflangemode);
        break;
    case 11:
        return(Poutsub);
        break;
    default:
        return (0);
    };

};

