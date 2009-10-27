/*
  ZynAddSubFX - a software synthesizer

  Distorsion.C - Distorsion effect
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
#include "Distorsion.h"


/*
 * Waveshape (this is called by OscilGen::waveshape and Distorsion::process)
 */

void waveshapesmps(int n,
                   REALTYPE *smps,
                   unsigned char type,
                   unsigned char drive)
{
    int      i;
    REALTYPE ws = drive / 127.0;
    REALTYPE tmpv;

    switch(type) {
    case 1:
        ws = pow(10, ws * ws * 3.0) - 1.0 + 0.001; //Arctangent
        for(i = 0; i < n; i++)
            smps[i] = atan(smps[i] * ws) / atan(ws);
        break;
    case 2:
        ws = ws * ws * 32.0 + 0.0001; //Asymmetric
        if(ws < 1.0)
            tmpv = sin(ws) + 0.1;
        else
            tmpv = 1.1;
        for(i = 0; i < n; i++)
            smps[i] = sin(smps[i] * (0.1 + ws - ws * smps[i])) / tmpv;
        ;
        break;
    case 3:
        ws = ws * ws * ws * 20.0 + 0.0001; //Pow
        for(i = 0; i < n; i++) {
            smps[i] *= ws;
            if(fabs(smps[i]) < 1.0) {
                smps[i] = (smps[i] - pow(smps[i], 3.0)) * 3.0;
                if(ws < 1.0)
                    smps[i] /= ws;
            }
            else
                smps[i] = 0.0;
        }
        break;
    case 4:
        ws = ws * ws * ws * 32.0 + 0.0001; //Sine
        if(ws < 1.57)
            tmpv = sin(ws);
        else
            tmpv = 1.0;
        for(i = 0; i < n; i++)
            smps[i] = sin(smps[i] * ws) / tmpv;
        break;
    case 5:
        ws = ws * ws + 0.000001; //Quantisize
        for(i = 0; i < n; i++)
            smps[i] = floor(smps[i] / ws + 0.5) * ws;
        break;
    case 6:
        ws = ws * ws * ws * 32 + 0.0001; //Zigzag
        if(ws < 1.0)
            tmpv = sin(ws);
        else
            tmpv = 1.0;
        for(i = 0; i < n; i++)
            smps[i] = asin(sin(smps[i] * ws)) / tmpv;
        break;
    case 7:
        ws = pow(2.0, -ws * ws * 8.0); //Limiter
        for(i = 0; i < n; i++) {
            REALTYPE tmp = smps[i];
            if(fabs(tmp) > ws) {
                if(tmp >= 0.0)
                    smps[i] = 1.0;
                else
                    smps[i] = -1.0;
            }
            else
                smps[i] /= ws;
        }
        break;
    case 8:
        ws = pow(2.0, -ws * ws * 8.0); //Upper Limiter
        for(i = 0; i < n; i++) {
            REALTYPE tmp = smps[i];
            if(tmp > ws)
                smps[i] = ws;
            smps[i] *= 2.0;
        }
        break;
    case 9:
        ws = pow(2.0, -ws * ws * 8.0); //Lower Limiter
        for(i = 0; i < n; i++) {
            REALTYPE tmp = smps[i];
            if(tmp < -ws)
                smps[i] = -ws;
            smps[i] *= 2.0;
        }
        break;
    case 10:
        ws = (pow(2.0, ws * 6.0) - 1.0) / pow(2.0, 6.0); //Inverse Limiter
        for(i = 0; i < n; i++) {
            REALTYPE tmp = smps[i];
            if(fabs(tmp) > ws) {
                if(tmp >= 0.0)
                    smps[i] = tmp - ws;
                else
                    smps[i] = tmp + ws;
            }
            else
                smps[i] = 0;
        }
        break;
    case 11:
        ws = pow(5, ws * ws * 1.0) - 1.0; //Clip
        for(i = 0; i < n; i++)
            smps[i] = smps[i]
                      * (ws + 0.5) * 0.9999 - floor(
                0.5 + smps[i] * (ws + 0.5) * 0.9999);
        break;
    case 12:
        ws = ws * ws * ws * 30 + 0.001; //Asym2
        if(ws < 0.3)
            tmpv = ws;
        else
            tmpv = 1.0;
        for(i = 0; i < n; i++) {
            REALTYPE tmp = smps[i] * ws;
            if((tmp > -2.0) && (tmp < 1.0))
                smps[i] = tmp * (1.0 - tmp) * (tmp + 2.0) / tmpv;
            else
                smps[i] = 0.0;
        }
        break;
    case 13:
        ws = ws * ws * ws * 32.0 + 0.0001; //Pow2
        if(ws < 1.0)
            tmpv = ws * (1 + ws) / 2.0;
        else
            tmpv = 1.0;
        for(i = 0; i < n; i++) {
            REALTYPE tmp = smps[i] * ws;
            if((tmp > -1.0) && (tmp < 1.618034))
                smps[i] = tmp * (1.0 - tmp) / tmpv;
            else
            if(tmp > 0.0)
                smps[i] = -1.0;
            else
                smps[i] = -2.0;
        }
        break;
    case 14:
        ws = pow(ws, 5.0) * 80.0 + 0.0001; //sigmoid
        if(ws > 10.0)
            tmpv = 0.5;
        else
            tmpv = 0.5 - 1.0 / (exp(ws) + 1.0);
        for(i = 0; i < n; i++) {
            REALTYPE tmp = smps[i] * ws;
            if(tmp < -10.0)
                tmp = -10.0;
            else
            if(tmp > 10.0)
                tmp = 10.0;
            tmp     = 0.5 - 1.0 / (exp(tmp) + 1.0);
            smps[i] = tmp / tmpv;
        }
        break;
        /**\todo update to Distorsion::changepar (Ptype max) if there is added more waveshapings functions*/
    }
}


