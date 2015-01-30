/*
  ZynAddSubFX - a software synthesizer

  EffectLFO.h - Stereo LFO used by some effects
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

#ifndef EFFECT_LFO_H
#define EFFECT_LFO_H

/**LFO for some of the Effect objects
 * \todo see if this should inherit LFO*/
class EffectLFO
{
    public:
        EffectLFO(float srate_f, float bufsize_f);
        ~EffectLFO();
        void effectlfoout(float *outl, float *outr);
        void updateparams(void);
        unsigned char Pfreq;
        unsigned char Prandomness;
        unsigned char PLFOtype;
        unsigned char Pstereo; // 64 is centered
    private:
        float getlfoshape(float x);

        float xl, xr;
        float incx;
        float ampl1, ampl2, ampr1, ampr2; //necessary for "randomness"
        float lfornd;
        char  lfotype;

        // current setup
        float samplerate_f;
        float buffersize_f;
};

#endif
