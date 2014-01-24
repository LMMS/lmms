/*

  Phaser.cpp  - Phasing and Approximate digital model of an analog JFET phaser.
  Analog modeling implemented by Ryan Billing aka Transmogrifox.
  ZynAddSubFX - a software synthesizer

  Phaser.cpp - Phaser effect
  Copyright (C) 2002-2005 Nasca Octavian Paul
  Copyright (C) 2009-2010 Ryan Billing
  Copyright (C) 2010-2010 Mark McCurry
  Author: Nasca Octavian Paul
          Ryan Billing
          Mark McCurry

  DSP analog modeling theory & practice largely influenced by various CCRMA publications, particularly works by Julius O. Smith.

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

#include <cmath>
#include <algorithm>
#include "Phaser.h"

using namespace std;

#define PHASER_LFO_SHAPE 2
#define ONE_  0.99999f        // To prevent LFO ever reaching 1.0 for filter stability purposes
#define ZERO_ 0.00001f        // Same idea as above.

Phaser::Phaser(const int &insertion_, REALTYPE *efxoutl_, REALTYPE *efxoutr_)
    :Effect(insertion_, efxoutl_, efxoutr_, NULL, 0), old(NULL), xn1(NULL), yn1(NULL), diff(0.0), oldgain(0.0),
     fb(0.0)
{
    analog_setup();
    setpreset(Ppreset);
    cleanup();
}

void Phaser::analog_setup()
{
    //model mismatch between JFET devices
    offset[0] = -0.2509303f;
    offset[1] = 0.9408924f;
    offset[2] = 0.998f;
    offset[3] = -0.3486182f;
    offset[4] = -0.2762545f;
    offset[5] = -0.5215785f;
    offset[6] = 0.2509303f;
    offset[7] = -0.9408924f;
    offset[8] = -0.998f;
    offset[9] = 0.3486182f;
    offset[10] = 0.2762545f;
    offset[11] = 0.5215785f;

    barber = 0;  //Deactivate barber pole phasing by default

    mis = 1.0f;
    Rmin = 625.0f;// 2N5457 typical on resistance at Vgs = 0
    Rmax = 22000.0f;// Resistor parallel to FET
    Rmx = Rmin/Rmax;
    Rconst = 1.0f + Rmx;  // Handle parallel resistor relationship
    C = 0.00000005f;     // 50 nF
    CFs = (float) 2.0f*(float)SAMPLE_RATE*C;
    invperiod = 1.0f / ((float) SOUND_BUFFER_SIZE);
}

Phaser::~Phaser()
{
    if(xn1.l)
        delete[] xn1.l;
    if(yn1.l)
        delete[] yn1.l;
    if(xn1.r)
        delete[] xn1.r;
    if(yn1.r)
        delete[] yn1.r;
}

/*
 * Effect output
 */
void Phaser::out(const Stereo<REALTYPE *> &input)
{
    if(Panalog)
        AnalogPhase(input);
    else
        normalPhase(input);
}

