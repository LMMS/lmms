/*
  ZynAddSubFX - a software synthesizer

  SUBnote.cpp - The "subtractive" synthesizer
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
#include <cstdio>
#include <cassert>
#include "../globals.h"
#include "SUBnote.h"
#include "../Misc/Util.h"

SUBnote::SUBnote(SUBnoteParameters *parameters,
                 Controller *ctl_,
                 float freq,
                 float velocity,
                 int portamento_,
                 int midinote,
                 bool besilent)
    :SynthNote(freq, velocity, portamento_, midinote, besilent)
{
    pars = parameters;
    ctl  = ctl_;
    NoteEnabled = ON;
    setup(freq, velocity, portamento_, midinote);
}

void SUBnote::setup(float freq,
                    float velocity,
                    int portamento_,
                    int midinote,
                    bool legato)
{
    portamento  = portamento_;
    NoteEnabled = ON;
    volume      = powf(0.1f, 3.0f * (1.0f - pars->PVolume / 96.0f)); //-60 dB .. 0 dB
    volume     *= VelF(velocity, pars->PAmpVelocityScaleFunction);
    if(pars->PPanning != 0)
        panning = pars->PPanning / 127.0f;
    else
        panning = RND;
    if(!legato) {
        numstages = pars->Pnumstages;
        stereo    = pars->Pstereo;
        start     = pars->Pstart;
        firsttick = 1;
    }
    int pos[MAX_SUB_HARMONICS];

    if(pars->Pfixedfreq == 0)
        basefreq = freq;
    else {
        basefreq = 440.0f;
        int fixedfreqET = pars->PfixedfreqET;
        if(fixedfreqET != 0) { //if the frequency varies according the keyboard note
            float tmp =
                (midinote
                 - 69.0f) / 12.0f
                * (powf(2.0f, (fixedfreqET - 1) / 63.0f) - 1.0f);
            if(fixedfreqET <= 64)
                basefreq *= powf(2.0f, tmp);
            else
                basefreq *= powf(3.0f, tmp);
        }
    }
    float detune = getdetune(pars->PDetuneType,
                             pars->PCoarseDetune,
                             pars->PDetune);
    basefreq *= powf(2.0f, detune / 1200.0f); //detune
//    basefreq*=ctl->pitchwheel.relfreq;//pitch wheel

    //global filter
    GlobalFilterCenterPitch = pars->GlobalFilter->getfreq() //center freq
                              + (pars->PGlobalFilterVelocityScale / 127.0f
                                 * 6.0f)                                           //velocity sensing
                              * (VelF(velocity,
                                      pars->PGlobalFilterVelocityScaleFunction)
                                 - 1);

    if(!legato) {
        GlobalFilterL = NULL;
        GlobalFilterR = NULL;
        GlobalFilterEnvelope = NULL;
    }

    int harmonics = 0;

    //select only harmonics that desire to compute
    for(int n = 0; n < MAX_SUB_HARMONICS; ++n) {
        if(pars->Phmag[n] == 0)
            continue;
        pos[harmonics++] = n;
    }
    if(!legato)
        firstnumharmonics = numharmonics = harmonics;
    else {
        if(harmonics > firstnumharmonics)
            numharmonics = firstnumharmonics;
        else
            numharmonics = harmonics;
    }


    if(numharmonics == 0) {
        NoteEnabled = OFF;
        return;
    }


    if(!legato) {
        lfilter = new bpfilter[numstages * numharmonics];
        if(stereo != 0)
            rfilter = new bpfilter[numstages * numharmonics];
    }

    //how much the amplitude is normalised (because the harmonics)
    float reduceamp = 0.0f;

    for(int n = 0; n < numharmonics; ++n) {
        float freq =  basefreq * pars->POvertoneFreqMult[pos[n]];
        overtone_freq[n] = freq;
        overtone_rolloff[n] = computerolloff(freq);

        //the bandwidth is not absolute(Hz); it is relative to frequency
        float bw =
            powf(10, (pars->Pbandwidth - 127.0f) / 127.0f * 4) * numstages;

        //Bandwidth Scale
        bw *= powf(1000 / freq, (pars->Pbwscale - 64.0f) / 64.0f * 3.0f);

        //Relative BandWidth
        bw *= powf(100, (pars->Phrelbw[pos[n]] - 64.0f) / 64.0f);

        if(bw > 25.0f)
            bw = 25.0f;

        //try to keep same amplitude on all freqs and bw. (empirically)
        float gain = sqrt(1500.0f / (bw * freq));

        float hmagnew = 1.0f - pars->Phmag[pos[n]] / 127.0f;
        float hgain;

        switch(pars->Phmagtype) {
            case 1:
                hgain = expf(hmagnew * logf(0.01f));
                break;
            case 2:
                hgain = expf(hmagnew * logf(0.001f));
                break;
            case 3:
                hgain = expf(hmagnew * logf(0.0001f));
                break;
            case 4:
                hgain = expf(hmagnew * logf(0.00001f));
                break;
            default:
                hgain = 1.0f - hmagnew;
        }
        gain      *= hgain;
        reduceamp += hgain;

        for(int nph = 0; nph < numstages; ++nph) {
            float amp = 1.0f;
            if(nph == 0)
                amp = gain;
            initfilter(lfilter[nph + n * numstages], freq, bw, amp, hgain);
            if(stereo != 0)
                initfilter(rfilter[nph + n * numstages], freq, bw, amp, hgain);
        }
    }

    if(reduceamp < 0.001f)
        reduceamp = 1.0f;
    volume /= reduceamp;

    oldpitchwheel = 0;
    oldbandwidth  = 64;
    if(!legato) {
        if(pars->Pfixedfreq == 0)
            initparameters(basefreq);
        else
            initparameters(basefreq / 440.0f * freq);
    }
    else {
        if(pars->Pfixedfreq == 0)
            freq = basefreq;
        else
            freq *= basefreq / 440.0f;

        if(pars->PGlobalFilterEnabled != 0) {
            globalfiltercenterq      = pars->GlobalFilter->getq();
            GlobalFilterFreqTracking = pars->GlobalFilter->getfreqtracking(
                basefreq);
        }
    }

    oldamplitude = newamplitude;
}

void SUBnote::legatonote(float freq, float velocity, int portamento_,
                         int midinote, bool externcall)
{
    // Manage legato stuff
    if(legato.update(freq, velocity, portamento_, midinote, externcall))
        return;

    setup(freq, velocity, portamento_, midinote, true);
}

SUBnote::~SUBnote()
{
    if(NoteEnabled != OFF)
        KillNote();
}

/*
 * Kill the note
 */
