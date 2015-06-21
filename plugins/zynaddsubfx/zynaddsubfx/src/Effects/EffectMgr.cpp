/*
  ZynAddSubFX - a software synthesizer

  EffectMgr.cpp - Effect manager, an interface betwen the program and effects
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

#include <rtosc/ports.h>
#include <rtosc/port-sugar.h>


#include "EffectMgr.h"
#include "Effect.h"
#include "Reverb.h"
#include "Echo.h"
#include "Chorus.h"
#include "Distorsion.h"
#include "EQ.h"
#include "DynamicFilter.h"
#include "../Misc/XMLwrapper.h"
#include "../Misc/Util.h"
#include "../Params/FilterParams.h"
#include "../Misc/Allocator.h"


#define rObject EffectMgr
rtosc::Ports EffectMgr::ports = {
    rSelf(EffectMgr),
    rPaste(),
    RECURP(EffectMgr, FilterParams, Filter, filterpars, "Filter Parameter for Dynamic Filter"),
    {"parameter#64::i", rProp(alias) rDoc("Parameter Accessor"), NULL,
        [](const char *msg, rtosc::RtData &d)
        {
            EffectMgr *eff = (EffectMgr*)d.obj;
            const char *mm = msg;
            while(!isdigit(*mm))++mm;

            if(!rtosc_narguments(msg))
                d.reply(d.loc, "i", eff->geteffectparrt(atoi(mm)));
            else 
                eff->seteffectparrt(atoi(mm), rtosc_argument(msg, 0).i);
        }},
    {"preset::i", rProp(alias) rDoc("Effect Preset Selector"), NULL,
        [](const char *msg, rtosc::RtData &d)
        {
            EffectMgr *eff = (EffectMgr*)d.obj;
            if(!rtosc_narguments(msg))
                d.reply(d.loc, "i", eff->getpreset());
            else
                eff->changepresetrt(rtosc_argument(msg, 0).i);
        }},
    {"eq-coeffs:", rProp(internal) rDoc("Get equalizer Coefficients"), NULL,
        [](const char *, rtosc::RtData &d)
        {
            EffectMgr *eff = (EffectMgr*)d.obj;
            if(eff->nefx != 7)
                return;
            EQ *eq = (EQ*)eff->efx;
            float a[MAX_EQ_BANDS*MAX_FILTER_STAGES*2+1];
            float b[MAX_EQ_BANDS*MAX_FILTER_STAGES*2+1];
            memset(a, 0, sizeof(a));
            memset(b, 0, sizeof(b));
            eq->getFilter(a,b);
            d.reply(d.loc, "bb", sizeof(a), a, sizeof(b), b);
        }},
    {"efftype::i", rDoc("Get Effect Type"), NULL, [](const char *m, rtosc::RtData &d)
        {
            EffectMgr *eff  = (EffectMgr*)d.obj;
            if(rtosc_narguments(m)) 
                eff->changeeffectrt(rtosc_argument(m,0).i);
            else
                d.reply(d.loc, "i", eff->nefx);
        }},
    {"efftype:b", rProp(internal) rDoc("Pointer swap EffectMgr"), NULL,
        [](const char *msg, rtosc::RtData &d)
        {
            printf("OBSOLETE METHOD CALLED\n");
            EffectMgr *eff  = (EffectMgr*)d.obj;
            EffectMgr *eff_ = *(EffectMgr**)rtosc_argument(msg,0).b.data;

            //Lets trade data
            std::swap(eff->nefx,eff_->nefx);
            std::swap(eff->efx,eff_->efx);
            std::swap(eff->filterpars,eff_->filterpars);
            std::swap(eff->efxoutl, eff_->efxoutl);
            std::swap(eff->efxoutr, eff_->efxoutr);

            //Return the old data for distruction
            d.reply("/free", "sb", "EffectMgr", sizeof(EffectMgr*), &eff_);
        }},

};


EffectMgr::EffectMgr(Allocator &alloc, const bool insertion_)
    :insertion(insertion_),
      efxoutl(new float[synth->buffersize]),
      efxoutr(new float[synth->buffersize]),
      filterpars(NULL),
      nefx(0),
      efx(NULL),
      dryonly(false),
      memory(alloc)
{
    setpresettype("Peffect");
    memset(efxoutl, 0, synth->bufferbytes);
    memset(efxoutr, 0, synth->bufferbytes);
    memset(settings, 0, sizeof(settings));
    defaults();
}


EffectMgr::~EffectMgr()
{
    memory.dealloc(efx);
    delete [] efxoutl;
    delete [] efxoutr;
}

void EffectMgr::defaults(void)
{
    changeeffect(0);
    setdryonly(false);
}

//Change the effect
void EffectMgr::changeeffectrt(int _nefx)
{
    cleanup();
    if(nefx == _nefx && efx != NULL)
        return;
    nefx = _nefx;
    memset(efxoutl, 0, synth->bufferbytes);
    memset(efxoutr, 0, synth->bufferbytes);
    memory.dealloc(efx);
    EffectParams pars(memory, insertion, efxoutl, efxoutr, 0,
            synth->samplerate, synth->buffersize);
    switch(nefx) {
        case 1:
            efx = memory.alloc<Reverb>(pars);
            break;
        case 2:
            efx = memory.alloc<Echo>(pars);
            break;
        case 3:
            efx = memory.alloc<Chorus>(pars);
            break;
        case 4:
            efx = memory.alloc<Phaser>(pars);
            break;
        case 5:
            efx = memory.alloc<Alienwah>(pars);
            break;
        case 6:
            efx = memory.alloc<Distorsion>(pars);
            break;
        case 7:
            efx = memory.alloc<EQ>(pars);
            break;
        case 8:
            efx = memory.alloc<DynamicFilter>(pars);
            break;
        //put more effect here
        default:
            efx = NULL;
            break; //no effect (thru)
    }

    if(efx)
        filterpars = efx->filterpars;
}

void EffectMgr::changeeffect(int _nefx)
{
    nefx = _nefx;
    //preset    = 0;
    //memset(settings, 0, sizeof(settings));
}

//Obtain the effect number
int EffectMgr::geteffect(void)
{
    return nefx;
}

// Initialize An Effect in RT context
void EffectMgr::init(void)
{
    changeeffectrt(nefx);
    changepresetrt(preset);
    for(int i=0; i<128; ++i)
        seteffectparrt(i, settings[i]);
}

//Strip effect manager of it's realtime memory
void EffectMgr::kill(void)
{
    //printf("Killing Effect(%d)\n", nefx);
    memory.dealloc(efx);
}

// Cleanup the current effect
void EffectMgr::cleanup(void)
{
    if(efx)
        efx->cleanup();
}


// Get the preset of the current effect
unsigned char EffectMgr::getpreset(void)
{
    if(efx)
        return efx->Ppreset;
    else
        return 0;
}

// Change the preset of the current effect
void EffectMgr::changepreset(unsigned char npreset)
{
    preset = npreset;
}

// Change the preset of the current effect
void EffectMgr::changepresetrt(unsigned char npreset)
{
    preset = npreset;
    if(efx)
        efx->setpreset(npreset);
}

//Change a parameter of the current effect
void EffectMgr::seteffectparrt(int npar, unsigned char value)
{
    if(npar<128)
        settings[npar] = value;
    if(!efx)
        return;
    efx->changepar(npar, value);
}

//Change a parameter of the current effect
void EffectMgr::seteffectpar(int npar, unsigned char value)
{
    settings[npar] = value;
}

//Get a parameter of the current effect
unsigned char EffectMgr::geteffectpar(int npar)
{
    if(npar<128)
        return settings[npar];

    if(!efx)
        return 0;
    return efx->getpar(npar);
}

unsigned char EffectMgr::geteffectparrt(int npar)
{
    if(!efx)
        return 0;
    return efx->getpar(npar);
}

// Apply the effect
void EffectMgr::out(float *smpsl, float *smpsr)
{
    if(!efx) {
        if(!insertion)
            for(int i = 0; i < synth->buffersize; ++i) {
                smpsl[i]   = 0.0f;
                smpsr[i]   = 0.0f;
                efxoutl[i] = 0.0f;
                efxoutr[i] = 0.0f;
            }
        return;
    }
    for(int i = 0; i < synth->buffersize; ++i) {
        smpsl[i]  += denormalkillbuf[i];
        smpsr[i]  += denormalkillbuf[i];
        efxoutl[i] = 0.0f;
        efxoutr[i] = 0.0f;
    }
    efx->out(smpsl, smpsr);

    float volume = efx->volume;

    if(nefx == 7) { //this is need only for the EQ effect
        memcpy(smpsl, efxoutl, synth->bufferbytes);
        memcpy(smpsr, efxoutr, synth->bufferbytes);
        return;
    }

    //Insertion effect
    if(insertion != 0) {
        float v1, v2;
        if(volume < 0.5f) {
            v1 = 1.0f;
            v2 = volume * 2.0f;
        }
        else {
            v1 = (1.0f - volume) * 2.0f;
            v2 = 1.0f;
        }
        if((nefx == 1) || (nefx == 2))
            v2 *= v2;  //for Reverb and Echo, the wet function is not liniar

        if(dryonly)   //this is used for instrument effect only
            for(int i = 0; i < synth->buffersize; ++i) {
                smpsl[i]   *= v1;
                smpsr[i]   *= v1;
                efxoutl[i] *= v2;
                efxoutr[i] *= v2;
            }
        else // normal instrument/insertion effect
            for(int i = 0; i < synth->buffersize; ++i) {
                smpsl[i] = smpsl[i] * v1 + efxoutl[i] * v2;
                smpsr[i] = smpsr[i] * v1 + efxoutr[i] * v2;
            }
    }
    else // System effect
        for(int i = 0; i < synth->buffersize; ++i) {
            efxoutl[i] *= 2.0f * volume;
            efxoutr[i] *= 2.0f * volume;
            smpsl[i]    = efxoutl[i];
            smpsr[i]    = efxoutr[i];
        }
}


// Get the effect volume for the system effect
float EffectMgr::sysefxgetvolume(void)
{
    return efx ? efx->outvolume : 1.0f;
}


// Get the EQ response
float EffectMgr::getEQfreqresponse(float freq)
{
    return (nefx == 7) ? efx->getfreqresponse(freq) : 0.0f;
}


void EffectMgr::setdryonly(bool value)
{
    dryonly = value;
}

void EffectMgr::paste(EffectMgr &e)
{
    changeeffectrt(e.nefx);
    changepresetrt(e.preset);
    for(int i=0;i<128;++i){
        seteffectparrt(e.settings[i], i);
    }
}

void EffectMgr::add2XML(XMLwrapper *xml)
{
    xml->addpar("type", geteffect());

    if(!geteffect())
        return;
    xml->addpar("preset", preset);

    xml->beginbranch("EFFECT_PARAMETERS");
    for(int n = 0; n < 128; ++n) {
        int par = geteffectpar(n);
        if(par == 0)
            continue;
        xml->beginbranch("par_no", n);
        xml->addpar("par", par);
        xml->endbranch();
    }
    if(filterpars) {
        xml->beginbranch("FILTER");
        filterpars->add2XML(xml);
        xml->endbranch();
    }
    xml->endbranch();
}

void EffectMgr::getfromXML(XMLwrapper *xml)
{
    changeeffect(xml->getpar127("type", geteffect()));

    if(!geteffect())
        return;

    preset = xml->getpar127("preset", preset);

    if(xml->enterbranch("EFFECT_PARAMETERS")) {
        for(int n = 0; n < 128; ++n) {
            seteffectpar(n, 0); //erase effect parameter
            if(xml->enterbranch("par_no", n) == 0)
                continue;
            int par = geteffectpar(n);
            seteffectpar(n, xml->getpar127("par", par));
            xml->exitbranch();
        }
        if(filterpars)
            if(xml->enterbranch("FILTER")) {
                filterpars->getfromXML(xml);
                xml->exitbranch();
            }
        xml->exitbranch();
    }
    cleanup();
}
