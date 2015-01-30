/*
  ZynAddSubFX - a software synthesizer

  Reverb.cpp - Reverberation effect
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

#include "Reverb.h"
#include "../Misc/Util.h"
#include "../DSP/AnalogFilter.h"
#include "../DSP/Unison.h"
#include <cmath>

Reverb::Reverb(bool insertion_, float *efxoutl_, float *efxoutr_, unsigned int srate, int bufsize)
    :Effect(insertion_, efxoutl_, efxoutr_, NULL, 0, srate, bufsize),
      // defaults
      Pvolume(48),
      Ptime(64),
      Pidelay(40),
      Pidelayfb(0),
      Plpf(127),
      Phpf(0),
      Plohidamp(80),
      Ptype(1),
      Proomsize(64),
      Pbandwidth(30),
      roomsize(1.0f),
      rs(1.0f),
      bandwidth(NULL),
      idelay(NULL),
      lpf(NULL),
      hpf(NULL) // no filter
{
    for(int i = 0; i < REV_COMBS * 2; ++i) {
        comblen[i] = 800 + (int)(RND * 1400.0f);
        combk[i]   = 0;
        lpcomb[i]  = 0;
        combfb[i]  = -0.97f;
        comb[i]    = NULL;
    }

    for(int i = 0; i < REV_APS * 2; ++i) {
        aplen[i] = 500 + (int)(RND * 500.0f);
        apk[i]   = 0;
        ap[i]    = NULL;
    }
    setpreset(Ppreset);
    cleanup(); //do not call this before the comb initialisation
}


Reverb::~Reverb()
{
    delete [] idelay;
    delete hpf;
    delete lpf;

    for(int i = 0; i < REV_APS * 2; ++i)
        delete [] ap[i];
    for(int i = 0; i < REV_COMBS * 2; ++i)
        delete [] comb[i];

    if(bandwidth)
        delete bandwidth;
}

//Cleanup the effect
void Reverb::cleanup(void)
{
    int i, j;
    for(i = 0; i < REV_COMBS * 2; ++i) {
        lpcomb[i] = 0.0f;
        for(j = 0; j < comblen[i]; ++j)
            comb[i][j] = 0.0f;
    }

    for(i = 0; i < REV_APS * 2; ++i)
        for(j = 0; j < aplen[i]; ++j)
            ap[i][j] = 0.0f;

    if(idelay)
        for(i = 0; i < idelaylen; ++i)
            idelay[i] = 0.0f;
    if(hpf)
        hpf->cleanup();
    if(lpf)
        lpf->cleanup();
}

//Process one channel; 0=left, 1=right
void Reverb::processmono(int ch, float *output, float *inputbuf)
{
    //todo: implement the high part from lohidamp

    for(int j = REV_COMBS * ch; j < REV_COMBS * (ch + 1); ++j) {
        int &ck = combk[j];
        const int comblength = comblen[j];
        float    &lpcombj    = lpcomb[j];

        for(int i = 0; i < buffersize; ++i) {
            float fbout = comb[j][ck] * combfb[j];
            fbout   = fbout * (1.0f - lohifb) + lpcombj * lohifb;
            lpcombj = fbout;

            comb[j][ck] = inputbuf[i] + fbout;
            output[i]  += fbout;

            if((++ck) >= comblength)
                ck = 0;
        }
    }

    for(int j = REV_APS * ch; j < REV_APS * (1 + ch); ++j) {
        int &ak = apk[j];
        const int aplength = aplen[j];
        for(int i = 0; i < buffersize; ++i) {
            float tmp = ap[j][ak];
            ap[j][ak] = 0.7f * tmp + output[i];
            output[i] = tmp - 0.7f * ap[j][ak];
            if((++ak) >= aplength)
                ak = 0;
        }
    }
}

//Effect output
void Reverb::out(const Stereo<float *> &smp)
{
    if(!Pvolume && insertion)
        return;

    float inputbuf[buffersize];
    for(int i = 0; i < buffersize; ++i)
        inputbuf[i] = (smp.l[i] + smp.r[i]) / 2.0f;

    if(idelay)
        for(int i = 0; i < buffersize; ++i) {
            //Initial delay r
            float tmp = inputbuf[i] + idelay[idelayk] * idelayfb;
            inputbuf[i]     = idelay[idelayk];
            idelay[idelayk] = tmp;
            idelayk++;
            if(idelayk >= idelaylen)
                idelayk = 0;
        }

    if(bandwidth)
        bandwidth->process(buffersize, inputbuf);

    if(lpf)
        lpf->filterout(inputbuf);
    if(hpf)
        hpf->filterout(inputbuf);

    processmono(0, efxoutl, inputbuf); //left
    processmono(1, efxoutr, inputbuf); //right

    float lvol = rs / REV_COMBS * pangainL;
    float rvol = rs / REV_COMBS * pangainR;
    if(insertion != 0) {
        lvol *= 2.0f;
        rvol *= 2.0f;
    }
    for(int i = 0; i < buffersize; ++i) {
        efxoutl[i] *= lvol;
        efxoutr[i] *= rvol;
    }
}


//Parameter control
void Reverb::setvolume(unsigned char _Pvolume)
{
    Pvolume = _Pvolume;
    if(!insertion) {
        outvolume = powf(0.01f, (1.0f - Pvolume / 127.0f)) * 4.0f;
        volume    = 1.0f;
    }
    else {
        volume = outvolume = Pvolume / 127.0f;
        if(Pvolume == 0)
            cleanup();
    }
}

void Reverb::settime(unsigned char _Ptime)
{
    Ptime = _Ptime;
    float t = powf(60.0f, Ptime / 127.0f) - 0.97f;

    for(int i = 0; i < REV_COMBS * 2; ++i)
        combfb[i] =
            -expf((float)comblen[i] / samplerate_f * logf(0.001f) / t);
    //the feedback is negative because it removes the DC
}

void Reverb::setlohidamp(unsigned char _Plohidamp)
{
    Plohidamp = (_Plohidamp < 64) ? 64 : _Plohidamp;
    //remove this when the high part from lohidamp is added
    if(Plohidamp == 64) {
        lohidamptype = 0;
        lohifb = 0.0f;
    }
    else {
        if(Plohidamp < 64)
            lohidamptype = 1;
        if(Plohidamp > 64)
            lohidamptype = 2;
        float x = fabsf((float)(Plohidamp - 64) / 64.1f);
        lohifb = x * x;
    }
}

void Reverb::setidelay(unsigned char _Pidelay)
{
    Pidelay = _Pidelay;
    float delay = powf(50.0f * Pidelay / 127.0f, 2.0f) - 1.0f;

    if(idelay)
        delete [] idelay;
    idelay = NULL;

    idelaylen = (int) (samplerate_f * delay / 1000);
    if(idelaylen > 1) {
        idelayk = 0;
        idelay  = new float[idelaylen];
        memset(idelay, 0, idelaylen * sizeof(float));
    }
}

void Reverb::setidelayfb(unsigned char _Pidelayfb)
{
    Pidelayfb = _Pidelayfb;
    idelayfb  = Pidelayfb / 128.0f;
}

void Reverb::sethpf(unsigned char _Phpf)
{
    Phpf = _Phpf;
    if(Phpf == 0) { //No HighPass
        if(hpf)
            delete hpf;
        hpf = NULL;
    }
    else {
        float fr = expf(powf(Phpf / 127.0f, 0.5f) * logf(10000.0f)) + 20.0f;
        if(hpf == NULL)
            hpf = new AnalogFilter(3, fr, 1, 0, samplerate, buffersize);
        else
            hpf->setfreq(fr);
    }
}

void Reverb::setlpf(unsigned char _Plpf)
{
    Plpf = _Plpf;
    if(Plpf == 127) { //No LowPass
        if(lpf)
            delete lpf;
        lpf = NULL;
    }
    else {
        float fr = expf(powf(Plpf / 127.0f, 0.5f) * logf(25000.0f)) + 40.0f;
        if(!lpf)
            lpf = new AnalogFilter(2, fr, 1, 0, samplerate, buffersize);
        else
            lpf->setfreq(fr);
    }
}

void Reverb::settype(unsigned char _Ptype)
{
    Ptype = _Ptype;
    const int NUM_TYPES = 3;
    const int combtunings[NUM_TYPES][REV_COMBS] = {
        //this is unused (for random)
        {0,    0,    0,    0,    0,    0,    0,    0      },
        //Freeverb by Jezar at Dreampoint
        {1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617   },
        //duplicate of Freeverb by Jezar at Dreampoint
        {1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617   }
    };

    const int aptunings[NUM_TYPES][REV_APS] = {
        //this is unused (for random)
        {0,   0,   0,   0    },
        //Freeverb by Jezar at Dreampoint
        {225, 341, 441, 556  },
        //duplicate of Freeverb by Jezar at Dreampoint
        {225, 341, 441, 556  }
    };

    if(Ptype >= NUM_TYPES)
        Ptype = NUM_TYPES - 1;

    // adjust the combs according to the samplerate
    float samplerate_adjust = samplerate_f / 44100.0f;
    float tmp;
    for(int i = 0; i < REV_COMBS * 2; ++i) {
        if(Ptype == 0)
            tmp = 800.0f + (int)(RND * 1400.0f);
        else
            tmp = combtunings[Ptype][i % REV_COMBS];
        tmp *= roomsize;
        if(i > REV_COMBS)
            tmp += 23.0f;
        tmp *= samplerate_adjust; //adjust the combs according to the samplerate
        if(tmp < 10.0f)
            tmp = 10.0f;
        comblen[i] = (int) tmp;
        combk[i]   = 0;
        lpcomb[i]  = 0;
        if(comb[i])
            delete [] comb[i];
        comb[i] = new float[comblen[i]];
    }

    for(int i = 0; i < REV_APS * 2; ++i) {
        if(Ptype == 0)
            tmp = 500 + (int)(RND * 500.0f);
        else
            tmp = aptunings[Ptype][i % REV_APS];
        tmp *= roomsize;
        if(i > REV_APS)
            tmp += 23.0f;
        tmp *= samplerate_adjust; //adjust the combs according to the samplerate
        if(tmp < 10)
            tmp = 10;
        aplen[i] = (int) tmp;
        apk[i]   = 0;
        if(ap[i])
            delete [] ap[i];
        ap[i] = new float[aplen[i]];
    }
    delete bandwidth;
    bandwidth = NULL;
    if(Ptype == 2) { //bandwidth
        //TODO the size of the unison buffer may be too small, though this has
        //not been verified yet.
        //As this cannot be resized in a RT context, a good upper bound should
        //be found
        bandwidth = new Unison(buffersize / 4 + 1, 2.0f, samplerate_f);
        bandwidth->setSize(50);
        bandwidth->setBaseFrequency(1.0f);
    }
    settime(Ptime);
    cleanup();
}

void Reverb::setroomsize(unsigned char _Proomsize)
{
    Proomsize = _Proomsize;
    if(!Proomsize)
        this->Proomsize = 64;  //this is because the older versions consider roomsize=0
    roomsize = (this->Proomsize - 64.0f) / 64.0f;
    if(roomsize > 0.0f)
        roomsize *= 2.0f;
    roomsize = powf(10.0f, roomsize);
    rs = sqrtf(roomsize);
    settype(Ptype);
}

void Reverb::setbandwidth(unsigned char _Pbandwidth)
{
    Pbandwidth = _Pbandwidth;
    float v = Pbandwidth / 127.0f;
    if(bandwidth)
        bandwidth->setBandwidth(powf(v, 2.0f) * 200.0f);
}

void Reverb::setpreset(unsigned char npreset)
{
    const int     PRESET_SIZE = 13;
    const int     NUM_PRESETS = 13;
    unsigned char presets[NUM_PRESETS][PRESET_SIZE] = {
        //Cathedral1
        {80,  64, 63,  24, 0,  0, 0, 85,  5,  83,  1, 64,  20},
        //Cathedral2
        {80,  64, 69,  35, 0,  0, 0, 127, 0,  71,  0, 64,  20},
        //Cathedral3
        {80,  64, 69,  24, 0,  0, 0, 127, 75, 78,  1, 85,  20},
        //Hall1
        {90,  64, 51,  10, 0,  0, 0, 127, 21, 78,  1, 64,  20},
        //Hall2
        {90,  64, 53,  20, 0,  0, 0, 127, 75, 71,  1, 64,  20},
        //Room1
        {100, 64, 33,  0,  0,  0, 0, 127, 0,  106, 0, 30,  20},
        //Room2
        {100, 64, 21,  26, 0,  0, 0, 62,  0,  77,  1, 45,  20},
        //Basement
        {110, 64, 14,  0,  0,  0, 0, 127, 5,  71,  0, 25,  20},
        //Tunnel
        {85,  80, 84,  20, 42, 0, 0, 51,  0,  78,  1, 105, 20},
        //Echoed1
        {95,  64, 26,  60, 71, 0, 0, 114, 0,  64,  1, 64,  20},
        //Echoed2
        {90,  64, 40,  88, 71, 0, 0, 114, 0,  88,  1, 64,  20},
        //VeryLong1
        {90,  64, 93,  15, 0,  0, 0, 114, 0,  77,  0, 95,  20},
        //VeryLong2
        {90,  64, 111, 30, 0,  0, 0, 114, 90, 74,  1, 80,  20}
    };

    if(npreset >= NUM_PRESETS)
        npreset = NUM_PRESETS - 1;
    for(int n = 0; n < PRESET_SIZE; ++n)
        changepar(n, presets[npreset][n]);
    if(insertion)
        changepar(0, presets[npreset][0] / 2);  //lower the volume if reverb is insertion effect
    Ppreset = npreset;
}


void Reverb::changepar(int npar, unsigned char value)
{
    switch(npar) {
        case 0:
            setvolume(value);
            break;
        case 1:
            setpanning(value);
            break;
        case 2:
            settime(value);
            break;
        case 3:
            setidelay(value);
            break;
        case 4:
            setidelayfb(value);
            break;
//      case 5:
//          setrdelay(value);
//          break;
//      case 6:
//          seterbalance(value);
//          break;
        case 7:
            setlpf(value);
            break;
        case 8:
            sethpf(value);
            break;
        case 9:
            setlohidamp(value);
            break;
        case 10:
            settype(value);
            break;
        case 11:
            setroomsize(value);
            break;
        case 12:
            setbandwidth(value);
            break;
    }
}

unsigned char Reverb::getpar(int npar) const
{
    switch(npar) {
        case 0:  return Pvolume;
        case 1:  return Ppanning;
        case 2:  return Ptime;
        case 3:  return Pidelay;
        case 4:  return Pidelayfb;
        case 7:  return Plpf;
        case 8:  return Phpf;
        case 9:  return Plohidamp;
        case 10: return Ptype;
        case 11: return Proomsize;
        case 12: return Pbandwidth;
        default: return 0;
    }
}
