/*
  ZynAddSubFX - a software synthesizer

  Effect.cpp - this class is inherited by the all effects(Reverb, Echo, ..)
  Copyright (C) 2002-2005 Nasca Octavian Paul
  Copyright 2011, Alan Calvert
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

#include "Effect.h"
#include "../Params/FilterParams.h"
#include <cmath>

Effect::Effect(bool insertion_, float *efxoutl_, float *efxoutr_,
               FilterParams *filterpars_, unsigned char Ppreset_,
               unsigned int srate, int bufsize)
    :Ppreset(Ppreset_),
      efxoutl(efxoutl_),
      efxoutr(efxoutr_),
      filterpars(filterpars_),
      insertion(insertion_),
      samplerate(srate),
      buffersize(bufsize)
{
    alias();
}

void Effect::out(float *const smpsl, float *const smpsr)
{
    out(Stereo<float *>(smpsl, smpsr));
}

void Effect::crossover(float &a, float &b, float crossover)
{
    float tmpa = a;
    float tmpb = b;
    a = tmpa * (1.0f - crossover) + tmpb * crossover;
    b = tmpb * (1.0f - crossover) + tmpa * crossover;
}

void Effect::setpanning(char Ppanning_)
{
    Ppanning = Ppanning_;
    float t = (Ppanning > 0) ? (float)(Ppanning - 1) / 126.0f : 0.0f;
    pangainL = cosf(t * PI / 2.0f);
    pangainR = cosf((1.0f - t) * PI / 2.0f);
}

void Effect::setlrcross(char Plrcross_)
{
    Plrcross = Plrcross_;
    lrcross  = (float)Plrcross / 127.0f;
}
