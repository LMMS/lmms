/*
  ZynAddSubFX - a software synthesizer

  Resonance.cpp - Resonance
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
#include <cstdlib>
#include "Resonance.h"
#include "../Misc/Util.h"

#include <rtosc/ports.h>
#include <rtosc/port-sugar.h>

#define rObject Resonance

using namespace rtosc;
rtosc::Ports Resonance::ports = {
    rSelf(Resonance),
    rPaste(),
    rToggle(Penabled, "resonance enable"),
    rToggle(Pprotectthefundamental, "Disable resonance filter on first harmonic"),
    rParams(Prespoints, N_RES_POINTS, "Resonance data points"),
    rParamZyn(PmaxdB, "how many dB the signal may be amplified"),
    rParamZyn(Pcenterfreq, "Center frequency"),
    rParamZyn(Poctavesfreq, "The number of octaves..."),
    rActioni(randomize, rMap(min,0), rMap(max, 2), "Randomize frequency response"),
    rActioni(interpolatepeaks, rMap(min,0), rMap(max, 2), "Generate response from peak values"),
    rAction(smooth, "Smooth out frequency response"),
    rAction(zero,   "Reset frequency response"),
    //UI Value listeners
    {"centerfreq:", rDoc("Get center frequency"),  NULL,
        [](const char *, RtData &d)
        {d.reply(d.loc, "f", ((rObject*)d.obj)->getcenterfreq());}},
    {"octavesfreq:", rDoc("Get center freq of graph"), NULL,
            [](const char *, RtData &d)
        {d.reply(d.loc, "f", ((rObject*)d.obj)->getoctavesfreq());}},
};

Resonance::Resonance():Presets()
{
    setpresettype("Presonance");
    defaults();
}

Resonance::~Resonance(void)
{}


void Resonance::defaults(void)
{
    Penabled     = 0;
    PmaxdB       = 20;
    Pcenterfreq  = 64; //1 kHz
    Poctavesfreq = 64;
    Pprotectthefundamental = 0;
    ctlcenter = 1.0f;
    ctlbw     = 1.0f;
    for(int i = 0; i < N_RES_POINTS; ++i)
        Prespoints[i] = 64;
}

/*
 * Set a point of resonance function with a value
 */
void Resonance::setpoint(int n, unsigned char p)
{
    if((n < 0) || (n >= N_RES_POINTS))
        return;
    Prespoints[n] = p;
}

/*
 * Apply the resonance to FFT data
 */
void Resonance::applyres(int n, fft_t *fftdata, float freq) const
{
    if(Penabled == 0)
        return;             //if the resonance is disabled

    const float l1  = logf(getfreqx(0.0f) * ctlcenter),
                l2  = logf(2.0f) * getoctavesfreq() * ctlbw;

    //Provide an upper bound for resonance
    const float upper =
        limit<float>(array_max(Prespoints, N_RES_POINTS), 1.0f, INFINITY);

    for(int i = 1; i < n; ++i) {
        //compute where the n-th hamonics fits to the graph
        const float x  = limit((logf(freq*i) - l1) / l2, 0.0f, INFINITY) * N_RES_POINTS;
        const float dx = x - floor(x);
        const int kx1  = limit<int>(floor(x), 0, N_RES_POINTS - 1);
        const int kx2  = limit<int>(kx1 + 1,  0, N_RES_POINTS - 1);
        float y =
            ((Prespoints[kx1] * (1.0f - dx) + Prespoints[kx2] * dx)
             - upper) / 127.0f;

        y = powf(10.0f, y * PmaxdB / 20.0f);

        if((Pprotectthefundamental != 0) && (i == 1))
            y = 1.0f;

        fftdata[i] *= y;
    }
}

/*
 * Gets the response at the frequency "freq"
 */
//Requires
// - resonance data
// - max resonance
// - mapping from resonance data to frequency
float Resonance::getfreqresponse(float freq) const
{
    const float l1 = logf(getfreqx(0.0f) * ctlcenter),
                l2 = logf(2.0f) * getoctavesfreq() * ctlbw;

    //Provide an upper bound for resonance
    const float upper =
        limit<float>(array_max(Prespoints, N_RES_POINTS), 1.0f, INFINITY);

    //compute where the n-th hamonics fits to the graph
    const float x   = limit((logf(freq) - l1) / l2, 0.0f, INFINITY) * N_RES_POINTS;
    const float dx  = x - floor(x);
    const int   kx1 = limit<int>(floor(x), 0, N_RES_POINTS - 1);
    const int   kx2 = limit<int>(kx1 + 1,  0, N_RES_POINTS - 1);
    //Interpolate
    const float result =
        ((Prespoints[kx1] * (1.0f - dx) + Prespoints[kx2] * dx) - upper) / 127.0f;
    return powf(10.0f, result * PmaxdB / 20.0f);
}


