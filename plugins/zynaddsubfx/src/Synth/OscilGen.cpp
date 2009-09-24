/*
  ZynAddSubFX - a software synthesizer

  OscilGen.C - Waveform generator for ADnote
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

#include <stdlib.h>
#include <math.h>
#include <stdio.h>

#include "OscilGen.h"
#include "../Effects/Distorsion.h"

OscilGen::OscilGen(FFTwrapper *fft_,Resonance *res_):Presets()
{
    setpresettype("Poscilgen");
    fft=fft_;
    res=res_;

    tmpsmps = new REALTYPE[OSCIL_SIZE];
    newFFTFREQS(&outoscilFFTfreqs, OSCIL_SIZE/2);
    newFFTFREQS(&oscilFFTfreqs,OSCIL_SIZE/2);
    newFFTFREQS(&basefuncFFTfreqs,OSCIL_SIZE/2);

    randseed=1;
    ADvsPAD=false;

    defaults();
};

OscilGen::~OscilGen()
{
    delete[] tmpsmps;
    deleteFFTFREQS(&outoscilFFTfreqs);
    deleteFFTFREQS(&basefuncFFTfreqs);
    deleteFFTFREQS(&oscilFFTfreqs);
};


void OscilGen::defaults()
{

    oldbasefunc=0;
    oldbasepar=64;
    oldhmagtype=0;
    oldwaveshapingfunction=0;
    oldwaveshaping=64;
    oldbasefuncmodulation=0;
    oldharmonicshift=0;
    oldbasefuncmodulationpar1=0;
    oldbasefuncmodulationpar2=0;
    oldbasefuncmodulationpar3=0;
    oldmodulation=0;
    oldmodulationpar1=0;
    oldmodulationpar2=0;
    oldmodulationpar3=0;

    for (int i=0;i<MAX_AD_HARMONICS;i++) {
        hmag[i]=0.0;
        hphase[i]=0.0;
        Phmag[i]=64;
        Phphase[i]=64;
    };
    Phmag[0]=127;
    Phmagtype=0;
    if (ADvsPAD) Prand=127;//max phase randomness (usefull if the oscil will be imported to a ADsynth from a PADsynth
    else Prand=64;//no randomness

    Pcurrentbasefunc=0;
    Pbasefuncpar=64;

    Pbasefuncmodulation=0;
    Pbasefuncmodulationpar1=64;
    Pbasefuncmodulationpar2=64;
    Pbasefuncmodulationpar3=32;

    Pmodulation=0;
    Pmodulationpar1=64;
    Pmodulationpar2=64;
    Pmodulationpar3=32;

    Pwaveshapingfunction=0;
    Pwaveshaping=64;
    Pfiltertype=0;
    Pfilterpar1=64;
    Pfilterpar2=64;
    Pfilterbeforews=0;
    Psatype=0;
    Psapar=64;

    Pamprandpower=64;
    Pamprandtype=0;

    Pharmonicshift=0;
    Pharmonicshiftfirst=0;

    Padaptiveharmonics=0;
    Padaptiveharmonicspower=100;
    Padaptiveharmonicsbasefreq=128;
    Padaptiveharmonicspar=50;

    for (int i=0;i<OSCIL_SIZE/2;i++) {
        oscilFFTfreqs.s[i]=0.0;
        oscilFFTfreqs.c[i]=0.0;
        basefuncFFTfreqs.s[i]=0.0;
        basefuncFFTfreqs.c[i]=0.0;
    };
    oscilprepared=0;
    oldfilterpars=0;
    oldsapars=0;
    prepare();
};

void OscilGen::convert2sine(int magtype)
{
    REALTYPE mag[MAX_AD_HARMONICS],phase[MAX_AD_HARMONICS];
    REALTYPE oscil[OSCIL_SIZE];
    FFTFREQS freqs;
    newFFTFREQS(&freqs,OSCIL_SIZE/2);

    get(oscil,-1.0);
    FFTwrapper *fft=new FFTwrapper(OSCIL_SIZE);
    fft->smps2freqs(oscil,freqs);
    delete(fft);

    REALTYPE max=0.0;

    mag[0]=0;
    phase[0]=0;
    for (int i=0;i<MAX_AD_HARMONICS;i++) {
        mag[i]=sqrt(pow(freqs.s[i+1],2)+pow(freqs.c[i+1],2.0));
        phase[i]=atan2(freqs.c[i+1],freqs.s[i+1]);
        if (max<mag[i]) max=mag[i];
    };
    if (max<0.00001) max=1.0;

    defaults();

    for (int i=0;i<MAX_AD_HARMONICS-1;i++) {
        REALTYPE newmag=mag[i]/max;
        REALTYPE newphase=phase[i];

        Phmag[i]=(int) ((newmag)*64.0)+64;

        Phphase[i]=64-(int) (64.0*newphase/PI);
        if (Phphase[i]>127) Phphase[i]=127;

        if (Phmag[i]==64) Phphase[i]=64;
    };
    deleteFFTFREQS(&freqs);
    prepare();
};

/*
 * Base Functions - START
 */
REALTYPE OscilGen::basefunc_pulse(REALTYPE x,REALTYPE a)
{
    return((fmod(x,1.0)<a)?-1.0:1.0);
};

REALTYPE OscilGen::basefunc_saw(REALTYPE x,REALTYPE a)
{
    if (a<0.00001) a=0.00001;
    else if (a>0.99999) a=0.99999;
    x=fmod(x,1);
    if (x<a) return(x/a*2.0-1.0);
    else return((1.0-x)/(1.0-a)*2.0-1.0);
};

REALTYPE OscilGen::basefunc_triangle(REALTYPE x,REALTYPE a)
{
    x=fmod(x+0.25,1);
    a=1-a;
    if (a<0.00001) a=0.00001;
    if (x<0.5) x=x*4-1.0;
    else x=(1.0-x)*4-1.0;
    x/=-a;
    if (x<-1.0) x=-1.0;
    if (x>1.0) x=1.0;
    return(x);
};

REALTYPE OscilGen::basefunc_power(REALTYPE x,REALTYPE a)
{
    x=fmod(x,1);
    if (a<0.00001) a=0.00001;
    else if (a>0.99999) a=0.99999;
    return(pow(x,exp((a-0.5)*10.0))*2.0-1.0);
};

