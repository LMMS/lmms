/*
  ZynAddSubFX - a software synthesizer

  Envelope.cpp - Envelope implementation
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
#include "Envelope.h"
#include "../Params/EnvelopeParams.h"

Envelope::Envelope(EnvelopeParams *envpars, float basefreq)
{
    int i;
    envpoints = envpars->Penvpoints;
    if(envpoints > MAX_ENVELOPE_POINTS)
        envpoints = MAX_ENVELOPE_POINTS;
    envsustain     = (envpars->Penvsustain == 0) ? -1 : envpars->Penvsustain;
    forcedrelase   = envpars->Pforcedrelease;
    envstretch     = powf(440.0f / basefreq, envpars->Penvstretch / 64.0f);
    linearenvelope = envpars->Plinearenvelope;

    if(envpars->Pfreemode == 0)
        envpars->converttofree();

    float bufferdt = synth->buffersize_f / synth->samplerate_f;

    int mode = envpars->Envmode;

    //for amplitude envelopes
    if((mode == 1) && (linearenvelope == 0))
        mode = 2;                              //change to log envelope
    if((mode == 2) && (linearenvelope != 0))
        mode = 1;                              //change to linear

    for(i = 0; i < MAX_ENVELOPE_POINTS; ++i) {
        float tmp = envpars->getdt(i) / 1000.0f * envstretch;
        if(tmp > bufferdt)
            envdt[i] = bufferdt / tmp;
        else
            envdt[i] = 2.0f;  //any value larger than 1

        switch(mode) {
            case 2:
                envval[i] = (1.0f - envpars->Penvval[i] / 127.0f) * -40;
                break;
            case 3:
                envval[i] =
                    (powf(2, 6.0f
                          * fabs(envpars->Penvval[i]
                                 - 64.0f) / 64.0f) - 1.0f) * 100.0f;
                if(envpars->Penvval[i] < 64)
                    envval[i] = -envval[i];
                break;
            case 4:
                envval[i] = (envpars->Penvval[i] - 64.0f) / 64.0f * 6.0f; //6 octaves (filtru)
                break;
            case 5:
                envval[i] = (envpars->Penvval[i] - 64.0f) / 64.0f * 10;
                break;
            default:
                envval[i] = envpars->Penvval[i] / 127.0f;
        }
    }

    envdt[0] = 1.0f;

    currentpoint = 1; //the envelope starts from 1
    keyreleased  = false;
    t = 0.0f;
    envfinish = false;
    inct      = envdt[1];
    envoutval = 0.0f;
}

Envelope::~Envelope()
{}


/*
 * Relase the key (note envelope)
 */
void Envelope::relasekey()
{
    if(keyreleased)
        return;
    keyreleased = true;
    if(forcedrelase != 0)
        t = 0.0f;
}

/*
 * Envelope Output
 */
float Envelope::envout()
{
    float out;

    if(envfinish) { //if the envelope is finished
        envoutval = envval[envpoints - 1];
        return envoutval;
    }
    if((currentpoint == envsustain + 1) && !keyreleased) { //if it is sustaining now
        envoutval = envval[envsustain];
        return envoutval;
    }

    if(keyreleased && (forcedrelase != 0)) { //do the forced release
        int tmp = (envsustain < 0) ? (envpoints - 1) : (envsustain + 1); //if there is no sustain point, use the last point for release

        if(envdt[tmp] < 0.00000001f)
            out = envval[tmp];
        else
            out = envoutval + (envval[tmp] - envoutval) * t;
        t += envdt[tmp] * envstretch;

        if(t >= 1.0f) {
            currentpoint = envsustain + 2;
            forcedrelase = 0;
            t    = 0.0f;
            inct = envdt[currentpoint];
            if((currentpoint >= envpoints) || (envsustain < 0))
                envfinish = true;
        }
        return out;
    }
    if(inct >= 1.0f)
        out = envval[currentpoint];
    else
        out = envval[currentpoint - 1]
              + (envval[currentpoint] - envval[currentpoint - 1]) * t;

    t += inct;
    if(t >= 1.0f) {
        if(currentpoint >= envpoints - 1)
            envfinish = true;
        else
            currentpoint++;
        t    = 0.0f;
        inct = envdt[currentpoint];
    }

    envoutval = out;
    return out;
}

/*
 * Envelope Output (dB)
 */
float Envelope::envout_dB()
{
    float out;
    if(linearenvelope != 0)
        return envout();

    if((currentpoint == 1) && (!keyreleased || (forcedrelase == 0))) { //first point is always lineary interpolated
        float v1 = dB2rap(envval[0]);
        float v2 = dB2rap(envval[1]);
        out = v1 + (v2 - v1) * t;

        t += inct;
        if(t >= 1.0f) {
            t    = 0.0f;
            inct = envdt[2];
            currentpoint++;
            out = v2;
        }

        if(out > 0.001f)
            envoutval = rap2dB(out);
        else
            envoutval = MIN_ENVELOPE_DB;
    }
    else
        out = dB2rap(envout());

    return out;
}

bool Envelope::finished() const
{
    return envfinish;
}