/*
 * Smooth the resonance function
 */
void Resonance::smooth(void)
{
    float old = Prespoints[0];
    for(int i = 0; i < N_RES_POINTS; ++i) {
        old = old * 0.4f + Prespoints[i] * 0.6f;
        Prespoints[i] = (int) old;
    }
    old = Prespoints[N_RES_POINTS - 1];
    for(int i = N_RES_POINTS - 1; i > 0; i--) {
        old = old * 0.4f + Prespoints[i] * 0.6f;
        Prespoints[i] = (int) old + 1;
        if(Prespoints[i] > 127)
            Prespoints[i] = 127;
    }
}

/*
 * Randomize the resonance function
 */
void Resonance::randomize(int type)
{
    int r = (int)(RND * 127.0f);
    for(int i = 0; i < N_RES_POINTS; ++i) {
        Prespoints[i] = r;
        if((RND < 0.1f) && (type == 0))
            r = (int)(RND * 127.0f);
        if((RND < 0.3f) && (type == 1))
            r = (int)(RND * 127.0f);
        if(type == 2)
            r = (int)(RND * 127.0f);
    }
    smooth();
}

void Resonance::zero(void)
{
    for(int i=0; i<N_RES_POINTS; ++i) 
        setpoint(i,64);
}

/*
 * Interpolate the peaks
 */
void Resonance::interpolatepeaks(int type)
{
    int x1 = 0, y1 = Prespoints[0];
    for(int i = 1; i < N_RES_POINTS; ++i)
        if((Prespoints[i] != 64) || (i + 1 == N_RES_POINTS)) {
            int y2 = Prespoints[i];
            for(int k = 0; k < i - x1; ++k) {
                float x = (float) k / (i - x1);
                if(type == 0)
                    x = (1 - cosf(x * PI)) * 0.5f;
                Prespoints[x1 + k] = (int)(y1 * (1.0f - x) + y2 * x);
            }
            x1 = i;
            y1 = y2;
        }
}

/*
 * Get the frequency from x, where x is [0..1]; x is the x coordinate
 */
float Resonance::getfreqx(float x) const
{
    const float octf = powf(2.0f, getoctavesfreq());
    return getcenterfreq() / sqrt(octf) * powf(octf, limit(x, 0.0f, 1.0f));
}

/*
 * Get the x coordinate from frequency (used by the UI)
 */
float Resonance::getfreqpos(float freq) const
{
    return (logf(freq) - logf(getfreqx(0.0f))) / logf(2.0f) / getoctavesfreq();
}

/*
 * Get the center frequency of the resonance graph
 */
float Resonance::getcenterfreq() const
{
    return 10000.0f * powf(10, -(1.0f - Pcenterfreq / 127.0f) * 2.0f);
}

/*
 * Get the number of octave that the resonance functions applies to
 */
float Resonance::getoctavesfreq() const
{
    return 0.25f + 10.0f * Poctavesfreq / 127.0f;
}

void Resonance::sendcontroller(MidiControllers ctl, float par)
{
    if(ctl == C_resonance_center)
        ctlcenter = par;
    else
        ctlbw = par;
}

void Resonance::paste(Resonance &r)
{
    memcpy((char*)this, (char*)&r, sizeof(r));
}

void Resonance::add2XML(XMLwrapper *xml)
{
    xml->addparbool("enabled", Penabled);

    if((Penabled == 0) && (xml->minimal))
        return;

    xml->addpar("max_db", PmaxdB);
    xml->addpar("center_freq", Pcenterfreq);
    xml->addpar("octaves_freq", Poctavesfreq);
    xml->addparbool("protect_fundamental_frequency", Pprotectthefundamental);
    xml->addpar("resonance_points", N_RES_POINTS);
    for(int i = 0; i < N_RES_POINTS; ++i) {
        xml->beginbranch("RESPOINT", i);
        xml->addpar("val", Prespoints[i]);
        xml->endbranch();
    }
}


void Resonance::getfromXML(XMLwrapper *xml)
{
    Penabled = xml->getparbool("enabled", Penabled);

    PmaxdB       = xml->getpar127("max_db", PmaxdB);
    Pcenterfreq  = xml->getpar127("center_freq", Pcenterfreq);
    Poctavesfreq = xml->getpar127("octaves_freq", Poctavesfreq);
    Pprotectthefundamental = xml->getparbool("protect_fundamental_frequency",
                                             Pprotectthefundamental);
    for(int i = 0; i < N_RES_POINTS; ++i) {
        if(xml->enterbranch("RESPOINT", i) == 0)
            continue;
        Prespoints[i] = xml->getpar127("val", Prespoints[i]);
        xml->exitbranch();
    }
}
