/*
  ZynAddSubFX - a software synthesizer

  Envelope.C - Envelope implementation
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

#include <stdio.h>
#include "Envelope.h"

Envelope::Envelope(EnvelopeParams *envpars,REALTYPE basefreq)
{
    int i;
    envpoints=envpars->Penvpoints;
    if (envpoints>MAX_ENVELOPE_POINTS) envpoints=MAX_ENVELOPE_POINTS;
    envsustain=(envpars->Penvsustain==0)?-1:envpars->Penvsustain;
    forcedrelase=envpars->Pforcedrelease;
    envstretch=pow(440.0/basefreq,envpars->Penvstretch/64.0);
    linearenvelope=envpars->Plinearenvelope;

    if (envpars->Pfreemode==0) envpars->converttofree();

    REALTYPE bufferdt=SOUND_BUFFER_SIZE/(REALTYPE)SAMPLE_RATE;

    int mode=envpars->Envmode;

    //for amplitude envelopes
    if ((mode==1)&&(linearenvelope==0)) mode=2;//change to log envelope
    if ((mode==2)&&(linearenvelope!=0)) mode=1;//change to linear

    for (i=0;i<MAX_ENVELOPE_POINTS;i++) {
        REALTYPE tmp=envpars->getdt(i)/1000.0*envstretch;
        if (tmp>bufferdt) envdt[i]=bufferdt/tmp;
        else envdt[i]=2.0;//any value larger than 1

        switch (mode) {
        case 2:
            envval[i]=(1.0-envpars->Penvval[i]/127.0)*MIN_ENVELOPE_DB;
            break;
        case 3:
            envval[i]=(pow(2,6.0*fabs(envpars->Penvval[i]-64.0)/64.0)-1.0)*100.0;
            if (envpars->Penvval[i]<64) envval[i]=-envval[i];
            break;
        case 4:
            envval[i]=(envpars->Penvval[i]-64.0)/64.0*6.0;//6 octaves (filtru)
            break;
        case 5:
            envval[i]=(envpars->Penvval[i]-64.0)/64.0*10;
            break;
        default:
            envval[i]=envpars->Penvval[i]/127.0;
        };

    };

    envdt[0]=1.0;

    currentpoint=1;//the envelope starts from 1
    keyreleased=0;
    t=0.0;
    envfinish=0;
    inct=envdt[1];
    envoutval=0.0;
};

Envelope::~Envelope()
{
};


/*
 * Relase the key (note envelope)
 */
void Envelope::relasekey()
{
    if (keyreleased==1) return;
    keyreleased=1;
    if (forcedrelase!=0) t=0.0;
};

/*
 * Envelope Output
 */
REALTYPE Envelope::envout()
{
    REALTYPE out;

    if (envfinish!=0) {//if the envelope is finished
        envoutval=envval[envpoints-1];
        return(envoutval);
    };
    if ((currentpoint==envsustain+1)&&(keyreleased==0)) {//if it is sustaining now
        envoutval=envval[envsustain];
        return(envoutval);
    };

    if ((keyreleased!=0) && (forcedrelase!=0)) {//do the forced release

        int tmp=(envsustain<0) ? (envpoints-1):(envsustain+1);//if there is no sustain point, use the last point for release

        if (envdt[tmp]<0.00000001) out=envval[tmp];
        else out=envoutval+(envval[tmp]-envoutval)*t;
        t+=envdt[tmp]*envstretch;

        if (t>=1.0) {
            currentpoint=envsustain+2;
            forcedrelase=0;
            t=0.0;
            inct=envdt[currentpoint];
            if ((currentpoint>=envpoints)||(envsustain<0)) envfinish=1;
        };
        return(out);
    };
    if (inct>=1.0) out=envval[currentpoint];
    else out=envval[currentpoint-1]+(envval[currentpoint]-envval[currentpoint-1])*t;

    t+=inct;
    if (t>=1.0) {
        if (currentpoint>=envpoints-1) envfinish=1;
        else currentpoint++;
        t=0.0;
        inct=envdt[currentpoint];
    };

    envoutval=out;
    return (out);
};

/*
 * Envelope Output (dB)
 */
REALTYPE Envelope::envout_dB()
{
    REALTYPE out;
    if (linearenvelope!=0) return (envout());

    if ((currentpoint==1)&&((keyreleased==0)||(forcedrelase==0))) {//first point is always lineary interpolated
        REALTYPE v1=dB2rap(envval[0]);
        REALTYPE v2=dB2rap(envval[1]);
        out=v1+(v2-v1)*t;

        t+=inct;
        if (t>=1.0) {
            t=0.0;
            inct=envdt[2];
            currentpoint++;
            out=v2;
        };

        if (out>0.001) envoutval=rap2dB(out);
        else envoutval=-40.0;
    } else out=dB2rap(envout());

    return(out);
};

int Envelope::finished()
{
    return(envfinish);
};

