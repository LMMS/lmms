/*
  ZynAddSubFX - a software synthesizer

  SV Filter.h - Several state-variable filters
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

#ifndef SV_FILTER_H
#define SV_FILTER_H

#include "../globals.h"
#include "Filter.h"

class SVFilter:public Filter
{
    public:
        SVFilter(unsigned char Ftype,
                 float Ffreq,
                 float Fq,
                 unsigned char Fstages,
                 unsigned int srate, int bufsize);
        ~SVFilter();
        void filterout(float *smp);
        void setfreq(float frequency);
        void setfreq_and_q(float frequency, float q_);
        void setq(float q_);

        void settype(int type_);
        void setgain(float dBgain);
        void setstages(int stages_);
        void cleanup();

    private:
        struct fstage {
            float low, high, band, notch;
        } st[MAX_FILTER_STAGES + 1];

        struct parameters {
            float f, q, q_sqrt;
        } par, ipar;

        void singlefilterout(float *smp, fstage &x, parameters &par);
        void computefiltercoefs(void);
        int   type;    // The type of the filter (LPF1,HPF1,LPF2,HPF2...)
        int   stages;  // how many times the filter is applied (0->1,1->2,etc.)
        float freq; // Frequency given in Hz
        float q;    // Q factor (resonance or Q factor)
        float gain; // the gain of the filter (if are shelf/peak) filters

        bool abovenq,   //if the frequency is above the nyquist
             oldabovenq;
        bool needsinterpolation, firsttime;
};

#endif
