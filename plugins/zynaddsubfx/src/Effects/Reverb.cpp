/*
  ZynAddSubFX - a software synthesizer

  Reverb.C - Reverberation effect
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
#include "Reverb.h"

/**\todo: EarlyReflections,Prdelay,Perbalance */

Reverb::Reverb(const int &insertion_,REALTYPE *efxoutl_,REALTYPE *efxoutr_)
        :Effect(insertion_,efxoutl_,efxoutr_,NULL,0)
{
    inputbuf=new REALTYPE[SOUND_BUFFER_SIZE];

    //defaults
    Pvolume=48;
    Ppan=64;
    Ptime=64;
    Pidelay=40;
    Pidelayfb=0;
    Prdelay=0;
    Plpf=127;
    Phpf=0;
    Perbalance=64;
    Plohidamp=80;
    Ptype=1;
    Proomsize=64;
    roomsize=1.0;
    rs=1.0;

    for (int i=0;i<REV_COMBS*2;i++) {
        comblen[i]=800+(int)(RND*1400);
        combk[i]=0;
        lpcomb[i]=0;
        combfb[i]=-0.97;
        comb[i]=NULL;
    };

    for (int i=0;i<REV_APS*2;i++) {
        aplen[i]=500+(int)(RND*500);
        apk[i]=0;
        ap[i]=NULL;
    };

    lpf=NULL;
    hpf=NULL;//no filter
    idelay=NULL;

    setpreset(Ppreset);
    cleanup();//do not call this before the comb initialisation
};


Reverb::~Reverb()
{
    int i;
    if (idelay!=NULL) delete []idelay;
    if (hpf!=NULL) delete hpf;
    if (lpf!=NULL) delete lpf;

    for (i=0;i<REV_APS*2;i++) delete [] ap[i];
    for (i=0;i<REV_COMBS*2;i++) delete [] comb[i];

    delete [] inputbuf;
};

/*
 * Cleanup the effect
 */
void Reverb::cleanup()
{
    int i,j;
    for (i=0;i<REV_COMBS*2;i++) {
        lpcomb[i]=0.0;
        for (j=0;j<comblen[i];j++) comb[i][j]=0.0;
    };

    for (i=0;i<REV_APS*2;i++)
        for (j=0;j<aplen[i];j++) ap[i][j]=0.0;

    if (idelay!=NULL) for (i=0;i<idelaylen;i++) idelay[i]=0.0;

    if (hpf!=NULL) hpf->cleanup();
    if (lpf!=NULL) lpf->cleanup();

};

/*
 * Process one channel; 0=left,1=right
 */
void Reverb::processmono(int ch,REALTYPE *output)
{
    int i,j;
    REALTYPE fbout,tmp;
    /**\todo: implement the high part from lohidamp*/

    for (j=REV_COMBS*ch;j<REV_COMBS*(ch+1);j++) {

        int ck=combk[j];
        int comblength=comblen[j];
        REALTYPE lpcombj=lpcomb[j];

        for (i=0;i<SOUND_BUFFER_SIZE;i++) {
            fbout=comb[j][ck]*combfb[j];
            fbout=fbout*(1.0-lohifb)+lpcombj*lohifb;
            lpcombj=fbout;

            comb[j][ck]=inputbuf[i]+fbout;
            output[i]+=fbout;

            if ((++ck)>=comblength) ck=0;
        };

        combk[j]=ck;
        lpcomb[j]=lpcombj;
    };

    for (j=REV_APS*ch;j<REV_APS*(1+ch);j++) {
        int ak=apk[j];
        int aplength=aplen[j];
        for (i=0;i<SOUND_BUFFER_SIZE;i++) {
            tmp=ap[j][ak];
            ap[j][ak]=0.7*tmp+output[i];
            output[i]=tmp-0.7*ap[j][ak];
            if ((++ak)>=aplength) ak=0;
        };
        apk[j]=ak;
    };
};

/*
 * Effect output
 */
void Reverb::out(REALTYPE *smps_l, REALTYPE *smps_r)
{
    int i;
    if ((Pvolume==0)&&(insertion!=0)) return;

    for (i=0;i<SOUND_BUFFER_SIZE;i++) {
        inputbuf[i]=(smps_l[i]+smps_r[i])/2.0;
        //Initial delay r
        if (idelay!=NULL) {
            REALTYPE tmp=inputbuf[i]+idelay[idelayk]*idelayfb;
            inputbuf[i]=idelay[idelayk];
            idelay[idelayk]=tmp;
            idelayk++;
            if (idelayk>=idelaylen) idelayk=0;
        };
    };

    if (lpf!=NULL) lpf->filterout(inputbuf);
    if (hpf!=NULL) hpf->filterout(inputbuf);

    processmono(0,efxoutl);//left
    processmono(1,efxoutr);//right

    REALTYPE lvol=rs/REV_COMBS*pan;
    REALTYPE rvol=rs/REV_COMBS*(1.0-pan);
    if (insertion!=0) {
        lvol*=2;
        rvol*=2;
    };
    for (int i=0;i<SOUND_BUFFER_SIZE;i++) {
        efxoutl[i]*=lvol;
        efxoutr[i]*=rvol;
    };
};


