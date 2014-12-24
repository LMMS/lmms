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

#include "Effect.h"

/**EQ Effect*/
class EQ:public Effect
{
    public:
        EQ(bool insertion_, float *efxoutl_, float *efxoutr_, unsigned int srate, int bufsize);
        ~EQ() {}
        void out(const Stereo<float *> &smp);
        void setpreset(unsigned char npreset);
        void changepar(int npar, unsigned char value);
        unsigned char getpar(int npar) const;
        void cleanup(void);
        float getfreqresponse(float freq);

    private:
        //Parameters
        unsigned char Pvolume;

        void setvolume(unsigned char _Pvolume);

        struct {
            //parameters
            unsigned char Ptype, Pfreq, Pgain, Pq, Pstages;
            //internal values
            class AnalogFilter * l, *r;
        } filter[MAX_EQ_BANDS];
};

#endif
