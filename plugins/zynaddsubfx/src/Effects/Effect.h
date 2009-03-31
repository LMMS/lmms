/*
  ZynAddSubFX - a software synthesizer
 
  Effect.h - this class is inherited by the all effects(Reverb, Echo, ..)
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

#ifndef EFFECT_H
#define EFFECT_H

#include <pthread.h>
#include "../Misc/Util.h"
#include "../globals.h"
#include "../Params/FilterParams.h"


/**this class is inherited by the all effects(Reverb, Echo, ..)*/
class Effect{
      public:
      /**
       * Effect Constructor
       * @param insertion_ 1 when it is an insertion Effect and 0 when it
       * is not an insertion Effect 
       * @param efxoutl_ Effect output buffer Left channel
       * @param efxoutr_ Effect output buffer Right channel
       * @param filterpars_ pointer to FilterParams array
       * @param Ppreset_ chosen preset
       * @return Initialized Effect object*/
      Effect(const int & insertion_,REALTYPE *const efxoutl_,
          REALTYPE *const efxoutr_,FilterParams *filterpars_,
          const unsigned char & Ppreset_);
      /**Deconstructor*/
      virtual ~Effect(){};
      /**
       * Choose a preset
       * @param npreset number of chosen preset*/
      virtual void setpreset(unsigned char npreset){};
      /**Change parameter npar to value
       * @param npar chosen parameter
       * @param value chosen new value*/
      virtual void changepar(const int &npar,const unsigned char &value){};
      /**Get the value of parameter npar
       * @param npar chosen parameter
       * @return the value of the parameter in an unsigned char or 0 if it
       * does not exist*/
      virtual unsigned char getpar(const int &npar)const {return(0);};
      virtual void out(REALTYPE *const smpsl,REALTYPE *const smpsr){};	
      /**Reset the state of the effect*/
      virtual void cleanup(){};
      /**This is only used for EQ (for user interface)*/
      virtual REALTYPE getfreqresponse(REALTYPE freq){return (0);};

      unsigned char Ppreset;
      REALTYPE *const efxoutl;/**<Effect out Left Channel*/
      REALTYPE *const efxoutr;/**<Effect out Right Channel*/

      REALTYPE outvolume;/**<This is the volume of effect and is public because
                          * it is needed in system effects.
                          * The out volume of such effects are always 1.0, so
                          * this setting tells me how is the volume to the
                          * Master Output only.*/

      REALTYPE volume;

      FilterParams *filterpars;/**<Parameters for filters used by Effect*/
      protected:

      const int insertion;/**<If Effect is an insertion effect, insertion=1
                           *otherwise, it should be insertion=0*/
};

#endif




