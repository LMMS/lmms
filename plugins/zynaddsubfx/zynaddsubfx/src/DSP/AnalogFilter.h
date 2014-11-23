/*
  ZynAddSubFX - a software synthesizer

  Analog Filter.h - Several analog filters (lowpass, highpass...)
  Copyright (C) 2002-2005 Nasca Octavian Paul
  Copyright (C) 2010-2010 Mark McCurry
  Author: Nasca Octavian Paul
          Mark McCurry

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
#include "Filter.h"

/**Implementation of Several analog filters (lowpass, highpass...)
 * Implemented with IIR filters
 * Coefficients generated with "Cookbook formulae for audio EQ"*/
class AnalogFilter:public Filter
{
    public:
        AnalogFilter(unsigned char Ftype, float Ffreq, float Fq,
                     unsigned char Fstages, unsigned int srate, int bufsize);
        ~AnalogFilter();
        void filterout(float *smp);
        void setfreq(float frequency);
        void setfreq_and_q(float frequency, float q_);
        void setq(float q_);

        void settype(int type_);
        void setgain(float dBgain);
        void setstages(int stages_);
        void cleanup();

        float H(float freq); //Obtains the response for a given frequency

    private:
        struct fstage {
            float x1, x2; //Input History
            float y1, y2; //Output History
        } history[MAX_FILTER_STAGES + 1], oldHistory[MAX_FILTER_STAGES + 1];

        struct Coeff {
            float c[3], //Feed Forward
                  d[3];    //Feed Back
        } coeff, oldCoeff;
        //old coeffs are used for interpolation when paremeters change quickly

        //Apply IIR filter to Samples, with coefficients, and past history
        void singlefilterout(float *smp, fstage &hist, const Coeff &coeff);
        //Update coeff and order
        void computefiltercoefs(void);

        int   type;   //The type of the filter (LPF1,HPF1,LPF2,HPF2...)
        int   stages; //how many times the filter is applied (0->1,1->2,etc.)
        float freq;   //Frequency given in Hz
        float q;      //Q factor (resonance or Q factor)
        float gain;   //the gain of the filter (if are shelf/peak) filters

        int order; //the order of the filter (number of poles)

        bool needsinterpolation,      //Interpolation between coeff changes
             firsttime;               //First Iteration of filter
        bool abovenq,                 //if the frequency is above the nyquist
             oldabovenq;              //if the last time was above nyquist
                                      //(used to see if it needs interpolation)
};


#endif
