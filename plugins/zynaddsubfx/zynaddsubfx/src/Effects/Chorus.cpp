/*
  ZynAddSubFX - a software synthesizer

  Chorus.cpp - Chorus and Flange effects
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

#include <cmath>
#include "Chorus.h"
#include <iostream>

using namespace std;

Chorus::Chorus(bool insertion_, float *const efxoutl_, float *efxoutr_, unsigned int srate, int bufsize)
    :Effect(insertion_, efxoutl_, efxoutr_, NULL, 0, srate, bufsize),
      lfo(srate, bufsize),
      maxdelay((int)(MAX_CHORUS_DELAY / 1000.0f * samplerate_f)),
      delaySample(new float[maxdelay], new float[maxdelay])
{
    dlk = 0;
    drk = 0;
    setpreset(Ppreset);
    changepar(1, 64);
    lfo.effectlfoout(&lfol, &lfor);
    dl2 = getdelay(lfol);
    dr2 = getdelay(lfor);
    cleanup();
}

Chorus::~Chorus()
{
    delete [] delaySample.l;
    delete [] delaySample.r;
}

//get the delay value in samples; xlfo is the current lfo value
float Chorus::getdelay(float xlfo)
{
    float result =
        (Pflangemode) ? 0 : (delay + xlfo * depth) * samplerate_f;

    //check if delay is too big (caused by bad setdelay() and setdepth()
    if((result + 0.5f) >= maxdelay) {
        cerr
        <<
        "WARNING: Chorus.cpp::getdelay(..) too big delay (see setdelay and setdepth funcs.)"
        << endl;
        result = maxdelay - 1.0f;
    }
    return result;
}

//Apply the effect
void Chorus::out(const Stereo<float *> &input)
{
    const float one = 1.0f;
    dl1 = dl2;
    dr1 = dr2;
    lfo.effectlfoout(&lfol, &lfor);

    dl2 = getdelay(lfol);
    dr2 = getdelay(lfor);

    for(int i = 0; i < buffersize; ++i) {
        float inL = input.l[i];
        float inR = input.r[i];
        //LRcross
        Stereo<float> tmpc(inL, inR);
        inL = tmpc.l * (1.0f - lrcross) + tmpc.r * lrcross;
        inR = tmpc.r * (1.0f - lrcross) + tmpc.l * lrcross;

        //Left channel

        //compute the delay in samples using linear interpolation between the lfo delays
        float mdel =
            (dl1 * (buffersize - i) + dl2 * i) / buffersize_f;
        if(++dlk >= maxdelay)
            dlk = 0;
        float tmp = dlk - mdel + maxdelay * 2.0f; //where should I get the sample from

        dlhi  = (int) tmp;
        dlhi %= maxdelay;

        float dlhi2 = (dlhi - 1 + maxdelay) % maxdelay;
        float dllo  = 1.0f - fmod(tmp, one);
        efxoutl[i] = cinterpolate(delaySample.l, maxdelay, dlhi2) * dllo
                     + cinterpolate(delaySample.l, maxdelay,
                                    dlhi) * (1.0f - dllo);
        delaySample.l[dlk] = inL + efxoutl[i] * fb;

        //Right channel

        //compute the delay in samples using linear interpolation between the lfo delays
        mdel = (dr1 * (buffersize - i) + dr2 * i) / buffersize_f;
        if(++drk >= maxdelay)
            drk = 0;
        tmp = drk * 1.0f - mdel + maxdelay * 2.0f; //where should I get the sample from

        dlhi  = (int) tmp;
        dlhi %= maxdelay;

        dlhi2      = (dlhi - 1 + maxdelay) % maxdelay;
        dllo       = 1.0f - fmodf(tmp, one);
        efxoutr[i] = cinterpolate(delaySample.r, maxdelay, dlhi2) * dllo
                     + cinterpolate(delaySample.r, maxdelay,
                                    dlhi) * (1.0f - dllo);
        delaySample.r[dlk] = inR + efxoutr[i] * fb;
    }

    if(Poutsub)
        for(int i = 0; i < buffersize; ++i) {
            efxoutl[i] *= -1.0f;
            efxoutr[i] *= -1.0f;
        }

    for(int i = 0; i < buffersize; ++i) {
        efxoutl[i] *= pangainL;
        efxoutr[i] *= pangainR;
    }
}

//Cleanup the effect
void Chorus::cleanup(void)
{
    memset(delaySample.l, 0, maxdelay * sizeof(float));
    memset(delaySample.r, 0, maxdelay * sizeof(float));
}

//Parameter control
void Chorus::setdepth(unsigned char _Pdepth)
{
    Pdepth = _Pdepth;
    depth  = (powf(8.0f, (Pdepth / 127.0f) * 2.0f) - 1.0f) / 1000.0f; //seconds
}

void Chorus::setdelay(unsigned char _Pdelay)
{
    Pdelay = _Pdelay;
    delay  = (powf(10.0f, (Pdelay / 127.0f) * 2.0f) - 1.0f) / 1000.0f; //seconds
}

void Chorus::setfb(unsigned char _Pfb)
{
    Pfb = _Pfb;
    fb  = (Pfb - 64.0f) / 64.1f;
}

void Chorus::setvolume(unsigned char _Pvolume)
{
    Pvolume   = _Pvolume;
    outvolume = Pvolume / 127.0f;
    volume    = (!insertion) ? 1.0f : outvolume;
}


void Chorus::setpreset(unsigned char npreset)
{
    const int     PRESET_SIZE = 12;
    const int     NUM_PRESETS = 10;
    unsigned char presets[NUM_PRESETS][PRESET_SIZE] = {
        //Chorus1
        {64, 64, 50, 0,   0, 90, 40,  85, 64,  119, 0, 0},
        //Chorus2
        {64, 64, 45, 0,   0, 98, 56,  90, 64,  19,  0, 0},
        //Chorus3
        {64, 64, 29, 0,   1, 42, 97,  95, 90,  127, 0, 0},
        //Celeste1
        {64, 64, 26, 0,   0, 42, 115, 18, 90,  127, 0, 0},
        //Celeste2
        {64, 64, 29, 117, 0, 50, 115, 9,  31,  127, 0, 1},
        //Flange1
        {64, 64, 57, 0,   0, 60, 23,  3,  62,  0,   0, 0},
        //Flange2
        {64, 64, 33, 34,  1, 40, 35,  3,  109, 0,   0, 0},
        //Flange3
        {64, 64, 53, 34,  1, 94, 35,  3,  54,  0,   0, 1},
        //Flange4
        {64, 64, 40, 0,   1, 62, 12,  19, 97,  0,   0, 0},
        //Flange5
        {64, 64, 55, 105, 0, 24, 39,  19, 17,  0,   0, 1}
    };

    if(npreset >= NUM_PRESETS)
        npreset = NUM_PRESETS - 1;
    for(int n = 0; n < PRESET_SIZE; ++n)
        changepar(n, presets[npreset][n]);
    Ppreset = npreset;
}


void Chorus::changepar(int npar, unsigned char value)
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
            break;
        case 5:
            lfo.Pstereo = value;
            lfo.updateparams();
            break;
        case 6:
            setdepth(value);
            break;
        case 7:
            setdelay(value);
            break;
        case 8:
            setfb(value);
            break;
        case 9:
            setlrcross(value);
            break;
        case 10:
            Pflangemode = (value > 1) ? 1 : value;
            break;
        case 11:
            Poutsub = (value > 1) ? 1 : value;
            break;
    }
}

unsigned char Chorus::getpar(int npar) const
{
    switch(npar) {
        case 0:  return Pvolume;
        case 1:  return Ppanning;
        case 2:  return lfo.Pfreq;
        case 3:  return lfo.Prandomness;
        case 4:  return lfo.PLFOtype;
        case 5:  return lfo.Pstereo;
        case 6:  return Pdepth;
        case 7:  return Pdelay;
        case 8:  return Pfb;
        case 9:  return Plrcross;
        case 10: return Pflangemode;
        case 11: return Poutsub;
        default: return 0;
    }
}
