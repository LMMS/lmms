/*
  ZynAddSubFX - a software synthesizer

  Analog Filter.h - Several analog filters (lowpass, highpass...)
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

#ifndef ANALOG_FILTER_H
#define ANALOG_FILTER_H

#include "../globals.h"
#include "Filter_.h"

/**Implementation of Several analog filters (lowpass, highpass...)*/
class AnalogFilter:public Filter_
{
    public:
        AnalogFilter(unsigned char Ftype,
                     REALTYPE Ffreq,
                     REALTYPE Fq,
                     unsigned char Fstages);
        ~AnalogFilter();
        void filterout(REALTYPE *smp);
        void setfreq(REALTYPE frequency);
        void setfreq_and_q(REALTYPE frequency, REALTYPE q_);
        void setq(REALTYPE q_);

        void settype(int type_);
        void setgain(REALTYPE dBgain);
        void setstages(int stages_);
        void cleanup();

        REALTYPE H(REALTYPE freq); //Obtains the response for a given frequency

    private:
        struct fstage {
            REALTYPE c1, c2;
        } x[MAX_FILTER_STAGES + 1], y[MAX_FILTER_STAGES + 1],
          oldx[MAX_FILTER_STAGES + 1], oldy[MAX_FILTER_STAGES + 1];

        void singlefilterout(REALTYPE *smp,
                             fstage &x,
                             fstage &y,
                             REALTYPE *c,
                             REALTYPE *d);
        void computefiltercoefs();
        int      type; //The type of the filter (LPF1,HPF1,LPF2,HPF2...)
        int      stages; //how many times the filter is applied (0->1,1->2,etc.)
        REALTYPE freq; //Frequency given in Hz
        REALTYPE q; //Q factor (resonance or Q factor)
        REALTYPE gain; //the gain of the filter (if are shelf/peak) filters

        int order; //the order of the filter (number of poles)

        REALTYPE c[3], d[3]; //coefficients

        REALTYPE oldc[3], oldd[3]; //old coefficients(used only if some filter paremeters changes very fast, and it needs interpolation)

        REALTYPE xd[3], yd[3]; //used if the filter is applied more times
        int      needsinterpolation, firsttime; /**\todo see if bool works for these*/
        int      abovenq; //this is 1 if the frequency is above the nyquist
        int      oldabovenq; //if the last time was above nyquist (used to see if it needs interpolation)
};


#endif

