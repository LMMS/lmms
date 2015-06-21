/*
  ZynAddSubFX - a software synthesizer

  LFO.cpp - LFO implementation
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

#include "LFO.h"
#include "../Params/LFOParams.h"
#include "../Misc/Util.h"

#include <cstdlib>
#include <cstdio>
#include <cmath>

LFO::LFO(const LFOParams &lfopars, float basefreq)
{
    int stretch = lfopars.Pstretch;
    if(stretch == 0)
        stretch = 1;

    //max 2x/octave
    const float lfostretch = powf(basefreq / 440.0f, (stretch - 64.0f) / 63.0f);

    float lfofreq =
        (powf(2, lfopars.Pfreq * 10.0f) - 1.0f) / 12.0f * lfostretch;
    incx = fabs(lfofreq) * synth->buffersize_f / synth->samplerate_f;

    if(!lfopars.Pcontinous) {
        if(lfopars.Pstartphase == 0)
            x = RND;
        else
            x = fmod((lfopars.Pstartphase - 64.0f) / 127.0f + 1.0f, 1.0f);
    }
    else {
        const float tmp = fmod(lfopars.time * incx, 1.0f);
        x = fmod((lfopars.Pstartphase - 64.0f) / 127.0f + 1.0f + tmp, 1.0f);
    }

    //Limit the Frequency(or else...)
    if(incx > 0.49999999f)
        incx = 0.499999999f;


    lfornd = limit(lfopars.Prandomness / 127.0f, 0.0f, 1.0f);
    lfofreqrnd = powf(lfopars.Pfreqrand / 127.0f, 2.0f) * 4.0f;

    switch(lfopars.fel) {
        case 1:
            lfointensity = lfopars.Pintensity / 127.0f;
            break;
        case 2:
            lfointensity = lfopars.Pintensity / 127.0f * 4.0f;
            break; //in octave
        default:
            lfointensity = powf(2, lfopars.Pintensity / 127.0f * 11.0f) - 1.0f; //in centi
            x -= 0.25f; //chance the starting phase
            break;
    }

    amp1     = (1 - lfornd) + lfornd * RND;
    amp2     = (1 - lfornd) + lfornd * RND;
    lfotype  = lfopars.PLFOtype;
    lfodelay = lfopars.Pdelay / 127.0f * 4.0f; //0..4 sec
    incrnd   = nextincrnd = 1.0f;
    freqrndenabled = (lfopars.Pfreqrand != 0);
    computenextincrnd();
    computenextincrnd(); //twice because I want incrnd & nextincrnd to be random
}

LFO::~LFO()
{}

/*
 * LFO out
 */
float LFO::lfoout()
{
    float out;
    switch(lfotype) {
        case 1: //LFO_TRIANGLE
            if((x >= 0.0f) && (x < 0.25f))
                out = 4.0f * x;
            else
            if((x > 0.25f) && (x < 0.75f))
                out = 2 - 4 * x;
            else
                out = 4.0f * x - 4.0f;
            break;
        case 2: //LFO_SQUARE
            if(x < 0.5f)
                out = -1;
            else
                out = 1;
            break;
        case 3: //LFO_RAMPUP
            out = (x - 0.5f) * 2.0f;
            break;
        case 4: //LFO_RAMPDOWN
            out = (0.5f - x) * 2.0f;
            break;
        case 5: //LFO_EXP_DOWN 1
            out = powf(0.05f, x) * 2.0f - 1.0f;
            break;
        case 6: //LFO_EXP_DOWN 2
            out = powf(0.001f, x) * 2.0f - 1.0f;
            break;
        default:
            out = cosf(x * 2.0f * PI); //LFO_SINE
    }


    if((lfotype == 0) || (lfotype == 1))
        out *= lfointensity * (amp1 + x * (amp2 - amp1));
    else
        out *= lfointensity * amp2;
    if(lfodelay < 0.00001f) {
        if(freqrndenabled == 0)
            x += incx;
        else {
            float tmp = (incrnd * (1.0f - x) + nextincrnd * x);
            if(tmp > 1.0f)
                tmp = 1.0f;
            else
            if(tmp < 0.0f)
                tmp = 0.0f;
            x += incx * tmp;
        }
        if(x >= 1) {
            x    = fmod(x, 1.0f);
            amp1 = amp2;
            amp2 = (1 - lfornd) + lfornd * RND;

            computenextincrnd();
        }
    }
    else
        lfodelay -= synth->buffersize_f / synth->samplerate_f;
    return out;
}

/*
 * LFO out (for amplitude)
 */
float LFO::amplfoout()
{
    return limit(1.0f - lfointensity + lfoout(), -1.0f, 1.0f);
}


void LFO::computenextincrnd()
{
    if(freqrndenabled == 0)
        return;
    incrnd     = nextincrnd;
    nextincrnd = powf(0.5f, lfofreqrnd) + RND * (powf(2.0f, lfofreqrnd) - 1.0f);
}