Distorsion::Distorsion(const int &insertion_,
                       REALTYPE *efxoutl_,
                       REALTYPE *efxoutr_)
    :Effect(insertion_, efxoutl_, efxoutr_, NULL, 0)
{
    lpfl = new AnalogFilter(2, 22000, 1, 0);
    lpfr = new AnalogFilter(2, 22000, 1, 0);
    hpfl = new AnalogFilter(3, 20, 1, 0);
    hpfr = new AnalogFilter(3, 20, 1, 0);


    //default values
    Pvolume = 50;
    Plrcross      = 40;
    Pdrive        = 90;
    Plevel        = 64;
    Ptype         = 0;
    Pnegate       = 0;
    Plpf          = 127;
    Phpf          = 0;
    Pstereo       = 0;
    Pprefiltering = 0;

    setpreset(Ppreset);
    cleanup();
}

Distorsion::~Distorsion()
{
    delete lpfl;
    delete lpfr;
    delete hpfl;
    delete hpfr;
}

/*
 * Cleanup the effect
 */
void Distorsion::cleanup()
{
    lpfl->cleanup();
    hpfl->cleanup();
    lpfr->cleanup();
    hpfr->cleanup();
}


/*
 * Apply the filters
 */

void Distorsion::applyfilters(REALTYPE *efxoutl, REALTYPE *efxoutr)
{
    lpfl->filterout(efxoutl);
    hpfl->filterout(efxoutl);
    if(Pstereo != 0) { //stereo
        lpfr->filterout(efxoutr);
        hpfr->filterout(efxoutr);
    }
}


/*
 * Effect output
 */
void Distorsion::out(REALTYPE *smpsl, REALTYPE *smpsr)
{
    int      i;
    REALTYPE l, r, lout, rout;

    REALTYPE inputvol = pow(5.0, (Pdrive - 32.0) / 127.0);
    if(Pnegate != 0)
        inputvol *= -1.0;

    if(Pstereo != 0) { //Stereo
        for(i = 0; i < SOUND_BUFFER_SIZE; i++) {
            efxoutl[i] = smpsl[i] * inputvol * panning;
            efxoutr[i] = smpsr[i] * inputvol * (1.0 - panning);
        }
    }
    else {
        for(i = 0; i < SOUND_BUFFER_SIZE; i++)
            efxoutl[i] =
                (smpsl[i] * panning + smpsr[i] * (1.0 - panning)) * inputvol;
        ;
    }

    if(Pprefiltering != 0)
        applyfilters(efxoutl, efxoutr);

    //no optimised, yet (no look table)
    waveshapesmps(SOUND_BUFFER_SIZE, efxoutl, Ptype + 1, Pdrive);
    if(Pstereo != 0)
        waveshapesmps(SOUND_BUFFER_SIZE, efxoutr, Ptype + 1, Pdrive);

    if(Pprefiltering == 0)
        applyfilters(efxoutl, efxoutr);

    if(Pstereo == 0)
        for(i = 0; i < SOUND_BUFFER_SIZE; i++)
            efxoutr[i] = efxoutl[i];

    REALTYPE level = dB2rap(60.0 * Plevel / 127.0 - 40.0);
    for(i = 0; i < SOUND_BUFFER_SIZE; i++) {
        lout = efxoutl[i];
        rout = efxoutr[i];
        l    = lout * (1.0 - lrcross) + rout * lrcross;
        r    = rout * (1.0 - lrcross) + lout * lrcross;
        lout = l;
        rout = r;

        efxoutl[i] = lout * 2.0 * level;
        efxoutr[i] = rout * 2.0 * level;
    }
}


