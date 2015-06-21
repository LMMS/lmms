/*
  ZynAddSubFX - a software synthesizer

  EnvelopeParams.cpp - Parameters for Envelope
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
#include <cstdlib>
#include <rtosc/ports.h>
#include <rtosc/port-sugar.h>

#include "EnvelopeParams.h"
#include "../Misc/Util.h"

#define rObject EnvelopeParams
using namespace rtosc;

static rtosc::Ports localPorts = {
    rSelf(EnvelopeParams),
    rPaste(),
    rToggle(Pfreemode, "Complex Envelope Definitions"),
    rParamZyn(Penvpoints, rProp(internal), "Number of points in complex definition"),
    rParamZyn(Penvsustain, rProp(internal), "Location of the sustain point"),
    rParams(Penvdt,  MAX_ENVELOPE_POINTS, "Envelope Delay Times"),
    rParams(Penvval, MAX_ENVELOPE_POINTS, "Envelope Values"),
    rParamZyn(Penvstretch, "Stretch with respect to frequency"),
    rToggle(Pforcedrelease, "Force Envelope to fully evaluate"),
    rToggle(Plinearenvelope, "Linear or Logarithmic Envelopes"),
    rParamZyn(PA_dt,  "Attack Time"),
    rParamZyn(PA_val, "Attack Value"),
    rParamZyn(PD_dt,  "Decay Time"),
    rParamZyn(PD_val, "Decay Value"),
    rParamZyn(PS_val, "Sustain Value"),
    rParamZyn(PR_dt,  "Release Time"),
    rParamZyn(PR_val, "Release Value"),

    {"addPoint:i", rProp(internal) rDoc("Add point to envelope"), NULL, [](const char *msg, RtData &d)
        {
            EnvelopeParams *env = (rObject*) d.obj;
            const int curpoint = rtosc_argument(msg, 0).i;
            //int curpoint=freeedit->lastpoint;
            if (curpoint<0 || curpoint>env->Penvpoints || env->Penvpoints>=MAX_ENVELOPE_POINTS)
                return;

            for (int i=env->Penvpoints; i>=curpoint+1; i--) {
                env->Penvdt[i]=env->Penvdt[i-1];
                env->Penvval[i]=env->Penvval[i-1];
            }

            if (curpoint==0) {
                env->Penvdt[1]=64;
            }

            env->Penvpoints++;
            if (curpoint<=env->Penvsustain) env->Penvsustain++;
        }},
    {"delPoint:i", rProp(internal) rDoc("Delete Envelope Point"), NULL, [](const char *msg, RtData &d)
        {
            EnvelopeParams *env = (rObject*) d.obj;
            const int curpoint=rtosc_argument(msg, 0).i;
            if(curpoint<1 || curpoint>=env->Penvpoints-1 || env->Penvpoints<=3)
                return;

            for (int i=curpoint+1;i<env->Penvpoints;i++){
                env->Penvdt[i-1]=env->Penvdt[i];
                env->Penvval[i-1]=env->Penvval[i];
            };

            env->Penvpoints--;

            if (curpoint<=env->Penvsustain)
                env->Penvsustain--;

        }},
};

rtosc::Ports &EnvelopeParams::ports = localPorts;

EnvelopeParams::EnvelopeParams(unsigned char Penvstretch_,
        unsigned char Pforcedrelease_)
{
    PA_dt  = 10;
    PD_dt  = 10;
    PR_dt  = 10;
    PA_val = 64;
    PD_val = 64;
    PS_val = 64;
    PR_val = 64;

    for(int i = 0; i < MAX_ENVELOPE_POINTS; ++i) {
        Penvdt[i]  = 32;
        Penvval[i] = 64;
    }
    Penvdt[0]       = 0; //no used
    Penvsustain     = 1;
    Penvpoints      = 1;
    Envmode         = 1;
    Penvstretch     = Penvstretch_;
    Pforcedrelease  = Pforcedrelease_;
    Pfreemode       = 1;
    Plinearenvelope = 0;

    store2defaults();
}

EnvelopeParams::~EnvelopeParams()
{}

void EnvelopeParams::paste(const EnvelopeParams &ep)
{
    //Avoid undefined behavior
    if(&ep == this)
        return;
    memcpy((char*)this, (const char*)&ep, sizeof(*this));
}

float EnvelopeParams::getdt(char i) const
{
    return EnvelopeParams::dt(Penvdt[(int)i]);
}

float EnvelopeParams::dt(char val)
{
    return (powf(2.0f, val / 127.0f * 12.0f) - 1.0f) * 10.0f; //miliseconds
}


/*
 * ADSR/ASR... initialisations
 */