REALTYPE OscilGen::basefunc_gauss(REALTYPE x,REALTYPE a)
{
    x=fmod(x,1)*2.0-1.0;
    if (a<0.00001) a=0.00001;
    return(exp(-x*x*(exp(a*8)+5.0))*2.0-1.0);
};

REALTYPE OscilGen::basefunc_diode(REALTYPE x,REALTYPE a)
{
    if (a<0.00001) a=0.00001;
    else if (a>0.99999) a=0.99999;
    a=a*2.0-1.0;
    x=cos((x+0.5)*2.0*PI)-a;
    if (x<0.0) x=0.0;
    return(x/(1.0-a)*2-1.0);
};

REALTYPE OscilGen::basefunc_abssine(REALTYPE x,REALTYPE a)
{
    x=fmod(x,1);
    if (a<0.00001) a=0.00001;
    else if (a>0.99999) a=0.99999;
    return(sin(pow(x,exp((a-0.5)*5.0))*PI)*2.0-1.0);
};

REALTYPE OscilGen::basefunc_pulsesine(REALTYPE x,REALTYPE a)
{
    if (a<0.00001) a=0.00001;
    x=(fmod(x,1)-0.5)*exp((a-0.5)*log(128));
    if (x<-0.5) x=-0.5;
    else if (x>0.5) x=0.5;
    x=sin(x*PI*2.0);
    return(x);
};

REALTYPE OscilGen::basefunc_stretchsine(REALTYPE x,REALTYPE a)
{
    x=fmod(x+0.5,1)*2.0-1.0;
    a=(a-0.5)*4;
    if (a>0.0) a*=2;
    a=pow(3.0,a);
    REALTYPE b=pow(fabs(x),a);
    if (x<0) b=-b;
    return(-sin(b*PI));
};

REALTYPE OscilGen::basefunc_chirp(REALTYPE x,REALTYPE a)
{
    x=fmod(x,1.0)*2.0*PI;
    a=(a-0.5)*4;
    if (a<0.0) a*=2.0;
    a=pow(3.0,a);
    return(sin(x/2.0)*sin(a*x*x));
};

REALTYPE OscilGen::basefunc_absstretchsine(REALTYPE x,REALTYPE a)
{
    x=fmod(x+0.5,1)*2.0-1.0;
    a=(a-0.5)*9;
    a=pow(3.0,a);
    REALTYPE b=pow(fabs(x),a);
    if (x<0) b=-b;
    return(-pow(sin(b*PI),2));
};

REALTYPE OscilGen::basefunc_chebyshev(REALTYPE x,REALTYPE a)
{
    a=a*a*a*30.0+1.0;
    return(cos(acos(x*2.0-1.0)*a));
};

REALTYPE OscilGen::basefunc_sqr(REALTYPE x,REALTYPE a)
{
    a=a*a*a*a*160.0+0.001;
    return(-atan(sin(x*2.0*PI)*a));
};
/*
 * Base Functions - END
 */


/*
 * Get the base function
 */
void OscilGen::getbasefunction(REALTYPE *smps)
{
    int i;
    REALTYPE par=(Pbasefuncpar+0.5)/128.0;
    if (Pbasefuncpar==64) par=0.5;

    REALTYPE basefuncmodulationpar1=Pbasefuncmodulationpar1/127.0,
                                    basefuncmodulationpar2=Pbasefuncmodulationpar2/127.0,
                                                           basefuncmodulationpar3=Pbasefuncmodulationpar3/127.0;

    switch (Pbasefuncmodulation) {
    case 1:
        basefuncmodulationpar1=(pow(2,basefuncmodulationpar1*5.0)-1.0)/10.0;
        basefuncmodulationpar3=floor((pow(2,basefuncmodulationpar3*5.0)-1.0));
        if (basefuncmodulationpar3<0.9999) basefuncmodulationpar3=-1.0;
        break;
    case 2:
        basefuncmodulationpar1=(pow(2,basefuncmodulationpar1*5.0)-1.0)/10.0;
        basefuncmodulationpar3=1.0+floor((pow(2,basefuncmodulationpar3*5.0)-1.0));
        break;
    case 3:
        basefuncmodulationpar1=(pow(2,basefuncmodulationpar1*7.0)-1.0)/10.0;
        basefuncmodulationpar3=0.01+(pow(2,basefuncmodulationpar3*16.0)-1.0)/10.0;
        break;
    };

//    printf("%.5f %.5f\n",basefuncmodulationpar1,basefuncmodulationpar3);

    for (i=0;i<OSCIL_SIZE;i++) {
        REALTYPE t=i*1.0/OSCIL_SIZE;

        switch (Pbasefuncmodulation) {
        case 1:
            t=t*basefuncmodulationpar3+sin((t+basefuncmodulationpar2)*2.0*PI)*basefuncmodulationpar1;//rev
            break;
        case 2:
            t=t+sin((t*basefuncmodulationpar3+basefuncmodulationpar2)*2.0*PI)*basefuncmodulationpar1;//sine
            break;
        case 3:
            t=t+pow((1.0-cos((t+basefuncmodulationpar2)*2.0*PI))*0.5,basefuncmodulationpar3)*basefuncmodulationpar1;//power
            break;
        };

        t=t-floor(t);

        switch (Pcurrentbasefunc) {
        case 1:
            smps[i]=basefunc_triangle(t,par);
            break;
        case 2:
            smps[i]=basefunc_pulse(t,par);
            break;
        case 3:
            smps[i]=basefunc_saw(t,par);
            break;
        case 4:
            smps[i]=basefunc_power(t,par);
            break;
        case 5:
            smps[i]=basefunc_gauss(t,par);
            break;
        case 6:
            smps[i]=basefunc_diode(t,par);
            break;
        case 7:
            smps[i]=basefunc_abssine(t,par);
            break;
        case 8:
            smps[i]=basefunc_pulsesine(t,par);
            break;
        case 9:
            smps[i]=basefunc_stretchsine(t,par);
            break;
        case 10:
            smps[i]=basefunc_chirp(t,par);
            break;
        case 11:
            smps[i]=basefunc_absstretchsine(t,par);
            break;
        case 12:
            smps[i]=basefunc_chebyshev(t,par);
            break;
        case 13:
            smps[i]=basefunc_sqr(t,par);
            break;
        default:
            smps[i]=-sin(2.0*PI*i/OSCIL_SIZE);
        };
    };
};

