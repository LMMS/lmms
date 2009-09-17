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
#include "Filter_.h"
class SVFilter:public Filter_
{
public:
    SVFilter(unsigned char Ftype,REALTYPE Ffreq, REALTYPE Fq,unsigned char Fstages);
    ~SVFilter();
    void filterout(REALTYPE *smp);
    void setfreq(REALTYPE frequency);
    void setfreq_and_q(REALTYPE frequency,REALTYPE q_);
    void setq(REALTYPE q_);

    void settype(int type_);
    void setgain(REALTYPE dBgain);
    void setstages(int stages_);
    void cleanup();

private:
    struct fstage {
        REALTYPE low,high,band,notch;
    } st[MAX_FILTER_STAGES+1];

    struct parameters {
        REALTYPE f,q,q_sqrt;
    }par,ipar;


    void singlefilterout(REALTYPE *smp,fstage &x,parameters &par);
    void computefiltercoefs();
    int type;//The type of the filter (LPF1,HPF1,LPF2,HPF2...)
    int stages;//how many times the filter is applied (0->1,1->2,etc.)
    REALTYPE freq;//Frequency given in Hz
    REALTYPE q; //Q factor (resonance or Q factor)
    REALTYPE gain;//the gain of the filter (if are shelf/peak) filters

    int abovenq;//this is 1 if the frequency is above the nyquist
    int oldabovenq;
    int needsinterpolation,firsttime;
};


#endif