void EnvelopeParams::ADSRinit(char A_dt, char D_dt, char S_val, char R_dt)
{
    //setpresettype("Penvamplitude");
    Envmode   = 1;
    PA_dt     = A_dt;
    PD_dt     = D_dt;
    PS_val    = S_val;
    PR_dt     = R_dt;
    Pfreemode = 0;
    converttofree();

    store2defaults();
}

void EnvelopeParams::ADSRinit_dB(char A_dt, char D_dt, char S_val, char R_dt)
{
    //setpresettype("Penvamplitude");
    Envmode   = 2;
    PA_dt     = A_dt;
    PD_dt     = D_dt;
    PS_val    = S_val;
    PR_dt     = R_dt;
    Pfreemode = 0;
    converttofree();

    store2defaults();
}

void EnvelopeParams::ASRinit(char A_val, char A_dt, char R_val, char R_dt)
{
    //setpresettype("Penvfrequency");
    Envmode   = 3;
    PA_val    = A_val;
    PA_dt     = A_dt;
    PR_val    = R_val;
    PR_dt     = R_dt;
    Pfreemode = 0;
    converttofree();

    store2defaults();
}

void EnvelopeParams::ADSRinit_filter(char A_val,
                                     char A_dt,
                                     char D_val,
                                     char D_dt,
                                     char R_dt,
                                     char R_val)
{
    //setpresettype("Penvfilter");
    Envmode   = 4;
    PA_val    = A_val;
    PA_dt     = A_dt;
    PD_val    = D_val;
    PD_dt     = D_dt;
    PR_dt     = R_dt;
    PR_val    = R_val;
    Pfreemode = 0;
    converttofree();
    store2defaults();
}

void EnvelopeParams::ASRinit_bw(char A_val, char A_dt, char R_val, char R_dt)
{
    //setpresettype("Penvbandwidth");
    Envmode   = 5;
    PA_val    = A_val;
    PA_dt     = A_dt;
    PR_val    = R_val;
    PR_dt     = R_dt;
    Pfreemode = 0;
    converttofree();
    store2defaults();
}

/*
 * Convert the Envelope to freemode
 */
void EnvelopeParams::converttofree()
{
    switch(Envmode) {
        case 1:
            Penvpoints  = 4;
            Penvsustain = 2;
            Penvval[0]  = 0;
            Penvdt[1]   = PA_dt;
            Penvval[1]  = 127;
            Penvdt[2]   = PD_dt;
            Penvval[2]  = PS_val;
            Penvdt[3]   = PR_dt;
            Penvval[3]  = 0;
            break;
        case 2:
            Penvpoints  = 4;
            Penvsustain = 2;
            Penvval[0]  = 0;
            Penvdt[1]   = PA_dt;
            Penvval[1]  = 127;
            Penvdt[2]   = PD_dt;
            Penvval[2]  = PS_val;
            Penvdt[3]   = PR_dt;
            Penvval[3]  = 0;
            break;
        case 3:
            Penvpoints  = 3;
            Penvsustain = 1;
            Penvval[0]  = PA_val;
            Penvdt[1]   = PA_dt;
            Penvval[1]  = 64;
            Penvdt[2]   = PR_dt;
            Penvval[2]  = PR_val;
            break;
        case 4:
            Penvpoints  = 4;
            Penvsustain = 2;
            Penvval[0]  = PA_val;
            Penvdt[1]   = PA_dt;
            Penvval[1]  = PD_val;
            Penvdt[2]   = PD_dt;
            Penvval[2]  = 64;
            Penvdt[3]   = PR_dt;
            Penvval[3]  = PR_val;
            break;
        case 5:
            Penvpoints  = 3;
            Penvsustain = 1;
            Penvval[0]  = PA_val;
            Penvdt[1]   = PA_dt;
            Penvval[1]  = 64;
            Penvdt[2]   = PR_dt;
            Penvval[2]  = PR_val;
            break;
    }
}