/*
 * Filter the oscillator
 */
void OscilGen::oscilfilter()
{
    if (Pfiltertype==0) return;
    REALTYPE par=1.0-Pfilterpar1/128.0;
    REALTYPE par2=Pfilterpar2/127.0;
    REALTYPE max=0.0,tmp=0.0,p2,x;
    for (int i=1;i<OSCIL_SIZE/2;i++) {
        REALTYPE gain=1.0;
        switch (Pfiltertype) {
        case 1:
            gain=pow(1.0-par*par*par*0.99,i);//lp
            tmp=par2*par2*par2*par2*0.5+0.0001;
            if (gain<tmp) gain=pow(gain,10.0)/pow(tmp,9.0);
            break;
        case 2:
            gain=1.0-pow(1.0-par*par,i+1);//hp1
            gain=pow(gain,par2*2.0+0.1);
            break;
        case 3:
            if (par<0.2) par=par*0.25+0.15;
            gain=1.0-pow(1.0-par*par*0.999+0.001,i*0.05*i+1.0);//hp1b
            tmp=pow(5.0,par2*2.0);
            gain=pow(gain,tmp);
            break;
        case 4:
            gain=i+1-pow(2,(1.0-par)*7.5);//bp1
            gain=1.0/(1.0+gain*gain/(i+1.0));
            tmp=pow(5.0,par2*2.0);
            gain=pow(gain,tmp);
            if (gain<1e-5) gain=1e-5;
            break;
        case 5:
            gain=i+1-pow(2,(1.0-par)*7.5);//bs1
            gain=pow(atan(gain/(i/10.0+1))/1.57,6);
            gain=pow(gain,par2*par2*3.9+0.1);
            break;
        case 6:
            tmp=pow(par2,0.33);
            gain=(i+1>pow(2,(1.0-par)*10)?0.0:1.0)*par2+(1.0-par2);//lp2
            break;
        case 7:
            tmp=pow(par2,0.33);
            //tmp=1.0-(1.0-par2)*(1.0-par2);
            gain=(i+1>pow(2,(1.0-par)*7)?1.0:0.0)*par2+(1.0-par2);//hp2
            if (Pfilterpar1==0) gain=1.0;
            break;
        case 8:
            tmp=pow(par2,0.33);
            //tmp=1.0-(1.0-par2)*(1.0-par2);
            gain=(fabs(pow(2,(1.0-par)*7)-i)>i/2+1?0.0:1.0)*par2+(1.0-par2);//bp2
            break;
        case 9:
            tmp=pow(par2,0.33);
            gain=(fabs(pow(2,(1.0-par)*7)-i)<i/2+1?0.0:1.0)*par2+(1.0-par2);//bs2
            break;
        case 10:
            tmp=pow(5.0,par2*2.0-1.0);
            tmp=pow(i/32.0,tmp)*32.0;
            if (Pfilterpar2==64) tmp=i;
            gain=cos(par*par*PI/2.0*tmp);//cos
            gain*=gain;
            break;
        case 11:
            tmp=pow(5.0,par2*2.0-1.0);
            tmp=pow(i/32.0,tmp)*32.0;
            if (Pfilterpar2==64) tmp=i;
            gain=sin(par*par*PI/2.0*tmp);//sin
            gain*=gain;
            break;
        case 12:
            p2=1.0-par+0.2;
            x=i/(64.0*p2*p2);
            if (x<0.0) x=0.0;
            else if (x>1.0) x=1.0;
            tmp=pow(1.0-par2,2.0);
            gain=cos(x*PI)*(1.0-tmp)+1.01+tmp;//low shelf
            break;
        case 13:
            tmp=(int) (pow(2.0,(1.0-par)*7.2));
            gain=1.0;
            if (i==(int) (tmp)) gain=pow(2.0,par2*par2*8.0);
            break;
        };


        oscilFFTfreqs.s[i]*=gain;
        oscilFFTfreqs.c[i]*=gain;
        REALTYPE tmp=oscilFFTfreqs.s[i]*oscilFFTfreqs.s[i]+
                     oscilFFTfreqs.c[i]*oscilFFTfreqs.c[i];
        if (max<tmp) max=tmp;
    };

    max=sqrt(max);
    if (max<1e-10) max=1.0;
    REALTYPE imax=1.0/max;
    for (int i=1;i<OSCIL_SIZE/2;i++) {
        oscilFFTfreqs.s[i]*=imax;
        oscilFFTfreqs.c[i]*=imax;
    };
};

/*
 * Change the base function
 */
void OscilGen::changebasefunction()
{
    if (Pcurrentbasefunc!=0) {
        getbasefunction(tmpsmps);
        fft->smps2freqs(tmpsmps,basefuncFFTfreqs);
        basefuncFFTfreqs.c[0]=0.0;
    } else {
        for (int i=0;i<OSCIL_SIZE/2;i++) {
            basefuncFFTfreqs.s[i]=0.0;
            basefuncFFTfreqs.c[i]=0.0;
        };
        //in this case basefuncFFTfreqs_ are not used
    }
    oscilprepared=0;
    oldbasefunc=Pcurrentbasefunc;
    oldbasepar=Pbasefuncpar;
    oldbasefuncmodulation=Pbasefuncmodulation;
    oldbasefuncmodulationpar1=Pbasefuncmodulationpar1;
    oldbasefuncmodulationpar2=Pbasefuncmodulationpar2;
    oldbasefuncmodulationpar3=Pbasefuncmodulationpar3;
};

/*
 * Waveshape
 */