void Phaser::AnalogPhase(const Stereo<REALTYPE *> &input)
{
    Stereo<REALTYPE> gain(0.0), lfoVal(0.0), mod(0.0), g(0.0), b(0.0), hpf(0.0);

    lfo.effectlfoout(&lfoVal.l, &lfoVal.r);
    mod.l = lfoVal.l*width + (depth - 0.5f);
    mod.r = lfoVal.r*width + (depth - 0.5f);

    mod.l = limit(mod.l, ZERO_, ONE_);
    mod.r = limit(mod.r, ZERO_, ONE_);

    if(Phyper) {
        //Triangle wave squared is approximately sin on bottom, tri on top
        //Result is exponential sweep more akin to filter in synth with
        //exponential generator circuitry.
        mod.l *= mod.l;
        mod.r *= mod.r;
    }

    //g.l,g.r is Vp - Vgs. Typical FET drain-source resistance follows constant/[1-sqrt(Vp - Vgs)]
    mod.l = sqrtf(1.0f - mod.l);
    mod.r = sqrtf(1.0f - mod.r);

    diff.r = (mod.r - oldgain.r) * invperiod;
    diff.l = (mod.l - oldgain.l) * invperiod;

    g = oldgain;
    oldgain = mod;

    for (int i = 0; i < SOUND_BUFFER_SIZE; i++) {
        g.l += diff.l;// Linear interpolation between LFO samples
        g.r += diff.r;

        Stereo<REALTYPE> xn(input.l[i] * panning, 
                            input.r[i] * (1.0f - panning));

        if (barber) {
            g.l = fmodf((g.l + 0.25f), ONE_);
            g.r = fmodf((g.r + 0.25f), ONE_);
        }

        xn.l = applyPhase(xn.l, g.l, fb.l, hpf.l, yn1.l, xn1.l);
        xn.r = applyPhase(xn.r, g.r, fb.r, hpf.r, yn1.r, xn1.r);


        fb.l = xn.l * feedback;
        fb.r = xn.r * feedback;
        efxoutl[i] = xn.l;
        efxoutr[i] = xn.r;
    }

    if(Poutsub) {
        invSignal(efxoutl, SOUND_BUFFER_SIZE);
        invSignal(efxoutr, SOUND_BUFFER_SIZE);
    }
}

REALTYPE Phaser::applyPhase(REALTYPE x, REALTYPE g, REALTYPE fb,
                            REALTYPE &hpf, REALTYPE *yn1, REALTYPE *xn1)
{
    for(int j = 0; j < Pstages; j++) { //Phasing routine
        mis = 1.0f + offsetpct*offset[j];

        //This is symmetrical.
        //FET is not, so this deviates slightly, however sym dist. is
        //better sounding than a real FET.
        float d = (1.0f + 2.0f*(0.25f + g)*hpf*hpf*distortion) * mis;
        Rconst =  1.0f + mis*Rmx;

        // This is 1/R. R is being modulated to control filter fc.
        float b = (Rconst - g)/ (d*Rmin);
        float gain = (CFs - b)/(CFs + b);
        yn1[j] = gain * (x + yn1[j]) - xn1[j];

        //high pass filter:
        //Distortion depends on the high-pass part of the AP stage.
        hpf = yn1[j] + (1.0f-gain)*xn1[j];

        xn1[j] = x;
        x = yn1[j];
        if (j==1)
            x += fb;  //Insert feedback after first phase stage
    }
    return x;
}
void Phaser::normalPhase(const Stereo<REALTYPE *> &input)
{
    Stereo<REALTYPE> gain(0.0), lfoVal(0.0);

    lfo.effectlfoout(&lfoVal.l, &lfoVal.r);
    gain.l = (exp(lfoVal.l * PHASER_LFO_SHAPE) - 1) / (exp(PHASER_LFO_SHAPE) - 1.0);
    gain.r = (exp(lfoVal.r * PHASER_LFO_SHAPE) - 1) / (exp(PHASER_LFO_SHAPE) - 1.0);

    gain.l = 1.0 - phase * (1.0 - depth) - (1.0 - phase) * gain.l * depth;
    gain.r = 1.0 - phase * (1.0 - depth) - (1.0 - phase) * gain.r * depth;

    gain.l = limit(gain.l, ZERO_, ONE_);
    gain.r = limit(gain.r, ZERO_, ONE_);

    for(int i = 0; i < SOUND_BUFFER_SIZE; i++) {
        REALTYPE x   = (REALTYPE) i / SOUND_BUFFER_SIZE;
        REALTYPE x1  = 1.0 - x;
        //TODO think about making panning an external feature
        Stereo<REALTYPE> xn(input.l[i] * panning + fb.l,
                            input.r[i] * (1.0 - panning) + fb.r);

        Stereo<REALTYPE> g(gain.l * x + oldgain.l * x1,
                           gain.r * x + oldgain.r * x1);

        xn.l = applyPhase(xn.l, g.l, old.l);
        xn.r = applyPhase(xn.r, g.r, old.r);

        //Left/Right crossing
        crossover(xn.l, xn.r, lrcross);

        fb.l = xn.l * feedback;
        fb.r = xn.r * feedback;
        efxoutl[i] = xn.l;
        efxoutr[i] = xn.r;
    }

    oldgain = gain;

    if(Poutsub) {
        invSignal(efxoutl, SOUND_BUFFER_SIZE);
        invSignal(efxoutr, SOUND_BUFFER_SIZE);
    }
}

