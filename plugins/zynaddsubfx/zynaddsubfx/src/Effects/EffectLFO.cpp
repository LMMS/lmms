/*
  ZynAddSubFX - a software synthesizer

  EffectLFO.cpp - Stereo LFO used by some effects
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

#include "EffectLFO.h"
#include "../Misc/Util.h"

#include <cmath>

EffectLFO::EffectLFO(float srate_f, float bufsize_f)
    :Pfreq(40),
      Prandomness(0),
      PLFOtype(0),
      Pstereo(64),
      xl(0.0f),
      xr(0.0f),
      ampl1(RND),
      ampl2(RND),
      ampr1(RND),
      ampr2(RND),
      lfornd(0.0f),
      samplerate_f(srate_f),
      buffersize_f(bufsize_f)
{
    updateparams();
}

EffectLFO::~EffectLFO() {}

//Update the changed parameters
void EffectLFO::updateparams(void)
{
    float lfofreq = (powf(2.0f, Pfreq / 127.0f * 10.0f) - 1.0f) * 0.03f;
    incx = fabsf(lfofreq) * buffersize_f / samplerate_f;
    if(incx > 0.49999999f)
        incx = 0.499999999f;  //Limit the Frequency

    lfornd = Prandomness / 127.0f;
    lfornd = (lfornd > 1.0f) ? 1.0f : lfornd;

    if(PLFOtype > 1)
        PLFOtype = 1;  //this has to be updated if more lfo's are added
    lfotype = PLFOtype;
    xr      = fmodf(xl + (Pstereo - 64.0f) / 127.0f + 1.0f, 1.0f);
}


//Compute the shape of the LFO
float EffectLFO::getlfoshape(float x)
{
    float out;
    switch(lfotype) {
        case 1: //EffectLFO_TRIANGLE
            if((x > 0.0f) && (x < 0.25f))
                out = 4.0f * x;
            else
            if((x > 0.25f) && (x < 0.75f))
                out = 2.0f - 4.0f * x;
            else
                out = 4.0f * x - 4.0f;
            break;
        //when adding more, ensure ::updateparams() gets updated
        default:
            out = cosf(x * 2.0f * PI); //EffectLFO_SINE
    }
    return out;
}

//LFO output
void EffectLFO::effectlfoout(float *outl, float *outr)
{
    float out;

    out = getlfoshape(xl);
    if((lfotype == 0) || (lfotype == 1))
        out *= (ampl1 + xl * (ampl2 - ampl1));
    xl += incx;
    if(xl > 1.0f) {
        xl   -= 1.0f;
        ampl1 = ampl2;
        ampl2 = (1.0f - lfornd) + lfornd * RND;
    }
    *outl = (out + 1.0f) * 0.5f;

    out = getlfoshape(xr);
    if((lfotype == 0) || (lfotype == 1))
        out *= (ampr1 + xr * (ampr2 - ampr1));
    xr += incx;
    if(xr > 1.0f) {
        xr   -= 1.0f;
        ampr1 = ampr2;
        ampr2 = (1.0f - lfornd) + lfornd * RND;
    }
    *outr = (out + 1.0f) * 0.5f;
}
