/*
  ZynAddSubFX - a software synthesizer
 
  Echo.h - Echo Effect
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

#ifndef ECHO_H
#define ECHO_H

#include "../globals.h"
#include "Effect.h"

/**Echo Effect*/
class Echo:public Effect{
    public:

        /**
         * The Constructor For Echo
         * @param insertion_ integer to determine if Echo is an insertion effect
         * or not
         * @param efxoutl_ Effect out Left Channel
         * @param efxoutr_ Effect out Right Channel
         * @return An initialized Echo Object
         */
        Echo(const int & insertion_,REALTYPE *const efxoutl_,REALTYPE *const efxoutr_);

        /**
         * The destructor
         */
        ~Echo();

        /**
         * Outputs the echo to efxoutl and efxoutr
         * @param smpsl Sample from Left channel
         * @param smpsr Sample from Right channel
         */
        void out(REALTYPE *const smpsl,REALTYPE *const smpr);
        
        /**
         * Sets the state of Echo to the specified preset
         * @param npreset number of chosen preset
         */
        void setpreset(unsigned char npreset);

        /**
         * Sets the value of the chosen variable
         *
         * The possible parameters are:
         *   -# Volume
         *   -# Panning
         *   -# Delay
         *   -# L/R Delay
         *   -# L/R Crossover
         *   -# Feedback
         *   -# Dampening
         * @param npar number of chosen parameter
         * @param value the new value
         */
        void changepar(const int & npar,const unsigned char & value);

        /**
         * Gets the specified parameter
         *
         * The possible parameters are
         *   -# Volume
         *   -# Panning
         *   -# Delay
         *   -# L/R Delay
         *   -# L/R Crossover
         *   -# Feedback
         *   -# Dampening
         * @param npar number of chosen parameter
         * @return value of parameter
         * 
         * \todo make this method use constant variables by reference
         *Currently doing so results in strange behavior
         */
        unsigned char getpar(const int & npar)const;

        /**Zeros out the state of the Echo*/
        void cleanup();
        
        /**\todo This function needs to be implemented or the  prototype should be removed*/
        void setdryonly();
    private:
        //Parameters
        unsigned char Pvolume;/**< Volume or Dry/Wetness*/
        unsigned char Ppanning;/**< Panning*/
        unsigned char Pdelay;/**< Delay of the Echo*/
        unsigned char Plrdelay;/**< L/R delay difference*/
        unsigned char Plrcross;/**< L/R Mixing*/
        unsigned char Pfb;/**<Feedback*/
        unsigned char Phidamp;/**<Dampening of the Echo*/
        
        void setvolume(const unsigned char & Pvolume);
        void setpanning(const unsigned char & Ppanning);
        void setdelay(const unsigned char & Pdelay);
        void setlrdelay(const unsigned char & Plrdelay);
        void setlrcross(const unsigned char & Plrcross);
        void setfb(const unsigned char & Pfb);
        void sethidamp(const unsigned char & Phidamp);

        //Real Parameters
        REALTYPE panning,lrcross,fb,hidamp;
        int dl,dr,delay,lrdelay;
        
        void initdelays();
        REALTYPE *ldelay,*rdelay;
        REALTYPE oldl,oldr;//pt. lpf
        int kl,kr;
};

#endif