void OscilGen::waveshape()
{
    int i;

    oldwaveshapingfunction=Pwaveshapingfunction;
    oldwaveshaping=Pwaveshaping;
    if (Pwaveshapingfunction==0) return;

    oscilFFTfreqs.c[0]=0.0;//remove the DC
    //reduce the amplitude of the freqs near the nyquist
    for (i=1;i<OSCIL_SIZE/8;i++) {
        REALTYPE tmp=i/(OSCIL_SIZE/8.0);
        oscilFFTfreqs.s[OSCIL_SIZE/2-i]*=tmp;
        oscilFFTfreqs.c[OSCIL_SIZE/2-i]*=tmp;
    };
    fft->freqs2smps(oscilFFTfreqs,tmpsmps);

    //Normalize
    REALTYPE max=0.0;
    for (i=0;i<OSCIL_SIZE;i++)
        if (max<fabs(tmpsmps[i])) max=fabs(tmpsmps[i]);
    if (max<0.00001) max=1.0;
    max=1.0/max;
    for (i=0;i<OSCIL_SIZE;i++) tmpsmps[i]*=max;

    //Do the waveshaping
    waveshapesmps(OSCIL_SIZE,tmpsmps,Pwaveshapingfunction,Pwaveshaping);

    fft->smps2freqs(tmpsmps,oscilFFTfreqs);//perform FFT
};


/*
 * Do the Frequency Modulation of the Oscil
 */
void OscilGen::modulation()
{
    int i;

    oldmodulation=Pmodulation;
    oldmodulationpar1=Pmodulationpar1;
    oldmodulationpar2=Pmodulationpar2;
    oldmodulationpar3=Pmodulationpar3;
    if (Pmodulation==0) return;


    REALTYPE modulationpar1=Pmodulationpar1/127.0,
                            modulationpar2=0.5-Pmodulationpar2/127.0,
                                           modulationpar3=Pmodulationpar3/127.0;

    switch (Pmodulation) {
    case 1:
        modulationpar1=(pow(2,modulationpar1*7.0)-1.0)/100.0;
        modulationpar3=floor((pow(2,modulationpar3*5.0)-1.0));
        if (modulationpar3<0.9999) modulationpar3=-1.0;
        break;
    case 2:
        modulationpar1=(pow(2,modulationpar1*7.0)-1.0)/100.0;
        modulationpar3=1.0+floor((pow(2,modulationpar3*5.0)-1.0));
        break;
    case 3:
        modulationpar1=(pow(2,modulationpar1*9.0)-1.0)/100.0;
        modulationpar3=0.01+(pow(2,modulationpar3*16.0)-1.0)/10.0;
        break;
    };

    oscilFFTfreqs.c[0]=0.0;//remove the DC
    //reduce the amplitude of the freqs near the nyquist
    for (i=1;i<OSCIL_SIZE/8;i++) {
        REALTYPE tmp=i/(OSCIL_SIZE/8.0);
        oscilFFTfreqs.s[OSCIL_SIZE/2-i]*=tmp;
        oscilFFTfreqs.c[OSCIL_SIZE/2-i]*=tmp;
    };
    fft->freqs2smps(oscilFFTfreqs,tmpsmps);
    int extra_points=2;
    REALTYPE *in=new REALTYPE[OSCIL_SIZE+extra_points];

    //Normalize
    REALTYPE max=0.0;
    for (i=0;i<OSCIL_SIZE;i++) if (max<fabs(tmpsmps[i])) max=fabs(tmpsmps[i]);
    if (max<0.00001) max=1.0;
    max=1.0/max;
    for (i=0;i<OSCIL_SIZE;i++) in[i]=tmpsmps[i]*max;
    for (i=0;i<extra_points;i++) in[i+OSCIL_SIZE]=tmpsmps[i]*max;

    //Do the modulation
    for (i=0;i<OSCIL_SIZE;i++) {
        REALTYPE t=i*1.0/OSCIL_SIZE;

        switch (Pmodulation) {
        case 1:
            t=t*modulationpar3+sin((t+modulationpar2)*2.0*PI)*modulationpar1;//rev
            break;
        case 2:
            t=t+sin((t*modulationpar3+modulationpar2)*2.0*PI)*modulationpar1;//sine
            break;
        case 3:
            t=t+pow((1.0-cos((t+modulationpar2)*2.0*PI))*0.5,modulationpar3)*modulationpar1;//power
            break;
        };

        t=(t-floor(t))*OSCIL_SIZE;

        int poshi=(int) t;
        REALTYPE poslo=t-floor(t);

        tmpsmps[i]=in[poshi]*(1.0-poslo)+in[poshi+1]*poslo;
    };

    delete [] in;
    fft->smps2freqs(tmpsmps,oscilFFTfreqs);//perform FFT
};



/*
 * Adjust the spectrum
 */
void OscilGen::spectrumadjust()
{
    if (Psatype==0) return;
    REALTYPE par=Psapar/127.0;
    switch (Psatype) {
    case 1:
        par=1.0-par*2.0;
        if (par>=0.0) par=pow(5.0,par);
        else par=pow(8.0,par);
        break;
    case 2:
        par=pow(10.0,(1.0-par)*3.0)*0.25;
        break;
    case 3:
        par=pow(10.0,(1.0-par)*3.0)*0.25;
        break;
    };


    REALTYPE max=0.0;
    for (int i=0;i<OSCIL_SIZE/2;i++) {
        REALTYPE tmp=pow(oscilFFTfreqs.c[i],2)+pow(oscilFFTfreqs.s[i],2.0);
        if (max<tmp) max=tmp;
    };
    max=sqrt(max)/OSCIL_SIZE*2.0;
    if (max<1e-8) max=1.0;


    for (int i=0;i<OSCIL_SIZE/2;i++) {
        REALTYPE mag=sqrt(pow(oscilFFTfreqs.s[i],2)+pow(oscilFFTfreqs.c[i],2.0))/max;
        REALTYPE phase=atan2(oscilFFTfreqs.s[i],oscilFFTfreqs.c[i]);

        switch (Psatype) {
        case 1:
            mag=pow(mag,par);
            break;
        case 2:
            if (mag<par) mag=0.0;
            break;
        case 3:
            mag/=par;
            if (mag>1.0) mag=1.0;
            break;
        };
        oscilFFTfreqs.c[i]=mag*cos(phase);
        oscilFFTfreqs.s[i]=mag*sin(phase);
    };

};

