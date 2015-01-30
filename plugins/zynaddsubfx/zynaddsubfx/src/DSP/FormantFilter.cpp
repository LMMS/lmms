/*
  ZynAddSubFX - a software synthesizer

  FormantFilter.cpp - formant filters
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
#include <cstdio>
#include "../Misc/Util.h"
#include "FormantFilter.h"
#include "AnalogFilter.h"
#include "../Params/FilterParams.h"

FormantFilter::FormantFilter(FilterParams *pars, unsigned int srate, int bufsize)
    : Filter(srate, bufsize)
{
    numformants = pars->Pnumformants;
    for(int i = 0; i < numformants; ++i)
        formant[i] = new AnalogFilter(4 /*BPF*/, 1000.0f, 10.0f, pars->Pstages, srate, bufsize);
    cleanup();

    for(int j = 0; j < FF_MAX_VOWELS; ++j)
        for(int i = 0; i < numformants; ++i) {
            formantpar[j][i].freq = pars->getformantfreq(
                pars->Pvowels[j].formants[i].freq);
            formantpar[j][i].amp = pars->getformantamp(
                pars->Pvowels[j].formants[i].amp);
            formantpar[j][i].q = pars->getformantq(
                pars->Pvowels[j].formants[i].q);
        }

    for(int i = 0; i < FF_MAX_FORMANTS; ++i)
        oldformantamp[i] = 1.0f;
    for(int i = 0; i < numformants; ++i) {
        currentformants[i].freq = 1000.0f;
        currentformants[i].amp  = 1.0f;
        currentformants[i].q    = 2.0f;
    }

    formantslowness = powf(1.0f - (pars->Pformantslowness / 128.0f), 3.0f);

    sequencesize = pars->Psequencesize;
    if(sequencesize == 0)
        sequencesize = 1;
    for(int k = 0; k < sequencesize; ++k)
        sequence[k].nvowel = pars->Psequence[k].nvowel;

    vowelclearness = powf(10.0f, (pars->Pvowelclearness - 32.0f) / 48.0f);

    sequencestretch = powf(0.1f, (pars->Psequencestretch - 32.0f) / 48.0f);
    if(pars->Psequencereversed)
        sequencestretch *= -1.0f;

    outgain = dB2rap(pars->getgain());

    oldinput   = -1.0f;
    Qfactor    = 1.0f;
    oldQfactor = Qfactor;
    firsttime  = 1;
}

FormantFilter::~FormantFilter()
{
    for(int i = 0; i < numformants; ++i)
        delete (formant[i]);
}

void FormantFilter::cleanup()
{
    for(int i = 0; i < numformants; ++i)
        formant[i]->cleanup();
}

void FormantFilter::setpos(float input)
{
    int p1, p2;

    if(firsttime != 0)
        slowinput = input;
    else
        slowinput = slowinput
                    * (1.0f - formantslowness) + input * formantslowness;

    if((fabsf(oldinput - input) < 0.001f) && (fabsf(slowinput - input) < 0.001f)
       && (fabsf(Qfactor - oldQfactor) < 0.001f)) {
        //	oldinput=input; daca setez asta, o sa faca probleme la schimbari foarte lente
        firsttime = 0;
        return;
    }
    else
        oldinput = input;

    float pos = fmodf(input * sequencestretch, 1.0f);
    if(pos < 0.0f)
        pos += 1.0f;

    F2I(pos * sequencesize, p2);
    p1 = p2 - 1;
    if(p1 < 0)
        p1 += sequencesize;

    pos = fmodf(pos * sequencesize, 1.0f);
    if(pos < 0.0f)
        pos = 0.0f;
    else
    if(pos > 1.0f)
        pos = 1.0f;
    pos =
        (atanf((pos * 2.0f
                - 1.0f)
               * vowelclearness) / atanf(vowelclearness) + 1.0f) * 0.5f;

    p1 = sequence[p1].nvowel;
    p2 = sequence[p2].nvowel;

    if(firsttime != 0) {
        for(int i = 0; i < numformants; ++i) {
            currentformants[i].freq =
                formantpar[p1][i].freq
                * (1.0f - pos) + formantpar[p2][i].freq * pos;
            currentformants[i].amp =
                formantpar[p1][i].amp
                * (1.0f - pos) + formantpar[p2][i].amp * pos;
            currentformants[i].q =
                formantpar[p1][i].q * (1.0f - pos) + formantpar[p2][i].q * pos;
            formant[i]->setfreq_and_q(currentformants[i].freq,
                                      currentformants[i].q * Qfactor);
            oldformantamp[i] = currentformants[i].amp;
        }
        firsttime = 0;
    }
    else
        for(int i = 0; i < numformants; ++i) {
            currentformants[i].freq =
                currentformants[i].freq * (1.0f - formantslowness)
                + (formantpar[p1][i].freq
                   * (1.0f - pos) + formantpar[p2][i].freq * pos)
                * formantslowness;

            currentformants[i].amp =
                currentformants[i].amp * (1.0f - formantslowness)
                + (formantpar[p1][i].amp * (1.0f - pos)
                   + formantpar[p2][i].amp * pos) * formantslowness;

            currentformants[i].q = currentformants[i].q
                                   * (1.0f - formantslowness)
                                   + (formantpar[p1][i].q * (1.0f - pos)
                                      + formantpar[p2][i].q
                                      * pos) * formantslowness;


            formant[i]->setfreq_and_q(currentformants[i].freq,
                                      currentformants[i].q * Qfactor);
        }

    oldQfactor = Qfactor;
}

void FormantFilter::setfreq(float frequency)
{
    setpos(frequency);
}

void FormantFilter::setq(float q_)
{
    Qfactor = q_;
    for(int i = 0; i < numformants; ++i)
        formant[i]->setq(Qfactor * currentformants[i].q);
}

void FormantFilter::setgain(float /*dBgain*/)
{}

inline float log_2(float x)
{
    return logf(x) / logf(2.0f);
}

void FormantFilter::setfreq_and_q(float frequency, float q_)
{
    //Convert form real freq[Hz]
    const float freq = log_2(frequency) - 9.96578428f; //log2(1000)=9.95748f.

    Qfactor = q_;
    setpos(freq);
}


void FormantFilter::filterout(float *smp)
{
    float inbuffer[buffersize];

    memcpy(inbuffer, smp, bufferbytes);
    memset(smp, 0, bufferbytes);

    for(int j = 0; j < numformants; ++j) {
        float tmpbuf[buffersize];
        for(int i = 0; i < buffersize; ++i)
            tmpbuf[i] = inbuffer[i] * outgain;
        formant[j]->filterout(tmpbuf);

        if(ABOVE_AMPLITUDE_THRESHOLD(oldformantamp[j], currentformants[j].amp))
            for(int i = 0; i < buffersize; ++i)
                smp[i] += tmpbuf[i]
                          * INTERPOLATE_AMPLITUDE(oldformantamp[j],
                                                  currentformants[j].amp,
                                                  i,
                                                  buffersize);
        else
            for(int i = 0; i < buffersize; ++i)
                smp[i] += tmpbuf[i] * currentformants[j].amp;
        oldformantamp[j] = currentformants[j].amp;
    }
}
