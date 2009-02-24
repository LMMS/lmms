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


class Effect{
      public:

      virtual ~Effect(){};
      virtual void setpreset(unsigned char npreset){};
      virtual void changepar(int npar,unsigned char value){};
      virtual unsigned char getpar(int npar){return(0);};
      virtual void out(REALTYPE *smpsl,REALTYPE *smpsr){};	
      virtual void cleanup(){};
      virtual REALTYPE getfreqresponse(REALTYPE freq){return (0);};//this is only used for EQ (for user interface)

      unsigned char Ppreset;
      REALTYPE *efxoutl;
      REALTYPE *efxoutr;

      REALTYPE outvolume;//this is the volume of effect and is public because need it in system effect. The out volume of such effects are always 1.0, so this setting tells me how is the volume to the Master Output only.

      REALTYPE volume;

      FilterParams *filterpars;
      protected:

      int insertion;//1 for insertion effect
};

#endif