void SUBnote::KillNote()
{
    if(NoteEnabled != OFF) {
        delete [] lfilter;
        lfilter = NULL;
        if(stereo != 0)
            delete [] rfilter;
        rfilter = NULL;
        delete AmpEnvelope;
        delete FreqEnvelope;
        delete BandWidthEnvelope;
        delete GlobalFilterL;
        delete GlobalFilterR;
        delete GlobalFilterEnvelope;
        NoteEnabled = OFF;
    }
}


/*
 * Compute the filters coefficients
 */
void SUBnote::computefiltercoefs(bpfilter &filter,
                                 float freq,
                                 float bw,
                                 float gain)
{
    if(freq > synth->samplerate_f / 2.0f - 200.0f)
        freq = synth->samplerate_f / 2.0f - 200.0f;


    float omega = 2.0f * PI * freq / synth->samplerate_f;
    float sn    = sinf(omega);
    float cs    = cosf(omega);
    float alpha = sn * sinh(LOG_2 / 2.0f * bw * omega / sn);

    if(alpha > 1)
        alpha = 1;
    if(alpha > bw)
        alpha = bw;

    filter.b0 = alpha / (1.0f + alpha) * filter.amp * gain;
    filter.b2 = -alpha / (1.0f + alpha) * filter.amp * gain;
    filter.a1 = -2.0f * cs / (1.0f + alpha);
    filter.a2 = (1.0f - alpha) / (1.0f + alpha);
}