void EnvelopeParams::add2XML(XMLwrapper *xml)
{
    xml->addparbool("free_mode", Pfreemode);
    xml->addpar("env_points", Penvpoints);
    xml->addpar("env_sustain", Penvsustain);
    xml->addpar("env_stretch", Penvstretch);
    xml->addparbool("forced_release", Pforcedrelease);
    xml->addparbool("linear_envelope", Plinearenvelope);
    xml->addpar("A_dt", PA_dt);
    xml->addpar("D_dt", PD_dt);
    xml->addpar("R_dt", PR_dt);
    xml->addpar("A_val", PA_val);
    xml->addpar("D_val", PD_val);
    xml->addpar("S_val", PS_val);
    xml->addpar("R_val", PR_val);

    if((Pfreemode != 0) || (!xml->minimal))
        for(int i = 0; i < Penvpoints; ++i) {
            xml->beginbranch("POINT", i);
            if(i != 0)
                xml->addpar("dt", Penvdt[i]);
            xml->addpar("val", Penvval[i]);
            xml->endbranch();
        }
}



void EnvelopeParams::getfromXML(XMLwrapper *xml)
{
    Pfreemode       = xml->getparbool("free_mode", Pfreemode);
    Penvpoints      = xml->getpar127("env_points", Penvpoints);
    Penvsustain     = xml->getpar127("env_sustain", Penvsustain);
    Penvstretch     = xml->getpar127("env_stretch", Penvstretch);
    Pforcedrelease  = xml->getparbool("forced_release", Pforcedrelease);
    Plinearenvelope = xml->getparbool("linear_envelope", Plinearenvelope);

    PA_dt  = xml->getpar127("A_dt", PA_dt);
    PD_dt  = xml->getpar127("D_dt", PD_dt);
    PR_dt  = xml->getpar127("R_dt", PR_dt);
    PA_val = xml->getpar127("A_val", PA_val);
    PD_val = xml->getpar127("D_val", PD_val);
    PS_val = xml->getpar127("S_val", PS_val);
    PR_val = xml->getpar127("R_val", PR_val);

    for(int i = 0; i < Penvpoints; ++i) {
        if(xml->enterbranch("POINT", i) == 0)
            continue;
        if(i != 0)
            Penvdt[i] = xml->getpar127("dt", Penvdt[i]);
        Penvval[i] = xml->getpar127("val", Penvval[i]);
        xml->exitbranch();
    }

    if(!Pfreemode)
        converttofree();
}


void EnvelopeParams::defaults()
{
    Penvstretch     = Denvstretch;
    Pforcedrelease  = Dforcedrelease;
    Plinearenvelope = Dlinearenvelope;
    PA_dt     = DA_dt;
    PD_dt     = DD_dt;
    PR_dt     = DR_dt;
    PA_val    = DA_val;
    PD_val    = DD_val;
    PS_val    = DS_val;
    PR_val    = DR_val;
    Pfreemode = 0;
    converttofree();
}

void EnvelopeParams::store2defaults()
{
    Denvstretch     = Penvstretch;
    Dforcedrelease  = Pforcedrelease;
    Dlinearenvelope = Plinearenvelope;
    DA_dt  = PA_dt;
    DD_dt  = PD_dt;
    DR_dt  = PR_dt;
    DA_val = PA_val;
    DD_val = PD_val;
    DS_val = PS_val;
    DR_val = PR_val;
}
