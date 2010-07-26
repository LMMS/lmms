/*
  ZynAddSubFX - a software synthesizer

  Phaser.h - Phaser effect
  Copyright (C) 2002-2005 Nasca Octavian Paul
  Copyright (C) 2009-2010 Ryan Billing
  Copyright (C) 2010-2010 Mark McCurry
  Author: Nasca Octavian Paul
          Ryan Billing
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

#ifndef PHASER_H
#define PHASER_H
#include "../globals.h"
#include "Effect.h"
#include "EffectLFO.h"

#define MAX_PHASER_STAGES 12

class Phaser:public Effect
{
    public:
        Phaser(const int &insertion_, REALTYPE *efxoutl_, REALTYPE *efxoutr_);
        ~Phaser();
        void out(const Stereo<REALTYPE *> &input);
        void setpreset(unsigned char npreset);
        void changepar(int npar, unsigned char value);
        unsigned char getpar(int npar) const;
        void cleanup();

    private:
        //Phaser parameters
        EffectLFO lfo;              //Phaser modulator
        unsigned char Pvolume;      //Used to set wet/dry mix
        unsigned char Ppanning;
        unsigned char Pdistortion;  //Model distortion added by FET element
        unsigned char Pdepth;       //Depth of phaser sweep
        unsigned char Pwidth;       //Phaser width (LFO amplitude)
        unsigned char Pfb;          //feedback
        unsigned char Poffset;      //Model mismatch between variable resistors
        unsigned char Plrcross;     //crossover
        unsigned char Pstages;      //Number of first-order All-Pass stages
        unsigned char Poutsub;      //if I wish to subtract the output instead of adding
        unsigned char Pphase;
        unsigned char Phyper;       //lfo^2 -- converts tri into hyper-sine
        unsigned char Pbarber;      //Enable parber pole phasing
        unsigned char Panalog;

        //Control parameters
        void setvolume(unsigned char Pvolume);
        void setpanning(unsigned char Ppanning);
        void setdepth(unsigned char Pdepth);
        void setfb(unsigned char Pfb);
        void setdistortion(unsigned char Pdistortion);
        void setwidth(unsigned char Pwidth);
        void setoffset(unsigned char Poffset);
        void setlrcross(unsigned char Plrcross);
        void setstages(unsigned char Pstages);
        void setphase(unsigned char Pphase);

        //Internal Variables
        bool barber; //Barber pole phasing flag
        REALTYPE distortion, width, offsetpct;
        REALTYPE panning, feedback, depth, lrcross, phase;
        Stereo<REALTYPE *> old, xn1, yn1;
        Stereo<REALTYPE> diff, oldgain, fb;
        REALTYPE invperiod;
        REALTYPE offset[12];
        
        float mis;
        float Rmin;     // 3N5457 typical on resistance at Vgs = 0
        float Rmax;     // Resistor parallel to FET
        float Rmx;      // Rmin/Rmax to avoid division in loop
        float Rconst;   // Handle parallel resistor relationship
        float C;        // Capacitor
        float CFs;      // A constant derived from capacitor and resistor relationships

        void analog_setup();
        void AnalogPhase(const Stereo<REALTYPE *> &input);
        //analog case
        REALTYPE applyPhase(REALTYPE x, REALTYPE g, REALTYPE fb,
                            REALTYPE &hpf, REALTYPE *yn1, REALTYPE *xn1);

        void normalPhase(const Stereo<REALTYPE *> &input);
        REALTYPE applyPhase(REALTYPE x, REALTYPE g, REALTYPE *old);
};

#endif

