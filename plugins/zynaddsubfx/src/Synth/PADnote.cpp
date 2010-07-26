/*
  ZynAddSubFX - a software synthesizer

  PADnote.cpp - The "pad" synthesizer
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
#include <math.h>
#include "PADnote.h"
#include "../Misc/Config.h"

PADnote::PADnote(PADnoteParameters *parameters,
                 Controller *ctl_,
                 REALTYPE freq,
                 REALTYPE velocity,
                 int portamento_,
                 int midinote,
                 bool besilent)
{
    ready = 0;

    // Initialise some legato-specific vars
    Legato.msg = LM_Norm;
    Legato.fade.length      = (int)(SAMPLE_RATE * 0.005); // 0.005 seems ok.
    if(Legato.fade.length < 1)
        Legato.fade.length = 1;                    // (if something's fishy)
    Legato.fade.step        = (1.0 / Legato.fade.length);
    Legato.decounter        = -10;
    Legato.param.freq       = freq;
    Legato.param.vel        = velocity;
    Legato.param.portamento = portamento_;
    Legato.param.midinote   = midinote;
    Legato.silent  = besilent;

    pars           = parameters;
    portamento     = portamento_;
    ctl            = ctl_;
    this->velocity = velocity;
    finished_      = false;


    if(pars->Pfixedfreq == 0)
        basefreq = freq;
    else {
        basefreq = 440.0;
        int fixedfreqET = pars->PfixedfreqET;
        if(fixedfreqET != 0) { //if the frequency varies according the keyboard note
            REALTYPE tmp =
                (midinote
                 - 69.0) / 12.0 * (pow(2.0, (fixedfreqET - 1) / 63.0) - 1.0);
            if(fixedfreqET <= 64)
                basefreq *= pow(2.0, tmp);
            else
                basefreq *= pow(3.0, tmp);
        }
    }

    firsttime = true;
    released  = false;
    realfreq  = basefreq;
    NoteGlobalPar.Detune = getdetune(pars->PDetuneType,
                                     pars->PCoarseDetune, pars->PDetune);


    //find out the closest note
    REALTYPE logfreq = log(basefreq * pow(2.0, NoteGlobalPar.Detune / 1200.0));
    REALTYPE mindist = fabs(logfreq - log(pars->sample[0].basefreq + 0.0001));
    nsample = 0;
    for(int i = 1; i < PAD_MAX_SAMPLES; i++) {
        if(pars->sample[i].smp == NULL)
            break;
        REALTYPE dist = fabs(logfreq - log(pars->sample[i].basefreq + 0.0001));
//	printf("(mindist=%g) %i %g                  %g\n",mindist,i,dist,pars->sample[i].basefreq);

        if(dist < mindist) {
            nsample = i;
            mindist = dist;
        }
    }

    int size = pars->sample[nsample].size;
    if(size == 0)
        size = 1;


    poshi_l = (int)(RND * (size - 1));
    if(pars->PStereo != 0)
        poshi_r = (poshi_l + size / 2) % size;
    else
        poshi_r = poshi_l;
    poslo   = 0.0;

    tmpwave = new REALTYPE [SOUND_BUFFER_SIZE];



    if(pars->PPanning == 0)
        NoteGlobalPar.Panning = RND;
    else
        NoteGlobalPar.Panning = pars->PPanning / 128.0;

    NoteGlobalPar.FilterCenterPitch = pars->GlobalFilter->getfreq() //center freq
                                      + pars->PFilterVelocityScale / 127.0
                                      * 6.0                                //velocity sensing
                                      * (VelF(velocity,
                                              pars->
                                              PFilterVelocityScaleFunction) - 1);

    if(pars->PPunchStrength != 0) {
        NoteGlobalPar.Punch.Enabled      = 1;
        NoteGlobalPar.Punch.t = 1.0; //start from 1.0 and to 0.0
        NoteGlobalPar.Punch.initialvalue =
            ((pow(10, 1.5 * pars->PPunchStrength / 127.0) - 1.0)
             * VelF(velocity,
                    pars->PPunchVelocitySensing));
        REALTYPE time    = pow(10, 3.0 * pars->PPunchTime / 127.0) / 10000.0; //0.1 .. 100 ms
        REALTYPE stretch = pow(440.0 / freq, pars->PPunchStretch / 64.0);
        NoteGlobalPar.Punch.dt = 1.0 / (time * SAMPLE_RATE * stretch);
    }
    else
        NoteGlobalPar.Punch.Enabled = 0;



    NoteGlobalPar.FreqEnvelope = new Envelope(pars->FreqEnvelope, basefreq);
    NoteGlobalPar.FreqLfo      = new LFO(pars->FreqLfo, basefreq);

    NoteGlobalPar.AmpEnvelope  = new Envelope(pars->AmpEnvelope, basefreq);
    NoteGlobalPar.AmpLfo = new LFO(pars->AmpLfo, basefreq);

    NoteGlobalPar.Volume = 4.0 * pow(0.1, 3.0 * (1.0 - pars->PVolume / 96.0)) //-60 dB .. 0 dB
                           * VelF(velocity, pars->PAmpVelocityScaleFunction); //velocity sensing

    NoteGlobalPar.AmpEnvelope->envout_dB(); //discard the first envelope output
    globaloldamplitude = globalnewamplitude = NoteGlobalPar.Volume
                                              * NoteGlobalPar.AmpEnvelope->
                                              envout_dB()
                                              * NoteGlobalPar.AmpLfo->amplfoout();

    NoteGlobalPar.GlobalFilterL      = new Filter(pars->GlobalFilter);
    NoteGlobalPar.GlobalFilterR      = new Filter(pars->GlobalFilter);

    NoteGlobalPar.FilterEnvelope     = new Envelope(pars->FilterEnvelope,
                                                    basefreq);
    NoteGlobalPar.FilterLfo          = new LFO(pars->FilterLfo, basefreq);
    NoteGlobalPar.FilterQ = pars->GlobalFilter->getq();
    NoteGlobalPar.FilterFreqTracking = pars->GlobalFilter->getfreqtracking(
        basefreq);

    ready = 1; ///sa il pun pe asta doar cand e chiar gata

    if(parameters->sample[nsample].smp == NULL) {
        finished_ = true;
        return;
    }
}


// PADlegatonote: This function is (mostly) a copy of PADnote(...)
// with some lines removed so that it only alter the already playing
// note (to perform legato). It is possible I left stuff that is not
// required for this.
void PADnote::PADlegatonote(REALTYPE freq,
                            REALTYPE velocity,
                            int portamento_,
                            int midinote,
                            bool externcall)
{
    PADnoteParameters *parameters = pars;
    //Controller *ctl_=ctl;

    // Manage legato stuff
    if(externcall)
        Legato.msg = LM_Norm;
    if(Legato.msg != LM_CatchUp) {
        Legato.lastfreq   = Legato.param.freq;
        Legato.param.freq = freq;
        Legato.param.vel  = velocity;
        Legato.param.portamento = portamento_;
        Legato.param.midinote   = midinote;
        if(Legato.msg == LM_Norm) {
            if(Legato.silent) {
                Legato.fade.m = 0.0;
                Legato.msg    = LM_FadeIn;
            }
            else {
                Legato.fade.m = 1.0;
                Legato.msg    = LM_FadeOut;
                return;
            }
        }
        if(Legato.msg == LM_ToNorm)
            Legato.msg = LM_Norm;
    }

    portamento     = portamento_;
    this->velocity = velocity;
    finished_      = false;

    if(pars->Pfixedfreq == 0)
        basefreq = freq;
    else {
        basefreq = 440.0;
        int fixedfreqET = pars->PfixedfreqET;
        if(fixedfreqET != 0) { //if the frequency varies according the keyboard note
            REALTYPE tmp =
                (midinote
                 - 69.0) / 12.0 * (pow(2.0, (fixedfreqET - 1) / 63.0) - 1.0);
            if(fixedfreqET <= 64)
                basefreq *= pow(2.0, tmp);
            else
                basefreq *= pow(3.0, tmp);
        }
    }

    released = false;
    realfreq = basefreq;

    getdetune(pars->PDetuneType, pars->PCoarseDetune, pars->PDetune);


    //find out the closest note
    REALTYPE logfreq = log(basefreq * pow(2.0, NoteGlobalPar.Detune / 1200.0));
    REALTYPE mindist = fabs(logfreq - log(pars->sample[0].basefreq + 0.0001));
    nsample = 0;
    for(int i = 1; i < PAD_MAX_SAMPLES; i++) {
        if(pars->sample[i].smp == NULL)
            break;
        REALTYPE dist = fabs(logfreq - log(pars->sample[i].basefreq + 0.0001));

        if(dist < mindist) {
            nsample = i;
            mindist = dist;
        }
    }

    int size = pars->sample[nsample].size;
    if(size == 0)
        size = 1;

    if(pars->PPanning == 0)
        NoteGlobalPar.Panning = RND;
    else
        NoteGlobalPar.Panning = pars->PPanning / 128.0;

    NoteGlobalPar.FilterCenterPitch = pars->GlobalFilter->getfreq() //center freq
                                      + pars->PFilterVelocityScale / 127.0
                                      * 6.0                                //velocity sensing
                                      * (VelF(velocity,
                                              pars->
                                              PFilterVelocityScaleFunction) - 1);


    NoteGlobalPar.Volume = 4.0 * pow(0.1, 3.0 * (1.0 - pars->PVolume / 96.0)) //-60 dB .. 0 dB
                           * VelF(velocity, pars->PAmpVelocityScaleFunction); //velocity sensing

    NoteGlobalPar.AmpEnvelope->envout_dB(); //discard the first envelope output
    globaloldamplitude = globalnewamplitude = NoteGlobalPar.Volume
                                              * NoteGlobalPar.AmpEnvelope->
                                              envout_dB()
                                              * NoteGlobalPar.AmpLfo->amplfoout();

    NoteGlobalPar.FilterQ = pars->GlobalFilter->getq();
    NoteGlobalPar.FilterFreqTracking = pars->GlobalFilter->getfreqtracking(
        basefreq);


    if(parameters->sample[nsample].smp == NULL) {
        finished_ = true;
        return;
    }

    // End of the PADlegatonote function.
}


PADnote::~PADnote()
{
    delete (NoteGlobalPar.FreqEnvelope);
    delete (NoteGlobalPar.FreqLfo);
    delete (NoteGlobalPar.AmpEnvelope);
    delete (NoteGlobalPar.AmpLfo);
    delete (NoteGlobalPar.GlobalFilterL);
    delete (NoteGlobalPar.GlobalFilterR);
    delete (NoteGlobalPar.FilterEnvelope);
    delete (NoteGlobalPar.FilterLfo);
    delete [] tmpwave;
}


inline void PADnote::fadein(REALTYPE *smps)
{
    int zerocrossings = 0;
    for(int i = 1; i < SOUND_BUFFER_SIZE; i++)
        if((smps[i - 1] < 0.0) && (smps[i] > 0.0))
            zerocrossings++;                                  //this is only the possitive crossings

    REALTYPE tmp = (SOUND_BUFFER_SIZE - 1.0) / (zerocrossings + 1) / 3.0;
    if(tmp < 8.0)
        tmp = 8.0;

    int n;
    F2I(tmp, n); //how many samples is the fade-in
    if(n > SOUND_BUFFER_SIZE)
        n = SOUND_BUFFER_SIZE;
    for(int i = 0; i < n; i++) { //fade-in
        REALTYPE tmp = 0.5 - cos((REALTYPE)i / (REALTYPE) n * PI) * 0.5;
        smps[i] *= tmp;
    }
}


void PADnote::computecurrentparameters()
{
    REALTYPE globalpitch, globalfilterpitch;
    globalpitch = 0.01 * (NoteGlobalPar.FreqEnvelope->envout()
                          + NoteGlobalPar.FreqLfo->lfoout()
                          * ctl->modwheel.relmod + NoteGlobalPar.Detune);
    globaloldamplitude = globalnewamplitude;
    globalnewamplitude = NoteGlobalPar.Volume
                         * NoteGlobalPar.AmpEnvelope->envout_dB()
                         * NoteGlobalPar.AmpLfo->amplfoout();

    globalfilterpitch = NoteGlobalPar.FilterEnvelope->envout()
                        + NoteGlobalPar.FilterLfo->lfoout()
                        + NoteGlobalPar.FilterCenterPitch;

    REALTYPE tmpfilterfreq = globalfilterpitch + ctl->filtercutoff.relfreq
                             + NoteGlobalPar.FilterFreqTracking;

    tmpfilterfreq = NoteGlobalPar.GlobalFilterL->getrealfreq(tmpfilterfreq);

    REALTYPE globalfilterq = NoteGlobalPar.FilterQ * ctl->filterq.relq;
    NoteGlobalPar.GlobalFilterL->setfreq_and_q(tmpfilterfreq, globalfilterq);
    NoteGlobalPar.GlobalFilterR->setfreq_and_q(tmpfilterfreq, globalfilterq);

    //compute the portamento, if it is used by this note
    REALTYPE portamentofreqrap = 1.0;
    if(portamento != 0) { //this voice use portamento
        portamentofreqrap = ctl->portamento.freqrap;
        if(ctl->portamento.used == 0) //the portamento has finished
            portamento = 0; //this note is no longer "portamented"
        ;
    }

    realfreq = basefreq * portamentofreqrap
               * pow(2.0, globalpitch / 12.0) * ctl->pitchwheel.relfreq;
}


int PADnote::Compute_Linear(REALTYPE *outl,
                            REALTYPE *outr,
                            int freqhi,
                            REALTYPE freqlo)
{
    REALTYPE *smps = pars->sample[nsample].smp;
    if(smps == NULL) {
        finished_ = true;
        return 1;
    }
    int size = pars->sample[nsample].size;
    for(int i = 0; i < SOUND_BUFFER_SIZE; i++) {
        poshi_l += freqhi;
        poshi_r += freqhi;
        poslo   += freqlo;
        if(poslo >= 1.0) {
            poshi_l += 1;
            poshi_r += 1;
            poslo   -= 1.0;
        }
        if(poshi_l >= size)
            poshi_l %= size;
        if(poshi_r >= size)
            poshi_r %= size;

        outl[i] = smps[poshi_l] * (1.0 - poslo) + smps[poshi_l + 1] * poslo;
        outr[i] = smps[poshi_r] * (1.0 - poslo) + smps[poshi_r + 1] * poslo;
    }
    return 1;
}
int PADnote::Compute_Cubic(REALTYPE *outl,
                           REALTYPE *outr,
                           int freqhi,
                           REALTYPE freqlo)
{
    REALTYPE *smps = pars->sample[nsample].smp;
    if(smps == NULL) {
        finished_ = true;
        return 1;
    }
    int      size = pars->sample[nsample].size;
    REALTYPE xm1, x0, x1, x2, a, b, c;
    for(int i = 0; i < SOUND_BUFFER_SIZE; i++) {
        poshi_l += freqhi;
        poshi_r += freqhi;
        poslo   += freqlo;
        if(poslo >= 1.0) {
            poshi_l += 1;
            poshi_r += 1;
            poslo   -= 1.0;
        }
        if(poshi_l >= size)
            poshi_l %= size;
        if(poshi_r >= size)
            poshi_r %= size;


        //left
        xm1     = smps[poshi_l];
        x0      = smps[poshi_l + 1];
        x1      = smps[poshi_l + 2];
        x2      = smps[poshi_l + 3];
        a       = (3.0 * (x0 - x1) - xm1 + x2) * 0.5;
        b       = 2.0 * x1 + xm1 - (5.0 * x0 + x2) * 0.5;
        c       = (x1 - xm1) * 0.5;
        outl[i] = (((a * poslo) + b) * poslo + c) * poslo + x0;
        //right
        xm1     = smps[poshi_r];
        x0      = smps[poshi_r + 1];
        x1      = smps[poshi_r + 2];
        x2      = smps[poshi_r + 3];
        a       = (3.0 * (x0 - x1) - xm1 + x2) * 0.5;
        b       = 2.0 * x1 + xm1 - (5.0 * x0 + x2) * 0.5;
        c       = (x1 - xm1) * 0.5;
        outr[i] = (((a * poslo) + b) * poslo + c) * poslo + x0;
    }
    return 1;
}


int PADnote::noteout(REALTYPE *outl, REALTYPE *outr)
{
    computecurrentparameters();
    REALTYPE *smps = pars->sample[nsample].smp;
    if(smps == NULL) {
        for(int i = 0; i < SOUND_BUFFER_SIZE; i++) {
            outl[i] = 0.0;
            outr[i] = 0.0;
        }
        return 1;
    }
    REALTYPE smpfreq = pars->sample[nsample].basefreq;


    REALTYPE freqrap = realfreq / smpfreq;
    int      freqhi  = (int) (floor(freqrap));
    REALTYPE freqlo  = freqrap - floor(freqrap);


    if(config.cfg.Interpolation)
        Compute_Cubic(outl, outr, freqhi, freqlo);
    else
        Compute_Linear(outl, outr, freqhi, freqlo);


    if(firsttime) {
        fadein(outl);
        fadein(outr);
        firsttime = false;
    }

    NoteGlobalPar.GlobalFilterL->filterout(outl);
    NoteGlobalPar.GlobalFilterR->filterout(outr);

    //Apply the punch
    if(NoteGlobalPar.Punch.Enabled != 0) {
        for(int i = 0; i < SOUND_BUFFER_SIZE; i++) {
            REALTYPE punchamp = NoteGlobalPar.Punch.initialvalue
                                * NoteGlobalPar.Punch.t + 1.0;
            outl[i] *= punchamp;
            outr[i] *= punchamp;
            NoteGlobalPar.Punch.t -= NoteGlobalPar.Punch.dt;
            if(NoteGlobalPar.Punch.t < 0.0) {
                NoteGlobalPar.Punch.Enabled = 0;
                break;
            }
        }
    }

    if(ABOVE_AMPLITUDE_THRESHOLD(globaloldamplitude, globalnewamplitude)) {
        // Amplitude Interpolation
        for(int i = 0; i < SOUND_BUFFER_SIZE; i++) {
            REALTYPE tmpvol = INTERPOLATE_AMPLITUDE(globaloldamplitude,
                                                    globalnewamplitude,
                                                    i,
                                                    SOUND_BUFFER_SIZE);
            outl[i] *= tmpvol * NoteGlobalPar.Panning;
            outr[i] *= tmpvol * (1.0 - NoteGlobalPar.Panning);
        }
    }
    else {
        for(int i = 0; i < SOUND_BUFFER_SIZE; i++) {
            outl[i] *= globalnewamplitude * NoteGlobalPar.Panning;
            outr[i] *= globalnewamplitude * (1.0 - NoteGlobalPar.Panning);
        }
    }


    // Apply legato-specific sound signal modifications
    if(Legato.silent)    // Silencer
        if(Legato.msg != LM_FadeIn)
            for(int i = 0; i < SOUND_BUFFER_SIZE; i++) {
                outl[i] = 0.0;
                outr[i] = 0.0;
            }
    switch(Legato.msg) {
    case LM_CatchUp:  // Continue the catch-up...
        if(Legato.decounter == -10)
            Legato.decounter = Legato.fade.length;
        for(int i = 0; i < SOUND_BUFFER_SIZE; i++) { //Yea, could be done without the loop...
            Legato.decounter--;
            if(Legato.decounter < 1) {
                // Catching-up done, we can finally set
                // the note to the actual parameters.
                Legato.decounter = -10;
                Legato.msg = LM_ToNorm;
                PADlegatonote(Legato.param.freq,
                              Legato.param.vel,
                              Legato.param.portamento,
                              Legato.param.midinote,
                              false);
                break;
            }
        }
        break;
    case LM_FadeIn:  // Fade-in
        if(Legato.decounter == -10)
            Legato.decounter = Legato.fade.length;
        Legato.silent = false;
        for(int i = 0; i < SOUND_BUFFER_SIZE; i++) {
            Legato.decounter--;
            if(Legato.decounter < 1) {
                Legato.decounter = -10;
                Legato.msg = LM_Norm;
                break;
            }
            Legato.fade.m += Legato.fade.step;
            outl[i] *= Legato.fade.m;
            outr[i] *= Legato.fade.m;
        }
        break;
    case LM_FadeOut:  // Fade-out, then set the catch-up
        if(Legato.decounter == -10)
            Legato.decounter = Legato.fade.length;
        for(int i = 0; i < SOUND_BUFFER_SIZE; i++) {
            Legato.decounter--;
            if(Legato.decounter < 1) {
                for(int j = i; j < SOUND_BUFFER_SIZE; j++) {
                    outl[j] = 0.0;
                    outr[j] = 0.0;
                }
                Legato.decounter = -10;
                Legato.silent    = true;
                // Fading-out done, now set the catch-up :
                Legato.decounter = Legato.fade.length;
                Legato.msg = LM_CatchUp;
                REALTYPE catchupfreq = Legato.param.freq
                                       * (Legato.param.freq / Legato.lastfreq);            //This freq should make this now silent note to catch-up (or should I say resync ?) with the heard note for the same length it stayed at the previous freq during the fadeout.
                PADlegatonote(catchupfreq,
                              Legato.param.vel,
                              Legato.param.portamento,
                              Legato.param.midinote,
                              false);
                break;
            }
            Legato.fade.m -= Legato.fade.step;
            outl[i] *= Legato.fade.m;
            outr[i] *= Legato.fade.m;
        }
        break;
    default:
        break;
    }


    // Check if the global amplitude is finished.
    // If it does, disable the note
    if(NoteGlobalPar.AmpEnvelope->finished() != 0) {
        for(int i = 0; i < SOUND_BUFFER_SIZE; i++) { //fade-out
            REALTYPE tmp = 1.0 - (REALTYPE)i / (REALTYPE)SOUND_BUFFER_SIZE;
            outl[i] *= tmp;
            outr[i] *= tmp;
        }
        finished_ = 1;
    }

    return 1;
}

int PADnote::finished()
{
    return finished_;
}

void PADnote::relasekey()
{
    NoteGlobalPar.FreqEnvelope->relasekey();
    NoteGlobalPar.FilterEnvelope->relasekey();
    NoteGlobalPar.AmpEnvelope->relasekey();
}

