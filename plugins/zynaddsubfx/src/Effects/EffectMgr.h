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

#include "Effect.h"
#include "Reverb.h"
#include "Echo.h"
#include "Chorus.h"
#include "Phaser.h"
#include "Alienwah.h"
#include "Distorsion.h"
#include "EQ.h"
#include "DynamicFilter.h"
#include "../Misc/XMLwrapper.h"
#include "../Params/FilterParams.h"
#include "../Params/Presets.h"


/**Effect manager, an interface betwen the program and effects*/
class EffectMgr:public Presets
{
    public:
        EffectMgr(int insertion_, pthread_mutex_t *mutex_);
        ~EffectMgr();

        void add2XML(XMLwrapper *xml);
        void defaults();
        void getfromXML(XMLwrapper *xml);

        void out(REALTYPE *smpsl, REALTYPE *smpsr);

        void setdryonly(bool value);

        /**get the output(to speakers) volume of the systemeffect*/
        REALTYPE sysefxgetvolume();

        void cleanup(); /**<cleanup the effect*/

        /**change effect to the given int
             * @param nefx_ the number of the effect*/
        void changeeffect(int nefx_);
        /**Get the number of the effect
         * @return the number*/
        int geteffect();
        /**
         * Change the preset to the given one
         * @param npreset number of the chosen preset
         */
        void changepreset(unsigned char npreset);
        /**
         * Change the preset to the given one without locking the thread
         * @param npreset number of the chosen preset
         */
        void changepreset_nolock(unsigned char npreset);
        /**
         * Get the current preset
         * @return the current preset*/
        unsigned char getpreset();
        /**sets the effect par*/
        void seteffectpar(int npar, unsigned char value);
        /**<sets the effect par without thread lock*/
        void seteffectpar_nolock(int npar, unsigned char value);
        unsigned char geteffectpar(int npar);
        const bool insertion; /**<1 if the effect is connected as insertion effect*/
        REALTYPE  *efxoutl, *efxoutr;

        /**used by UI
             * \todo needs to be decoupled*/
        REALTYPE getEQfreqresponse(REALTYPE freq);

        FilterParams *filterpars;

    private:
        int     nefx;
        Effect *efx;
        pthread_mutex_t *mutex;
        bool dryonly;
};

#endif

