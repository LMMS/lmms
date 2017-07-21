/*
  ZynAddSubFX - a software synthesizer

  LFO.h - LFO implementation
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

#ifndef LFO_H
#define LFO_H

#include "../globals.h"
#include "../Params/LFOParams.h"

/**Class for creating Low Frequency Ocillators*/
class LFO
{
    public:
        /**Constructor
         *
         * @param lfopars pointer to a LFOParams object
         * @param basefreq base frequency of LFO
         */
        LFO(LFOParams *lfopars, float basefreq);
        /**Deconstructor*/
        ~LFO();
        float lfoout();
        float amplfoout();
    private:
        float x;
        float incx, incrnd, nextincrnd;
        float amp1, amp2; // used for randomness
        float lfointensity;
        float lfornd, lfofreqrnd;
        float lfodelay;
        /**\todo see if an enum would be better here*/
        char lfotype;
        int  freqrndenabled;


        void computenextincrnd();
};

#endif
