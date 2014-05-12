/*
  ZynAddSubFX - a software synthesizer

  Alienwah.h - "AlienWah" effect
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

#ifndef ALIENWAH_H
#define ALIENWAH_H

#include <complex>
#include "Effect.h"
#include "EffectLFO.h"

using namespace std;

#define MAX_ALIENWAH_DELAY 100

/**"AlienWah" Effect*/
class Alienwah:public Effect
{
    public:
        /**
         * Constructor
         * @param insertion_ true for insertion Effect
         * @param efxoutl_ Pointer to Alienwah's left channel output buffer
         * @param efxoutr_ Pointer to Alienwah's left channel output buffer
         * @return Initialized Alienwah
         */
        Alienwah(bool insertion_,
                 float *const efxoutl_,
                 float *const efxoutr_,
                 unsigned int srate, int bufsize);
        ~Alienwah();
        void out(const Stereo<float *> &smp);

        void setpreset(unsigned char npreset);
        void changepar(int npar, unsigned char value);
        unsigned char getpar(int npar) const;
        void cleanup(void);

    private:
        //Alienwah Parameters
        EffectLFO     lfo;      //lfo-ul Alienwah
        unsigned char Pvolume;
        unsigned char Pdepth;   //the depth of the Alienwah
        unsigned char Pfb;      //feedback
        unsigned char Pdelay;
        unsigned char Pphase;


        //Control Parameters
        void setvolume(unsigned char _Pvolume);
        void setdepth(unsigned char _Pdepth);
        void setfb(unsigned char _Pfb);
        void setdelay(unsigned char _Pdelay);
        void setphase(unsigned char _Pphase);

        //Internal Values
        float fb, depth, phase;
        complex<float> *oldl, *oldr;
        complex<float>  oldclfol, oldclfor;
        int oldk;
};

#endif
