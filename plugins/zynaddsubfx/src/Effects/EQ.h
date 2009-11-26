/*
  ZynAddSubFX - a software synthesizer

  EQ.h - EQ Effect
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

#ifndef EQ_H
#define EQ_H

#include "../globals.h"
#include "../DSP/AnalogFilter.h"
#include "Effect.h"

/**EQ Effect*/
class EQ:public Effect
{
    public:
        EQ(const int &insertion_, REALTYPE *efxoutl_, REALTYPE *efxoutr_);
        ~EQ();
        void out(REALTYPE *smpsl, REALTYPE *smpr);
        void setpreset(unsigned char npreset);
        void changepar(const int &npar, const unsigned char &value);
        unsigned char getpar(const int &npar) const;
        void cleanup();
        REALTYPE getfreqresponse(REALTYPE freq);
    private:
        //Parameters
        unsigned char Pvolume; /**<Volume*/

        void setvolume(const unsigned char &Pvolume);

        struct {
            //parameters
            unsigned char Ptype, Pfreq, Pgain, Pq, Pstages;
            //internal values
            AnalogFilter *l, *r;
        } filter[MAX_EQ_BANDS];
};


#endif