/*
 * Parameter control
 */
void Reverb::setvolume(const unsigned char &Pvolume)
{
    this->Pvolume=Pvolume;
    if (insertion==0) {
        outvolume=pow(0.01,(1.0-Pvolume/127.0))*4.0;
        volume=1.0;
    } else {
        volume=outvolume=Pvolume/127.0;
        if (Pvolume==0) cleanup();
    };
};

void Reverb::setpan(const unsigned char &Ppan)
{
    this->Ppan=Ppan;
    pan=(REALTYPE)Ppan/127.0;
};

void Reverb::settime(const unsigned char &Ptime)
{
    int i;
    REALTYPE t;
    this->Ptime=Ptime;
    t=pow(60.0,(REALTYPE)Ptime/127.0)-0.97;

    for (i=0;i<REV_COMBS*2;i++) {
        combfb[i]=-exp((REALTYPE)comblen[i]/(REALTYPE)SAMPLE_RATE*log(0.001)/t);
        //the feedback is negative because it removes the DC
    };
};

void Reverb::setlohidamp(unsigned char Plohidamp)
{
    REALTYPE x;

    if (Plohidamp<64) Plohidamp=64;//remove this when the high part from lohidamp will be added

    this->Plohidamp=Plohidamp;
    if (Plohidamp==64) {
        lohidamptype=0;
        lohifb=0.0;
    } else {
        if (Plohidamp<64) lohidamptype=1;
        if (Plohidamp>64) lohidamptype=2;
        x=fabs((REALTYPE)(Plohidamp-64)/64.1);
        lohifb=x*x;
    };
};

void Reverb::setidelay(const unsigned char &Pidelay)
{
    REALTYPE delay;
    this->Pidelay=Pidelay;
    delay=pow(50*Pidelay/127.0,2)-1.0;

    if (idelay!=NULL) delete []idelay;
    idelay=NULL;

    idelaylen=(int) (SAMPLE_RATE*delay/1000);
    if (idelaylen>1) {
        idelayk=0;
        idelay=new REALTYPE[idelaylen];
        for (int i=0;i<idelaylen;i++) idelay[i]=0.0;
    };
};

void Reverb::setidelayfb(const unsigned char &Pidelayfb)
{
    this->Pidelayfb=Pidelayfb;
    idelayfb=Pidelayfb/128.0;
};

void Reverb::sethpf(const unsigned char &Phpf)
{
    this->Phpf=Phpf;
    if (Phpf==0) {//No HighPass
        if (hpf!=NULL) delete hpf;
        hpf=NULL;
    } else {
        REALTYPE fr=exp(pow(Phpf/127.0,0.5)*log(10000.0))+20.0;
        if (hpf==NULL) hpf=new AnalogFilter(3,fr,1,0);
        else hpf->setfreq(fr);
    };
};

void Reverb::setlpf(const unsigned char &Plpf)
{
    this->Plpf=Plpf;
    if (Plpf==127) {//No LowPass
        if (lpf!=NULL) delete lpf;
        lpf=NULL;
    } else {
        REALTYPE fr=exp(pow(Plpf/127.0,0.5)*log(25000.0))+40;
        if (lpf==NULL) lpf=new AnalogFilter(2,fr,1,0);
        else lpf->setfreq(fr);
    };
};

