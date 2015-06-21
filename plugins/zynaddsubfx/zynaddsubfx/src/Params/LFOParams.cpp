/*
  ZynAddSubFX - a software synthesizer

  LFOParams.cpp - Parameters for LFO
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
#include <cstdio>
#include "../globals.h"
#include "../Misc/Util.h"
#include "../Misc/XMLwrapper.h"
#include "LFOParams.h"

#include <rtosc/port-sugar.h>
#include <rtosc/ports.h>
using namespace rtosc;


#define rObject LFOParams
static rtosc::Ports _ports = {
    rSelf(LFOParams),
    rPaste(),
    rParamF(Pfreq, "frequency of LFO"),
    rParamZyn(Pintensity, "Intensity of LFO"),
    rParamZyn(Pstartphase, rSpecial(random), "Starting Phase"),
    rOption(PLFOtype,"Shape of LFO"),
    rParamZyn(Prandomness, rSpecial(disable), "Amplitude Randomness"),
    rParamZyn(Pfreqrand, rSpecial(disable), "Frequency Randomness"),
    rParamZyn(Pdelay, rSpecial(disable), "Delay before LFO start"),
    rToggle(Pcontinous, "Enable for global operation"),
    rParamZyn(Pstretch, rCentered, "Note frequency stretch"),
};

rtosc::Ports &LFOParams::ports = _ports;

int LFOParams::time;

//LFOParams::LFOParams()
//{
//    Dfreq       = 64;
//    Dintensity  = 0;
//    Dstartphase = 0;
//    DLFOtype    = 0;
//    Drandomness = 0;
//    Ddelay      = 0;
//    Dcontinous  = 0;
//    fel  = 0;
//    time = 0;

//    defaults();
//}

LFOParams::LFOParams(char Pfreq_,
                     char Pintensity_,
                     char Pstartphase_,
                     char PLFOtype_,
                     char Prandomness_,
                     char Pdelay_,
                     char Pcontinous_,
                     char fel_)
{
    //switch(fel_) {
    //    case 0:
    //        setpresettype("Plfofrequency");
    //        break;
    //    case 1:
    //        setpresettype("Plfoamplitude");
    //        break;
    //    case 2:
    //        setpresettype("Plfofilter");
    //        break;
    //}
    Dfreq       = Pfreq_;
    Dintensity  = Pintensity_;
    Dstartphase = Pstartphase_;
    DLFOtype    = PLFOtype_;
    Drandomness = Prandomness_;
    Ddelay      = Pdelay_;
    Dcontinous  = Pcontinous_;
    fel  = fel_;
    time = 0;

    defaults();
}

LFOParams::~LFOParams()
{}

void LFOParams::defaults()
{
    Pfreq       = Dfreq / 127.0f;
    Pintensity  = Dintensity;
    Pstartphase = Dstartphase;
    PLFOtype    = DLFOtype;
    Prandomness = Drandomness;
    Pdelay      = Ddelay;
    Pcontinous  = Dcontinous;
    Pfreqrand   = 0;
    Pstretch    = 64;
}


void LFOParams::add2XML(XMLwrapper *xml)
{
    xml->addparreal("freq", Pfreq);
    xml->addpar("intensity", Pintensity);
    xml->addpar("start_phase", Pstartphase);
    xml->addpar("lfo_type", PLFOtype);
    xml->addpar("randomness_amplitude", Prandomness);
    xml->addpar("randomness_frequency", Pfreqrand);
    xml->addpar("delay", Pdelay);
    xml->addpar("stretch", Pstretch);
    xml->addparbool("continous", Pcontinous);
}

void LFOParams::getfromXML(XMLwrapper *xml)
{
    Pfreq       = xml->getparreal("freq", Pfreq, 0.0f, 1.0f);
    Pintensity  = xml->getpar127("intensity", Pintensity);
    Pstartphase = xml->getpar127("start_phase", Pstartphase);
    PLFOtype    = xml->getpar127("lfo_type", PLFOtype);
    Prandomness = xml->getpar127("randomness_amplitude", Prandomness);
    Pfreqrand   = xml->getpar127("randomness_frequency", Pfreqrand);
    Pdelay      = xml->getpar127("delay", Pdelay);
    Pstretch    = xml->getpar127("stretch", Pstretch);
    Pcontinous  = xml->getparbool("continous", Pcontinous);
}

void LFOParams::paste(LFOParams &x)
{
    //Avoid undefined behavior
    if(&x == this)
        return;
    memcpy((char*)this, (const char*)&x, sizeof(*this));
}