/*
 * Parameter control
 */
void Distorsion::setvolume(const unsigned char &Pvolume)
{
    this->Pvolume = Pvolume;

    if(insertion == 0) {
        outvolume = pow(0.01, (1.0 - Pvolume / 127.0)) * 4.0;
        volume    = 1.0;
    }
    else
        volume = outvolume = Pvolume / 127.0;
    ;
    if(Pvolume == 0)
        cleanup();
}

void Distorsion::setpanning(const unsigned char &Ppanning)
{
    this->Ppanning = Ppanning;
    panning = (Ppanning + 0.5) / 127.0;
}


void Distorsion::setlrcross(const unsigned char &Plrcross)
{
    this->Plrcross = Plrcross;
    lrcross = Plrcross / 127.0 * 1.0;
}

void Distorsion::setlpf(const unsigned char &Plpf)
{
    this->Plpf = Plpf;
    REALTYPE fr = exp(pow(Plpf / 127.0, 0.5) * log(25000.0)) + 40;
    lpfl->setfreq(fr);
    lpfr->setfreq(fr);
}

void Distorsion::sethpf(const unsigned char &Phpf)
{
    this->Phpf = Phpf;
    REALTYPE fr = exp(pow(Phpf / 127.0, 0.5) * log(25000.0)) + 20.0;
    hpfl->setfreq(fr);
    hpfr->setfreq(fr);
}


void Distorsion::setpreset(unsigned char npreset)
{
    const int     PRESET_SIZE = 11;
    const int     NUM_PRESETS = 6;
    unsigned char presets[NUM_PRESETS][PRESET_SIZE] = {
        //Overdrive 1
        {127, 64,  35,  56, 70, 0, 0, 96,  0,   0,   0  },
        //Overdrive 2
        {127, 64,  35,  29, 75, 1, 0, 127, 0,   0,   0  },
        //A. Exciter 1
        {64,  64,  35,  75, 80, 5, 0, 127, 105, 1,   0  },
        //A. Exciter 2
        {64,  64,  35,  85, 62, 1, 0, 127, 118, 1,   0  },
        //Guitar Amp
        {127, 64,  35,  63, 75, 2, 0, 55,  0,   0,   0  },
        //Quantisize
        {127, 64,  35,  88, 75, 4, 0, 127, 0,   1,   0  }
    };


    if(npreset >= NUM_PRESETS)
        npreset = NUM_PRESETS - 1;
    for(int n = 0; n < PRESET_SIZE; n++)
        changepar(n, presets[npreset][n]);
    if(insertion == 0)
        changepar(0, (int) (presets[npreset][0] / 1.5));           //lower the volume if this is system effect
    Ppreset = npreset;
    cleanup();
}


void Distorsion::changepar(const int &npar, const unsigned char &value)
{
    switch(npar) {
    case 0:
        setvolume(value);
        break;
    case 1:
        setpanning(value);
        break;
    case 2:
        setlrcross(value);
        break;
    case 3:
        Pdrive = value;
        break;
    case 4:
        Plevel = value;
        break;
    case 5:
        if(value > 13)
            Ptype = 13;        //this must be increased if more distorsion types are added
        else
            Ptype = value;
        break;
    case 6:
        if(value > 1)
            Pnegate = 1;
        else
            Pnegate = value;
        break;
    case 7:
        setlpf(value);
        break;
    case 8:
        sethpf(value);
        break;
    case 9:
        if(value > 1)
            Pstereo = 1;
        else
            Pstereo = value;
        break;
    case 10:
        Pprefiltering = value;
        break;
    }
}

unsigned char Distorsion::getpar(const int &npar) const
{
    switch(npar) {
    case 0:
        return Pvolume;
        break;
    case 1:
        return Ppanning;
        break;
    case 2:
        return Plrcross;
        break;
    case 3:
        return Pdrive;
        break;
    case 4:
        return Plevel;
        break;
    case 5:
        return Ptype;
        break;
    case 6:
        return Pnegate;
        break;
    case 7:
        return Plpf;
        break;
    case 8:
        return Phpf;
        break;
    case 9:
        return Pstereo;
        break;
    case 10:
        return Pprefiltering;
        break;
    }
    return 0; //in case of bogus parameter number
}