void OscilGen::shiftharmonics()
{
    if (Pharmonicshift==0) return;

    REALTYPE hc,hs;
    int harmonicshift=-Pharmonicshift;

    if (harmonicshift>0) {
        for (int i=OSCIL_SIZE/2-2;i>=0;i--) {
            int oldh=i-harmonicshift;
            if (oldh<0) {
                hc=0.0;
                hs=0.0;
            } else {
                hc=oscilFFTfreqs.c[oldh+1];
                hs=oscilFFTfreqs.s[oldh+1];
            };
            oscilFFTfreqs.c[i+1]=hc;
            oscilFFTfreqs.s[i+1]=hs;
        };
    } else {
        for (int i=0;i<OSCIL_SIZE/2-1;i++) {
            int oldh=i+abs(harmonicshift);
            if (oldh>=(OSCIL_SIZE/2-1)) {
                hc=0.0;
                hs=0.0;
            } else {
                hc=oscilFFTfreqs.c[oldh+1];
                hs=oscilFFTfreqs.s[oldh+1];
                if (fabs(hc)<0.000001) hc=0.0;
                if (fabs(hs)<0.000001) hs=0.0;
            };

            oscilFFTfreqs.c[i+1]=hc;
            oscilFFTfreqs.s[i+1]=hs;
        };
    };

    oscilFFTfreqs.c[0]=0.0;
};

/*
 * Prepare the Oscillator
 */
void OscilGen::prepare()
{
    int i,j,k;
    REALTYPE a,b,c,d,hmagnew;

    if ((oldbasepar!=Pbasefuncpar)||(oldbasefunc!=Pcurrentbasefunc)||
            (oldbasefuncmodulation!=Pbasefuncmodulation)||
            (oldbasefuncmodulationpar1!=Pbasefuncmodulationpar1)||
            (oldbasefuncmodulationpar2!=Pbasefuncmodulationpar2)||
            (oldbasefuncmodulationpar3!=Pbasefuncmodulationpar3))
        changebasefunction();

    for (i=0;i<MAX_AD_HARMONICS;i++) hphase[i]=(Phphase[i]-64.0)/64.0*PI/(i+1);

    for (i=0;i<MAX_AD_HARMONICS;i++) {
        hmagnew=1.0-fabs(Phmag[i]/64.0-1.0);
        switch (Phmagtype) {
        case 1:
            hmag[i]=exp(hmagnew*log(0.01));
            break;
        case 2:
            hmag[i]=exp(hmagnew*log(0.001));
            break;
        case 3:
            hmag[i]=exp(hmagnew*log(0.0001));
            break;
        case 4:
            hmag[i]=exp(hmagnew*log(0.00001));
            break;
        default:
            hmag[i]=1.0-hmagnew;
            break;
        };

        if (Phmag[i]<64) hmag[i]=-hmag[i];
    };

    //remove the harmonics where Phmag[i]==64
    for (i=0;i<MAX_AD_HARMONICS;i++) if (Phmag[i]==64) hmag[i]=0.0;


    for (i=0;i<OSCIL_SIZE/2;i++) {
        oscilFFTfreqs.c[i]=0.0;
        oscilFFTfreqs.s[i]=0.0;
    };
    if (Pcurrentbasefunc==0) {//the sine case
        for (i=0;i<MAX_AD_HARMONICS;i++) {
            oscilFFTfreqs.c[i+1]=-hmag[i]*sin(hphase[i]*(i+1))/2.0;
            oscilFFTfreqs.s[i+1]=hmag[i]*cos(hphase[i]*(i+1))/2.0;
        };
    } else {
        for (j=0;j<MAX_AD_HARMONICS;j++) {
            if (Phmag[j]==64) continue;
            for (i=1;i<OSCIL_SIZE/2;i++) {
                k=i*(j+1);
                if (k>=OSCIL_SIZE/2) break;
                a=basefuncFFTfreqs.c[i];
                b=basefuncFFTfreqs.s[i];
                c=hmag[j]*cos(hphase[j]*k);
                d=hmag[j]*sin(hphase[j]*k);
                oscilFFTfreqs.c[k]+=a*c-b*d;
                oscilFFTfreqs.s[k]+=a*d+b*c;
            };
        };

    };

    if (Pharmonicshiftfirst!=0)  shiftharmonics();



    if (Pfilterbeforews==0) {
        waveshape();
        oscilfilter();
    } else {
        oscilfilter();
        waveshape();
    };

    modulation();
    spectrumadjust();
    if (Pharmonicshiftfirst==0)  shiftharmonics();

    oscilFFTfreqs.c[0]=0.0;

    oldhmagtype=Phmagtype;
    oldharmonicshift=Pharmonicshift+Pharmonicshiftfirst*256;

    oscilprepared=1;
};

void OscilGen::adaptiveharmonic(FFTFREQS f,REALTYPE freq)
{
    if ((Padaptiveharmonics==0)/*||(freq<1.0)*/) return;
    if (freq<1.0) freq=440.0;

    FFTFREQS inf;
    newFFTFREQS(&inf,OSCIL_SIZE/2);
    for (int i=0;i<OSCIL_SIZE/2;i++) {
        inf.s[i]=f.s[i];
        inf.c[i]=f.c[i];
        f.s[i]=0.0;
        f.c[i]=0.0;
    };
    inf.c[0]=0.0;
    inf.s[0]=0.0;

    REALTYPE hc=0.0,hs=0.0;
    REALTYPE basefreq=30.0*pow(10.0,Padaptiveharmonicsbasefreq/128.0);
    REALTYPE power=(Padaptiveharmonicspower+1.0)/101.0;

    REALTYPE rap=freq/basefreq;

    rap=pow(rap,power);

    bool down=false;
    if (rap>1.0) {
        rap=1.0/rap;
        down=true;
    };

    for (int i=0;i<OSCIL_SIZE/2-2;i++) {
        REALTYPE h=i*rap;
        int high=(int)(i*rap);
        REALTYPE low=fmod(h,1.0);

        if (high>=(OSCIL_SIZE/2-2)) {
            break;
        } else {
            if (down) {
                f.c[high]+=inf.c[i]*(1.0-low);
                f.s[high]+=inf.s[i]*(1.0-low);
                f.c[high+1]+=inf.c[i]*low;
                f.s[high+1]+=inf.s[i]*low;
            } else {
                hc=inf.c[high]*(1.0-low)+inf.c[high+1]*low;
                hs=inf.s[high]*(1.0-low)+inf.s[high+1]*low;
            };
            if (fabs(hc)<0.000001) hc=0.0;
            if (fabs(hs)<0.000001) hs=0.0;
        };

        if (!down) {
            if (i==0) {//corect the aplitude of the first harmonic
                hc*=rap;
                hs*=rap;
            };
            f.c[i]=hc;
            f.s[i]=hs;
        };
    };

    f.c[1]+=f.c[0];
    f.s[1]+=f.s[0];
    f.c[0]=0.0;
    f.s[0]=0.0;
    deleteFFTFREQS(&inf);
};