/*
 * Initialise the filters
 */
void SUBnote::initfilter(bpfilter &filter,
                         float freq,
                         float bw,
                         float amp,
                         float mag)
{
    filter.xn1 = 0.0f;
    filter.xn2 = 0.0f;

    if(start == 0) {
        filter.yn1 = 0.0f;
        filter.yn2 = 0.0f;
    }
    else {
        float a = 0.1f * mag; //empirically
        float p = RND * 2.0f * PI;
        if(start == 1)
            a *= RND;
        filter.yn1 = a * cosf(p);
        filter.yn2 = a * cosf(p + freq * 2.0f * PI / synth->samplerate_f);

        //correct the error of computation the start amplitude
        //at very high frequencies
        if(freq > synth->samplerate_f * 0.96f) {
            filter.yn1 = 0.0f;
            filter.yn2 = 0.0f;
        }
    }

    filter.amp  = amp;
    filter.freq = freq;
    filter.bw   = bw;
    computefiltercoefs(filter, freq, bw, 1.0f);
}

/*
 * Do the filtering
 */

inline void SubFilterA(const float coeff[4], float &src, float work[4])
{
    work[3] = src*coeff[0]+work[1]*coeff[1]+work[2]*coeff[2]+work[3]*coeff[3];
    work[1] = src;
    src     = work[3];
}

inline void SubFilterB(const float coeff[4], float &src, float work[4])
{
    work[2] = src*coeff[0]+work[0]*coeff[1]+work[3]*coeff[2]+work[2]*coeff[3];
    work[0] = src;
    src     = work[2];
}

//This dance is designed to minimize unneeded memory operations which can result
//in quite a bit of wasted time
void SUBnote::filter(bpfilter &filter, float *smps)
{
    assert(synth->buffersize % 8 == 0);
    float coeff[4] = {filter.b0, filter.b2,  -filter.a1, -filter.a2};
    float work[4]  = {filter.xn1, filter.xn2, filter.yn1, filter.yn2};

    for(int i = 0; i < synth->buffersize; i += 8) {
        SubFilterA(coeff, smps[i + 0], work);
        SubFilterB(coeff, smps[i + 1], work);
        SubFilterA(coeff, smps[i + 2], work);
        SubFilterB(coeff, smps[i + 3], work);
        SubFilterA(coeff, smps[i + 4], work);
        SubFilterB(coeff, smps[i + 5], work);
        SubFilterA(coeff, smps[i + 6], work);
        SubFilterB(coeff, smps[i + 7], work);
    }
    filter.xn1 = work[0];
    filter.xn2 = work[1];
    filter.yn1 = work[2];
    filter.yn2 = work[3];
}

/*
 * Init Parameters
 */
void SUBnote::initparameters(float freq)
{
    AmpEnvelope = new Envelope(pars->AmpEnvelope, freq);
    if(pars->PFreqEnvelopeEnabled != 0)
        FreqEnvelope = new Envelope(pars->FreqEnvelope, freq);
    else
        FreqEnvelope = NULL;
    if(pars->PBandWidthEnvelopeEnabled != 0)
        BandWidthEnvelope = new Envelope(pars->BandWidthEnvelope, freq);
    else
        BandWidthEnvelope = NULL;
    if(pars->PGlobalFilterEnabled != 0) {
        globalfiltercenterq = pars->GlobalFilter->getq();
        GlobalFilterL = Filter::generate(pars->GlobalFilter);
        if(stereo)
            GlobalFilterR = Filter::generate(pars->GlobalFilter);
        GlobalFilterEnvelope = new Envelope(pars->GlobalFilterEnvelope,
                                            freq);
        GlobalFilterFreqTracking = pars->GlobalFilter->getfreqtracking(basefreq);
    }
    computecurrentparameters();
}