REALTYPE Phaser::applyPhase(REALTYPE x, REALTYPE g, REALTYPE *old)
{
    for(int j = 0; j < Pstages * 2; j++) { //Phasing routine
        REALTYPE tmp = old[j];
        old[j] = g * tmp + x;
        x = tmp - g *old[j];
    }
    return x;
}

/*
 * Cleanup the effect
 */
void Phaser::cleanup()
{
    fb = oldgain = Stereo<REALTYPE>(0.0);
    for(int i = 0; i < Pstages * 2; i++) {
        old.l[i] = 0.0;
        old.r[i] = 0.0;
    }
    for(int i = 0; i < Pstages; i++) {
        xn1.l[i] = 0.0;
        yn1.l[i] = 0.0;
        xn1.r[i] = 0.0;
        yn1.r[i] = 0.0;
    }
}

/*
 * Parameter control
 */
void Phaser::setwidth(unsigned char Pwidth)
{
    this->Pwidth = Pwidth;
    width = ((float)Pwidth / 127.0f);
}

void Phaser::setfb(unsigned char Pfb)
{
    this->Pfb = Pfb;
    feedback = (float) (Pfb - 64) / 64.2f;
}

void Phaser::setvolume(unsigned char Pvolume)
{
    this->Pvolume = Pvolume;
    outvolume     = Pvolume / 127.0;
    if(insertion == 0)
        volume = 1.0;
    else
        volume = outvolume;
}

void Phaser::setpanning(unsigned char Ppanning)
{
    this->Ppanning = Ppanning;
    panning = (float)Ppanning / 127.0;
}

void Phaser::setlrcross(unsigned char Plrcross)
{
    this->Plrcross = Plrcross;
    lrcross = Plrcross / 127.0;
}

void Phaser::setdistortion(unsigned char Pdistortion)
{
    this->Pdistortion = Pdistortion;
    distortion = (float)Pdistortion / 127.0f;
}

void Phaser::setoffset(unsigned char Poffset)
{
    this->Poffset = Poffset;
    offsetpct = (float)Poffset / 127.0f;
}

void Phaser::setstages(unsigned char Pstages)
{
    if(xn1.l)
        delete[] xn1.l;
    if(yn1.l)
        delete[] yn1.l;
    if(xn1.r)
        delete[] xn1.r;
    if(yn1.r)
        delete[] yn1.r;


    this->Pstages = min(MAX_PHASER_STAGES, (int)Pstages);

    old = Stereo<REALTYPE *>(new REALTYPE[Pstages * 2],
                             new REALTYPE[Pstages * 2]);

    xn1 = Stereo<REALTYPE *>(new REALTYPE[Pstages],
                             new REALTYPE[Pstages]);

    yn1 = Stereo<REALTYPE *>(new REALTYPE[Pstages],
                             new REALTYPE[Pstages]);

    cleanup();
}

void Phaser::setphase(unsigned char Pphase)
{
    this->Pphase = Pphase;
    phase = (Pphase / 127.0);
}

void Phaser::setdepth(unsigned char Pdepth)
{
    this->Pdepth = Pdepth;
    depth = (float)(Pdepth) / 127.0f;
}