void OscilGen::adaptiveharmonicpostprocess(REALTYPE *f,int size)
{
    if (Padaptiveharmonics<=1) return;
    REALTYPE *inf=new REALTYPE[size];
    REALTYPE par=Padaptiveharmonicspar*0.01;
    par=1.0-pow((1.0-par),1.5);

    for (int i=0;i<size;i++) {
        inf[i]=f[i]*par;
        f[i]=f[i]*(1.0-par);
    };


    if (Padaptiveharmonics==2) {//2n+1
        for (int i=0;i<size;i++) if ((i%2)==0) f[i]+=inf[i];//i=0 pt prima armonica,etc.
    } else {//celelalte moduri
        int nh=(Padaptiveharmonics-3)/2+2;
        int sub_vs_add=(Padaptiveharmonics-3)%2;
        if (sub_vs_add==0) {
            for (int i=0;i<size;i++) {
                if (((i+1)%nh)==0) {
                    f[i]+=inf[i];
                };
            };
        } else {
            for (int i=0;i<size/nh-1;i++) {
                f[(i+1)*nh-1]+=inf[i];
            };
        };
    };

    delete(inf);
};



/*
 * Get the oscillator function
 */
short int OscilGen::get(REALTYPE *smps,REALTYPE freqHz)
{
    return(this->get(smps,freqHz,0));
};

void OscilGen::newrandseed(unsigned int randseed)
{
    this->randseed=randseed;
};

/*
 * Get the oscillator function
 */
short int OscilGen::get(REALTYPE *smps,REALTYPE freqHz,int resonance)
{
    int i;
    int nyquist,outpos;

    if ((oldbasepar!=Pbasefuncpar)||(oldbasefunc!=Pcurrentbasefunc)||(oldhmagtype!=Phmagtype)
            ||(oldwaveshaping!=Pwaveshaping)||(oldwaveshapingfunction!=Pwaveshapingfunction)) oscilprepared=0;
    if (oldfilterpars!=Pfiltertype*256+Pfilterpar1+Pfilterpar2*65536+Pfilterbeforews*16777216) {
        oscilprepared=0;
        oldfilterpars=Pfiltertype*256+Pfilterpar1+Pfilterpar2*65536+Pfilterbeforews*16777216;
    };
    if (oldsapars!=Psatype*256+Psapar) {
        oscilprepared=0;
        oldsapars=Psatype*256+Psapar;
    };

    if ((oldbasefuncmodulation!=Pbasefuncmodulation)||
            (oldbasefuncmodulationpar1!=Pbasefuncmodulationpar1)||
            (oldbasefuncmodulationpar2!=Pbasefuncmodulationpar2)||
            (oldbasefuncmodulationpar3!=Pbasefuncmodulationpar3))
        oscilprepared=0;

    if ((oldmodulation!=Pmodulation)||
            (oldmodulationpar1!=Pmodulationpar1)||
            (oldmodulationpar2!=Pmodulationpar2)||
            (oldmodulationpar3!=Pmodulationpar3))
        oscilprepared=0;

    if (oldharmonicshift!=Pharmonicshift+Pharmonicshiftfirst*256) oscilprepared=0;

    if (oscilprepared!=1) prepare();

    outpos=(int)((RND*2.0-1.0)*(REALTYPE) OSCIL_SIZE*(Prand-64.0)/64.0);
    outpos=(outpos+2*OSCIL_SIZE) % OSCIL_SIZE;


    for (i=0;i<OSCIL_SIZE/2;i++) {
        outoscilFFTfreqs.c[i]=0.0;
        outoscilFFTfreqs.s[i]=0.0;
    };

    nyquist=(int)(0.5*SAMPLE_RATE/fabs(freqHz))+2;
    if (ADvsPAD) nyquist=(int)(OSCIL_SIZE/2);
    if (nyquist>OSCIL_SIZE/2) nyquist=OSCIL_SIZE/2;


    int realnyquist=nyquist;

    if (Padaptiveharmonics!=0) nyquist=OSCIL_SIZE/2;
    for (i=1;i<nyquist-1;i++) {
        outoscilFFTfreqs.c[i]=oscilFFTfreqs.c[i];
        outoscilFFTfreqs.s[i]=oscilFFTfreqs.s[i];
    };

    adaptiveharmonic(outoscilFFTfreqs,freqHz);
    adaptiveharmonicpostprocess(&outoscilFFTfreqs.c[1],OSCIL_SIZE/2-1);
    adaptiveharmonicpostprocess(&outoscilFFTfreqs.s[1],OSCIL_SIZE/2-1);

    nyquist=realnyquist;
    if (Padaptiveharmonics) {//do the antialiasing in the case of adaptive harmonics
        for (i=nyquist;i<OSCIL_SIZE/2;i++) {
            outoscilFFTfreqs.s[i]=0;
            outoscilFFTfreqs.c[i]=0;
        };
    };

    // Randomness (each harmonic), the block type is computed
    // in ADnote by setting start position according to this setting
    if ((Prand>64)&&(freqHz>=0.0)&&(!ADvsPAD)) {
        REALTYPE rnd,angle,a,b,c,d;
        rnd=PI*pow((Prand-64.0)/64.0,2.0);
        for (i=1;i<nyquist-1;i++) {//to Nyquist only for AntiAliasing
            angle=rnd*i*RND;
            a=outoscilFFTfreqs.c[i];
            b=outoscilFFTfreqs.s[i];
            c=cos(angle);
            d=sin(angle);
            outoscilFFTfreqs.c[i]=a*c-b*d;
            outoscilFFTfreqs.s[i]=a*d+b*c;
        };
    };

    //Harmonic Amplitude Randomness
    if ((freqHz>0.1)&&(!ADvsPAD)) {
        unsigned int realrnd=rand();
        srand(randseed);
        REALTYPE power=Pamprandpower/127.0;
        REALTYPE normalize=1.0/(1.2-power);
        switch (Pamprandtype) {
        case 1:
            power=power*2.0-0.5;
            power=pow(15.0,power);
            for (i=1;i<nyquist-1;i++) {
                REALTYPE amp=pow(RND,power)*normalize;
                outoscilFFTfreqs.c[i]*=amp;
                outoscilFFTfreqs.s[i]*=amp;
            };
            break;
        case 2:
            power=power*2.0-0.5;
            power=pow(15.0,power)*2.0;
            REALTYPE rndfreq=2*PI*RND;
            for (i=1;i<nyquist-1;i++) {
                REALTYPE amp=pow(fabs(sin(i*rndfreq)),power)*normalize;
                outoscilFFTfreqs.c[i]*=amp;
                outoscilFFTfreqs.s[i]*=amp;
            };
            break;
        };
        srand(realrnd+1);
    };

    if ((freqHz>0.1)&&(resonance!=0)) res->applyres(nyquist-1,outoscilFFTfreqs,freqHz);

    //Full RMS normalize
    REALTYPE sum=0;
    for (int j=1;j<OSCIL_SIZE/2;j++) {
        REALTYPE term=outoscilFFTfreqs.c[j]*outoscilFFTfreqs.c[j]
                      +outoscilFFTfreqs.s[j]*outoscilFFTfreqs.s[j];
        sum+=term;
    };
    if (sum<0.000001) sum=1.0;
    sum=1.0/sqrt(sum);
    for (int j=1;j<OSCIL_SIZE/2;j++) {
        outoscilFFTfreqs.c[j]*=sum;
        outoscilFFTfreqs.s[j]*=sum;
    };


    if ((ADvsPAD)&&(freqHz>0.1)) {//in this case the smps will contain the freqs
        for (i=1;i<OSCIL_SIZE/2;i++) smps[i-1]=sqrt(outoscilFFTfreqs.c[i]*outoscilFFTfreqs.c[i]
                                                   +outoscilFFTfreqs.s[i]*outoscilFFTfreqs.s[i]);
    } else {
        fft->freqs2smps(outoscilFFTfreqs,smps);
        for (i=0;i<OSCIL_SIZE;i++) smps[i]*=0.25;//correct the amplitude
    };

    if (Prand<64) return(outpos);
    else return(0);
};