void Reverb::settype(unsigned char Ptype)
{
    const int NUM_TYPES=2;
    int combtunings[NUM_TYPES][REV_COMBS]={
        //this is unused (for random)
        {0,0,0,0,0,0,0,0},
        //Freeverb by Jezar at Dreampoint
        {1116,1188,1277,1356,1422,1491,1557,1617}
    };
    int aptunings[NUM_TYPES][REV_APS]={
        //this is unused (for random)
        {0,0,0,0},
        //Freeverb by Jezar at Dreampoint
        {225,341,441,556}
    };

    if (Ptype>=NUM_TYPES) Ptype=NUM_TYPES-1;
    this->Ptype=Ptype;

    REALTYPE tmp;
    for (int i=0;i<REV_COMBS*2;i++) {
        if (Ptype==0) tmp=800.0+(int)(RND*1400.0);
        else tmp=combtunings[Ptype][i%REV_COMBS];
        tmp*=roomsize;
        if (i>REV_COMBS) tmp+=23.0;
        tmp*=SAMPLE_RATE/44100.0;//adjust the combs according to the samplerate
        if (tmp<10) tmp=10;

        comblen[i]=(int) tmp;
        combk[i]=0;
        lpcomb[i]=0;
        if (comb[i]!=NULL) delete []comb[i];
        comb[i]=new REALTYPE[comblen[i]];
    };

    for (int i=0;i<REV_APS*2;i++) {
        if (Ptype==0) tmp=500+(int)(RND*500);
        else tmp=aptunings[Ptype][i%REV_APS];
        tmp*=roomsize;
        if (i>REV_APS) tmp+=23.0;
        tmp*=SAMPLE_RATE/44100.0;//adjust the combs according to the samplerate
        if (tmp<10) tmp=10;
        aplen[i]=(int) tmp;
        apk[i]=0;
        if (ap[i]!=NULL) delete []ap[i];
        ap[i]=new REALTYPE[aplen[i]];
    };
    settime(Ptime);
    cleanup();
};

void Reverb::setroomsize(const unsigned char &Proomsize)
{
    this->Proomsize=Proomsize;
    if (Proomsize==0) this->Proomsize=64;//this is because the older versions consider roomsize=0
    roomsize=(this->Proomsize-64.0)/64.0;
    if (roomsize>0.0) roomsize*=2.0;
    roomsize=pow(10.0,roomsize);
    rs=sqrt(roomsize);
    settype(Ptype);
};

void Reverb::setpreset(unsigned char npreset)
{
    const int PRESET_SIZE=12;
    const int NUM_PRESETS=13;
    unsigned char presets[NUM_PRESETS][PRESET_SIZE]={
        //Cathedral1
        {80,64,63,24,0,0,0,85,5,83,1,64},
        //Cathedral2
        {80,64,69,35,0,0,0,127,0,71,0,64},
        //Cathedral3
        {80,64,69,24,0,0,0,127,75,78,1,85},
        //Hall1
        {90,64,51,10,0,0,0,127,21,78,1,64},
        //Hall2
        {90,64,53,20,0,0,0,127,75,71,1,64},
        //Room1
        {100,64,33,0,0,0,0,127,0,106,0,30},
        //Room2
        {100,64,21,26,0,0,0,62,0,77,1,45},
        //Basement
        {110,64,14,0,0,0,0,127,5,71,0,25},
        //Tunnel
        {85,80,84,20,42,0,0,51,0,78,1,105},
        //Echoed1
        {95,64,26,60,71,0,0,114,0,64,1,64},
        //Echoed2
        {90,64,40,88,71,0,0,114,0,88,1,64},
        //VeryLong1
        {90,64,93,15,0,0,0,114,0,77,0,95},
        //VeryLong2
        {90,64,111,30,0,0,0,114,90,74,1,80}
    };

    if (npreset>=NUM_PRESETS) npreset=NUM_PRESETS-1;
    for (int n=0;n<PRESET_SIZE;n++) changepar(n,presets[npreset][n]);
    if (insertion!=0) changepar(0,presets[npreset][0]/2);//lower the volume if reverb is insertion effect
    Ppreset=npreset;
};


void Reverb::changepar(const int &npar,const unsigned char &value)
{
    switch (npar) {
    case 0:
        setvolume(value);
        break;
    case 1:
        setpan(value);
        break;
    case 2:
        settime(value);
        break;
    case 3:
        setidelay(value);
        break;
    case 4:
        setidelayfb(value);
        break;
//  case 5: setrdelay(value);
//      break;
//  case 6: seterbalance(value);
//      break;
    case 7:
        setlpf(value);
        break;
    case 8:
        sethpf(value);
        break;
    case 9:
        setlohidamp(value);
        break;
    case 10:
        settype(value);
        break;
    case 11:
        setroomsize(value);
        break;
    };
};

unsigned char Reverb::getpar(const int &npar)const
{
    switch (npar) {
    case 0:
        return(Pvolume);
        break;
    case 1:
        return(Ppan);
        break;
    case 2:
        return(Ptime);
        break;
    case 3:
        return(Pidelay);
        break;
    case 4:
        return(Pidelayfb);
        break;
//  case 5: return(Prdelay);
//      break;
//  case 6: return(Perbalance);
//      break;
    case 7:
        return(Plpf);
        break;
    case 8:
        return(Phpf);
        break;
    case 9:
        return(Plohidamp);
        break;
    case 10:
        return(Ptype);
        break;
    case 11:
        return(Proomsize);
        break;
    };
    return(0);//in case of bogus "parameter"
};

