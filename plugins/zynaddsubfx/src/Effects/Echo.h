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
#include "../Misc/Stereo.h"
#include "../Samples/Sample.h"

/**Echo Effect*/
class Echo:public Effect
{
    public:

        /**
         * The Constructor For Echo
         * @param insertion_ integer to determine if Echo is an insertion effect
         * or not
         * @param efxoutl_ Effect out Left Channel
         * @param efxoutr_ Effect out Right Channel
         * @return An initialized Echo Object
         */
        Echo(const int &insertion_,
             REALTYPE *const efxoutl_,
             REALTYPE *const efxoutr_);

        /**
         * The destructor
         */
        ~Echo();

        void out(const Stereo<float *> &input);

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
        void changepar(int npar, unsigned char value);

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
         */
        unsigned char getpar(int npar) const;

        int getnumparams();

        /**Zeros out the state of the Echo*/
        void cleanup();

        /**\todo This function needs to be implemented or the  prototype should be removed*/
        void setdryonly();
    private:
        //Parameters
        char     Pvolume;  /**<#1 Volume or Dry/Wetness*/
        char     Ppanning; /**<#2 Panning*/
        char     Pdelay;   /**<#3 Delay of the Echo*/
        char     Plrdelay; /**<#4 L/R delay difference*/
        char     Plrcross; /**<#5 L/R Mixing*/
        char     Pfb;      /**<#6Feedback*/
        char     Phidamp;  /**<#7Dampening of the Echo*/

        void setvolume(unsigned char Pvolume);
        void setpanning(unsigned char Ppanning);
        void setdelay(unsigned char Pdelay);
        void setlrdelay(unsigned char Plrdelay);
        void setlrcross(unsigned char Plrcross);
        void setfb(unsigned char Pfb);
        void sethidamp(unsigned char Phidamp);

        //Real Parameters
        REALTYPE panning, lrcross, fb, hidamp;
        //Left/Right delay lengths
        Stereo<int> delayTime;
        REALTYPE lrdelay;
        REALTYPE avgDelay;

        void initdelays();
        //2 channel ring buffer
        Stereo<REALTYPE *> delay;
        Stereo<REALTYPE> old;

        //position of reading/writing from delaysample
        Stereo<int> pos;
        //step size for delay buffer
        Stereo<int> delta;
        Stereo<int> ndelta;
};

#endif