/*
 * Compute how much to reduce amplitude near nyquist or subaudible frequencies.
 */
float SUBnote::computerolloff(float freq)
{
    const float lower_limit = 10.0f;
    const float lower_width = 10.0f;
    const float upper_width = 200.0f;
    float upper_limit = synth->samplerate / 2.0f;

    if (freq > lower_limit + lower_width &&
            freq < upper_limit - upper_width)
        return 1.0f;
    if (freq <= lower_limit || freq >= upper_limit)
        return 0.0f;
    if (freq <= lower_limit + lower_width)
        return (1.0f - cosf(M_PI * (freq - lower_limit) / lower_width)) / 2.0f;
    return (1.0f - cosf(M_PI * (freq - upper_limit) / upper_width)) / 2.0f;
}

/*
 * Compute Parameters of SUBnote for each tick
 */
void SUBnote::computecurrentparameters()
{
    if((FreqEnvelope != NULL) || (BandWidthEnvelope != NULL)
       || (oldpitchwheel != ctl->pitchwheel.data)
       || (oldbandwidth != ctl->bandwidth.data)
       || (portamento != 0)) {
        float envfreq = 1.0f;
        float envbw   = 1.0f;
        float gain    = 1.0f;

        if(FreqEnvelope != NULL) {
            envfreq = FreqEnvelope->envout() / 1200;
            envfreq = powf(2.0f, envfreq);
        }
        envfreq *= ctl->pitchwheel.relfreq; //pitch wheel
        if(portamento != 0) { //portamento is used
            envfreq *= ctl->portamento.freqrap;
            if(ctl->portamento.used == 0) //the portamento has finished
                portamento = 0;  //this note is no longer "portamented"
            ;
        }

        if(BandWidthEnvelope != NULL) {
            envbw = BandWidthEnvelope->envout();
            envbw = powf(2, envbw);
        }
        envbw *= ctl->bandwidth.relbw; //bandwidth controller

        float tmpgain = 1.0f / sqrt(envbw * envfreq);

        for(int n = 0; n < numharmonics; ++n) {
            overtone_rolloff[n] = computerolloff(overtone_freq[n] * envfreq);
        }
        for(int n = 0; n < numharmonics; ++n)
            for(int nph = 0; nph < numstages; ++nph) {
                if(nph == 0)
                    gain = tmpgain;
                else
                    gain = 1.0f;
                computefiltercoefs(lfilter[nph + n * numstages],
                                   lfilter[nph + n * numstages].freq * envfreq,
                                   lfilter[nph + n * numstages].bw * envbw,
                                   gain);
            }
        if(stereo != 0)
            for(int n = 0; n < numharmonics; ++n)
                for(int nph = 0; nph < numstages; ++nph) {
                    if(nph == 0)
                        gain = tmpgain;
                    else
                        gain = 1.0f;
                    computefiltercoefs(
                        rfilter[nph + n * numstages],
                        rfilter[nph + n
                                * numstages].freq * envfreq,
                        rfilter[nph + n * numstages].bw * envbw,
                        gain);
                }


        oldbandwidth  = ctl->bandwidth.data;
        oldpitchwheel = ctl->pitchwheel.data;
    }
    newamplitude = volume * AmpEnvelope->envout_dB() * 2.0f;

    //Filter
    if(GlobalFilterL != NULL) {
        float globalfilterpitch = GlobalFilterCenterPitch
                                  + GlobalFilterEnvelope->envout();
        float filterfreq = globalfilterpitch + ctl->filtercutoff.relfreq
                           + GlobalFilterFreqTracking;
        filterfreq = Filter::getrealfreq(filterfreq);

        GlobalFilterL->setfreq_and_q(filterfreq,
                                     globalfiltercenterq * ctl->filterq.relq);
        if(GlobalFilterR != NULL)
            GlobalFilterR->setfreq_and_q(
                filterfreq,
                globalfiltercenterq
                * ctl->filterq.relq);
    }
}

