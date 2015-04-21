/*
  ZynAddSubFX - a software synthesizer

  EffectMgr.h - Effect manager, an interface betwen the program and effects
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

#ifndef EFFECTMGR_H
#define EFFECTMGR_H

#include <pthread.h>

#include "Alienwah.h"
#include "Phaser.h"
#include "../Params/Presets.h"

class Effect;
class FilterParams;
class XMLwrapper;
class Allocator;

#include "Distorsion.h"
#include "EQ.h"
#include "DynamicFilter.h"
#include "../Params/FilterParams.h"
#include "../Params/Presets.h"

using namespace Zyn;


/**Effect manager, an interface betwen the program and effects*/
class EffectMgr:public Presets
{
    public:
        EffectMgr(Allocator &alloc, const bool insertion_);
        ~EffectMgr();

        void paste(EffectMgr &e);
        void add2XML(XMLwrapper *xml);
        void defaults(void) REALTIME;
        void getfromXML(XMLwrapper *xml);

        void out(float *smpsl, float *smpsr) REALTIME;

        void setdryonly(bool value);

        /**get the output(to speakers) volume of the systemeffect*/
        float sysefxgetvolume(void);

        void init(void) REALTIME;
        void kill(void) REALTIME;
        void cleanup(void) REALTIME;

        void changeeffectrt(int nefx_) REALTIME;
        void changeeffect(int nefx_) NONREALTIME;
        int geteffect(void);
        void changepreset(unsigned char npreset) NONREALTIME;
        void changepresetrt(unsigned char npreset) REALTIME;
        unsigned char getpreset(void);
        void seteffectpar(int npar, unsigned char value) NONREALTIME;
        void seteffectparrt(int npar, unsigned char value) REALTIME;
        unsigned char geteffectpar(int npar);
        unsigned char geteffectparrt(int npar) REALTIME;

        const bool insertion;
        float     *efxoutl, *efxoutr;

        // used by UI
        float getEQfreqresponse(float freq);

        FilterParams *filterpars;

        static rtosc::Ports ports;
        int     nefx;
        Zyn::Effect *efx;
    private:

        //Parameters Prior to initialization
        char effect_id;
        char preset;
        char settings[128];

        bool dryonly;
        Allocator &memory;
};

#endif