/*
 * Get the spectrum of the oscillator for the UI
 */
void OscilGen::getspectrum(int n, REALTYPE *spc,int what)
{
    if (n>OSCIL_SIZE/2) n=OSCIL_SIZE/2;

    for (int i=1;i<n;i++) {
        if (what==0) {
            spc[i-1]=sqrt(oscilFFTfreqs.c[i]*oscilFFTfreqs.c[i]
                          +oscilFFTfreqs.s[i]*oscilFFTfreqs.s[i]);
        } else {
            if (Pcurrentbasefunc==0) spc[i-1]=((i==1)?(1.0):(0.0));
            else spc[i-1]=sqrt(basefuncFFTfreqs.c[i]*basefuncFFTfreqs.c[i]+
                                   basefuncFFTfreqs.s[i]*basefuncFFTfreqs.s[i]);
        };
    };

    if (what==0) {
        for (int i=0;i<n;i++) outoscilFFTfreqs.s[i]=outoscilFFTfreqs.c[i]=spc[i];
        for (int i=n;i<OSCIL_SIZE/2;i++) outoscilFFTfreqs.s[i]=outoscilFFTfreqs.c[i]=0.0;
        adaptiveharmonic(outoscilFFTfreqs,0.0);
        for (int i=0;i<n;i++) spc[i]=outoscilFFTfreqs.s[i];
        adaptiveharmonicpostprocess(spc,n-1);
    };
};


/*
 * Convert the oscillator as base function
 */
void OscilGen::useasbase()
{
    int i;

    for (i=0;i<OSCIL_SIZE/2;i++) {
        basefuncFFTfreqs.c[i]=oscilFFTfreqs.c[i];
        basefuncFFTfreqs.s[i]=oscilFFTfreqs.s[i];
    };

    oldbasefunc=Pcurrentbasefunc=127;

    prepare();
};


/*
 * Get the base function for UI
 */
void OscilGen::getcurrentbasefunction(REALTYPE *smps)
{
    if (Pcurrentbasefunc!=0) {
        fft->freqs2smps(basefuncFFTfreqs,smps);
    } else getbasefunction(smps);//the sine case
};