/*
 * Note Output
 */
int SUBnote::noteout(float *outl, float *outr)
{
    memcpy(outl, denormalkillbuf, synth->bufferbytes);
    memcpy(outr, denormalkillbuf, synth->bufferbytes);

    if(NoteEnabled == OFF)
        return 0;

    float tmprnd[synth->buffersize];
    float tmpsmp[synth->buffersize];
    //left channel
    for(int i = 0; i < synth->buffersize; ++i)
        tmprnd[i] = RND * 2.0f - 1.0f;
    for(int n = 0; n < numharmonics; ++n) {
        float rolloff = overtone_rolloff[n];
        memcpy(tmpsmp, tmprnd, synth->bufferbytes);
        for(int nph = 0; nph < numstages; ++nph)
            filter(lfilter[nph + n * numstages], tmpsmp);
        for(int i = 0; i < synth->buffersize; ++i)
            outl[i] += tmpsmp[i] * rolloff;
    }

    if(GlobalFilterL != NULL)
        GlobalFilterL->filterout(&outl[0]);

    //right channel
    if(stereo != 0) {
        for(int i = 0; i < synth->buffersize; ++i)
            tmprnd[i] = RND * 2.0f - 1.0f;
        for(int n = 0; n < numharmonics; ++n) {
            float rolloff = overtone_rolloff[n];
            memcpy(tmpsmp, tmprnd, synth->bufferbytes);
            for(int nph = 0; nph < numstages; ++nph)
                filter(rfilter[nph + n * numstages], tmpsmp);
            for(int i = 0; i < synth->buffersize; ++i)
                outr[i] += tmpsmp[i] * rolloff;
        }
        if(GlobalFilterR != NULL)
            GlobalFilterR->filterout(&outr[0]);
    }
    else
        memcpy(outr, outl, synth->bufferbytes);

    if(firsttick != 0) {
        int n = 10;
        if(n > synth->buffersize)
            n = synth->buffersize;
        for(int i = 0; i < n; ++i) {
            float ampfadein = 0.5f - 0.5f * cosf(
                (float) i / (float) n * PI);
            outl[i] *= ampfadein;
            outr[i] *= ampfadein;
        }
        firsttick = 0;
    }

    if(ABOVE_AMPLITUDE_THRESHOLD(oldamplitude, newamplitude))
        // Amplitude interpolation
        for(int i = 0; i < synth->buffersize; ++i) {
            float tmpvol = INTERPOLATE_AMPLITUDE(oldamplitude,
                                                 newamplitude,
                                                 i,
                                                 synth->buffersize);
            outl[i] *= tmpvol * panning;
            outr[i] *= tmpvol * (1.0f - panning);
        }
    else
        for(int i = 0; i < synth->buffersize; ++i) {
            outl[i] *= newamplitude * panning;
            outr[i] *= newamplitude * (1.0f - panning);
        }

    oldamplitude = newamplitude;
    computecurrentparameters();

    // Apply legato-specific sound signal modifications
    legato.apply(*this, outl, outr);

    // Check if the note needs to be computed more
    if(AmpEnvelope->finished() != 0) {
        for(int i = 0; i < synth->buffersize; ++i) { //fade-out
            float tmp = 1.0f - (float)i / synth->buffersize_f;
            outl[i] *= tmp;
            outr[i] *= tmp;
        }
        KillNote();
    }
    return 1;
}

/*
 * Relase Key (Note Off)
 */
void SUBnote::relasekey()
{
    AmpEnvelope->relasekey();
    if(FreqEnvelope)
        FreqEnvelope->relasekey();
    if(BandWidthEnvelope)
        BandWidthEnvelope->relasekey();
    if(GlobalFilterEnvelope)
        GlobalFilterEnvelope->relasekey();
}

/*
 * Check if the note is finished
 */
int SUBnote::finished() const
{
    if(NoteEnabled == OFF)
        return 1;
    else
        return 0;
}