void Phaser::setpreset(unsigned char npreset)
{
    const int PRESET_SIZE = 15;
    const int NUM_PRESETS = 12;
    unsigned char presets[NUM_PRESETS][PRESET_SIZE] = {
        //Phaser
        //0   1    2    3  4   5     6   7   8    9 10   11 12  13 14
        {64, 64,  36, 0,   0, 64,  110, 64,  1,   0, 0,  20, 0,  0, 0},
        {64, 64,  35, 0,   0, 88,  40,  64,  3,   0, 0,  20, 0,  0, 0},
        {64, 64,  31, 0,   0, 66,  68,  107, 2,   0, 0,  20, 0,  0, 0},
        {39, 64,  22, 0,   0, 66,  67,  10,  5,   0, 1,  20, 0,  0, 0},
        {64, 64,  20, 0,   1, 110, 67,  78,  10,  0, 0,  20, 0,  0, 0},
        {64, 64,  53, 100, 0, 58,  37,  78,  3,   0, 0,  20, 0,  0, 0},
        //APhaser
        //0   1    2   3   4   5     6   7   8    9 10   11 12  13 14
        {64, 64,  14,   0, 1, 64,   64, 40,  4,  10, 0, 110, 1, 20, 1},
        {64, 64,  14,   5, 1, 64,   70, 40,  6,  10, 0, 110, 1, 20, 1},
        {64, 64,   9,   0, 0, 64,   60, 40,  8,  10, 0,  40, 0, 20, 1},
        {64, 64,  14,  10, 0, 64,   45, 80,  7,  10, 1, 110, 1, 20, 1},
        {25, 64, 127,  10, 0, 64,   25, 16,  8, 100, 0,  25, 0, 20, 1},
        {64, 64,   1,  10, 1, 64,   70, 40, 12,  10, 0, 110, 1, 20, 1}
    };
    if(npreset >= NUM_PRESETS)
        npreset = NUM_PRESETS - 1;
    for(int n = 0; n < PRESET_SIZE; n++)
        changepar(n, presets[npreset][n]);
    Ppreset = npreset;
}


void Phaser::changepar(int npar, unsigned char value)
{
    switch(npar) {
        case 0:
            setvolume(value);
            break;
        case 1:
            setpanning(value);
            break;
        case 2:
            lfo.Pfreq = value;
            lfo.updateparams();
            break;
        case 3:
            lfo.Prandomness = value;
            lfo.updateparams();
            break;
        case 4:
            lfo.PLFOtype = value;
            lfo.updateparams();
            barber = (2 == value);
            break;
        case 5:
            lfo.Pstereo = value;
            lfo.updateparams();
            break;
        case 6:
            setdepth(value);
            break;
        case 7:
            setfb(value);
            break;
        case 8:
            setstages(value);
            break;
        case 9:
            setlrcross(value);
            setoffset(value);
            break;
        case 10:
            Poutsub = min((int)value,1);
            break;
        case 11:
            setphase(value);
            setwidth(value);
            break;
        case 12:
            Phyper = min((int)value, 1);
            break;
        case 13:
            setdistortion(value);
            break;
        case 14:
            Panalog = value;
            break;
    }
}

unsigned char Phaser::getpar(int npar) const
{
    switch(npar) {
        case 0:
            return Pvolume;
        case 1:
            return Ppanning;
        case 2:
            return lfo.Pfreq;
        case 3:
            return lfo.Prandomness;
        case 4:
            return lfo.PLFOtype;
        case 5:
            return lfo.Pstereo;
        case 6:
            return Pdepth;
        case 7:
            return Pfb;
        case 8:
            return Pstages;
        case 9:
            return Plrcross;
            return Poffset;
        case 10:
            return Poutsub;
        case 11:
            return Pphase;
            return Pwidth;
        case 12:
            return Phyper;
        case 13:
            return Pdistortion;
        case 14:
            return Panalog;
        default:
            return 0;
    }
}