void OscilGen::add2XML(XMLwrapper *xml)
{
    xml->addpar("harmonic_mag_type",Phmagtype);

    xml->addpar("base_function",Pcurrentbasefunc);
    xml->addpar("base_function_par",Pbasefuncpar);
    xml->addpar("base_function_modulation",Pbasefuncmodulation);
    xml->addpar("base_function_modulation_par1",Pbasefuncmodulationpar1);
    xml->addpar("base_function_modulation_par2",Pbasefuncmodulationpar2);
    xml->addpar("base_function_modulation_par3",Pbasefuncmodulationpar3);

    xml->addpar("modulation",Pmodulation);
    xml->addpar("modulation_par1",Pmodulationpar1);
    xml->addpar("modulation_par2",Pmodulationpar2);
    xml->addpar("modulation_par3",Pmodulationpar3);

    xml->addpar("wave_shaping",Pwaveshaping);
    xml->addpar("wave_shaping_function",Pwaveshapingfunction);

    xml->addpar("filter_type",Pfiltertype);
    xml->addpar("filter_par1",Pfilterpar1);
    xml->addpar("filter_par2",Pfilterpar2);
    xml->addpar("filter_before_wave_shaping",Pfilterbeforews);

    xml->addpar("spectrum_adjust_type",Psatype);
    xml->addpar("spectrum_adjust_par",Psapar);

    xml->addpar("rand",Prand);
    xml->addpar("amp_rand_type",Pamprandtype);
    xml->addpar("amp_rand_power",Pamprandpower);

    xml->addpar("harmonic_shift",Pharmonicshift);
    xml->addparbool("harmonic_shift_first",Pharmonicshiftfirst);

    xml->addpar("adaptive_harmonics",Padaptiveharmonics);
    xml->addpar("adaptive_harmonics_base_frequency",Padaptiveharmonicsbasefreq);
    xml->addpar("adaptive_harmonics_power",Padaptiveharmonicspower);

    xml->beginbranch("HARMONICS");
    for (int n=0;n<MAX_AD_HARMONICS;n++) {
        if ((Phmag[n]==64)&&(Phphase[n]==64)) continue;
        xml->beginbranch("HARMONIC",n+1);
        xml->addpar("mag",Phmag[n]);
        xml->addpar("phase",Phphase[n]);
        xml->endbranch();
    };
    xml->endbranch();

    if (Pcurrentbasefunc==127) {
        REALTYPE max=0.0;

        for (int i=0;i<OSCIL_SIZE/2;i++) {
            if (max<fabs(basefuncFFTfreqs.c[i])) max=fabs(basefuncFFTfreqs.c[i]);
            if (max<fabs(basefuncFFTfreqs.s[i])) max=fabs(basefuncFFTfreqs.s[i]);
        };
        if (max<0.00000001) max=1.0;

        xml->beginbranch("BASE_FUNCTION");
        for (int i=1;i<OSCIL_SIZE/2;i++) {
            REALTYPE xc=basefuncFFTfreqs.c[i]/max;
            REALTYPE xs=basefuncFFTfreqs.s[i]/max;
            if ((fabs(xs)>0.00001)&&(fabs(xs)>0.00001)) {
                xml->beginbranch("BF_HARMONIC",i);
                xml->addparreal("cos",xc);
                xml->addparreal("sin",xs);
                xml->endbranch();
            };
        };
        xml->endbranch();
    };
};


void OscilGen::getfromXML(XMLwrapper *xml)
{

    Phmagtype=xml->getpar127("harmonic_mag_type",Phmagtype);

    Pcurrentbasefunc=xml->getpar127("base_function",Pcurrentbasefunc);
    Pbasefuncpar=xml->getpar127("base_function_par",Pbasefuncpar);

    Pbasefuncmodulation=xml->getpar127("base_function_modulation",Pbasefuncmodulation);
    Pbasefuncmodulationpar1=xml->getpar127("base_function_modulation_par1",Pbasefuncmodulationpar1);
    Pbasefuncmodulationpar2=xml->getpar127("base_function_modulation_par2",Pbasefuncmodulationpar2);
    Pbasefuncmodulationpar3=xml->getpar127("base_function_modulation_par3",Pbasefuncmodulationpar3);

    Pmodulation=xml->getpar127("modulation",Pmodulation);
    Pmodulationpar1=xml->getpar127("modulation_par1",Pmodulationpar1);
    Pmodulationpar2=xml->getpar127("modulation_par2",Pmodulationpar2);
    Pmodulationpar3=xml->getpar127("modulation_par3",Pmodulationpar3);

    Pwaveshaping=xml->getpar127("wave_shaping",Pwaveshaping);
    Pwaveshapingfunction=xml->getpar127("wave_shaping_function",Pwaveshapingfunction);

    Pfiltertype=xml->getpar127("filter_type",Pfiltertype);
    Pfilterpar1=xml->getpar127("filter_par1",Pfilterpar1);
    Pfilterpar2=xml->getpar127("filter_par2",Pfilterpar2);
    Pfilterbeforews=xml->getpar127("filter_before_wave_shaping",Pfilterbeforews);

    Psatype=xml->getpar127("spectrum_adjust_type",Psatype);
    Psapar=xml->getpar127("spectrum_adjust_par",Psapar);

    Prand=xml->getpar127("rand",Prand);
    Pamprandtype=xml->getpar127("amp_rand_type",Pamprandtype);
    Pamprandpower=xml->getpar127("amp_rand_power",Pamprandpower);

    Pharmonicshift=xml->getpar("harmonic_shift",Pharmonicshift,-64,64);
    Pharmonicshiftfirst=xml->getparbool("harmonic_shift_first",Pharmonicshiftfirst);

    Padaptiveharmonics=xml->getpar("adaptive_harmonics",Padaptiveharmonics,0,127);
    Padaptiveharmonicsbasefreq=xml->getpar("adaptive_harmonics_base_frequency",Padaptiveharmonicsbasefreq,0,255);
    Padaptiveharmonicspower=xml->getpar("adaptive_harmonics_power",Padaptiveharmonicspower,0,200);


    if (xml->enterbranch("HARMONICS")) {
        Phmag[0]=64;
        Phphase[0]=64;
        for (int n=0;n<MAX_AD_HARMONICS;n++) {
            if (xml->enterbranch("HARMONIC",n+1)==0) continue;
            Phmag[n]=xml->getpar127("mag",64);
            Phphase[n]=xml->getpar127("phase",64);
            xml->exitbranch();
        };
        xml->exitbranch();
    };

    if (Pcurrentbasefunc!=0) changebasefunction();


    if (xml->enterbranch("BASE_FUNCTION")) {
        for (int i=1;i<OSCIL_SIZE/2;i++) {
            if (xml->enterbranch("BF_HARMONIC",i)) {
                basefuncFFTfreqs.c[i]=xml->getparreal("cos",0.0);
                basefuncFFTfreqs.s[i]=xml->getparreal("sin",0.0);
                xml->exitbranch();
            };


        };
        xml->exitbranch();

        REALTYPE max=0.0;

        basefuncFFTfreqs.c[0]=0.0;
        for (int i=0;i<OSCIL_SIZE/2;i++) {
            if (max<fabs(basefuncFFTfreqs.c[i])) max=fabs(basefuncFFTfreqs.c[i]);
            if (max<fabs(basefuncFFTfreqs.s[i])) max=fabs(basefuncFFTfreqs.s[i]);
        };
        if (max<0.00000001) max=1.0;

        for (int i=0;i<OSCIL_SIZE/2;i++) {
            if (basefuncFFTfreqs.c[i]) basefuncFFTfreqs.c[i]/=max;
            if (basefuncFFTfreqs.s[i]) basefuncFFTfreqs.s[i]/=max;
        };
    };
};

