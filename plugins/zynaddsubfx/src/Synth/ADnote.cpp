/*
  ZynAddSubFX - a software synthesizer

  ADnote.cpp - The "additive" synthesizer
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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../globals.h"
#include "../Misc/Util.h"
#include "ADnote.h"


ADnote::ADnote(ADnoteParameters *pars,
               Controller *ctl_,
               REALTYPE freq,
               REALTYPE velocity,
               int portamento_,
               int midinote_,
               bool besilent)
{
    ready    = 0;

    tmpwavel = new REALTYPE [SOUND_BUFFER_SIZE];
    tmpwaver = new REALTYPE [SOUND_BUFFER_SIZE];
    bypassl  = new REALTYPE [SOUND_BUFFER_SIZE];
    bypassr  = new REALTYPE [SOUND_BUFFER_SIZE];

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
    Legato.param.midinote   = midinote_;
    Legato.silent  = besilent;

    partparams     = pars;
    ctl = ctl_;
    portamento     = portamento_;
    midinote       = midinote_;
    NoteEnabled    = ON;
    basefreq       = freq;
    if(velocity > 1.0)
        velocity = 1.0;
    this->velocity = velocity;
    time   = 0.0;
    stereo = pars->GlobalPar.PStereo;

    NoteGlobalPar.Detune      = getdetune(pars->GlobalPar.PDetuneType,
                                          pars->GlobalPar.PCoarseDetune,
                                          pars->GlobalPar.PDetune);
    bandwidthDetuneMultiplier = pars->getBandwidthDetuneMultiplier();

    if(pars->GlobalPar.PPanning == 0)
        NoteGlobalPar.Panning = RND;
    else
        NoteGlobalPar.Panning = pars->GlobalPar.PPanning / 128.0;


    NoteGlobalPar.FilterCenterPitch = pars->GlobalPar.GlobalFilter->getfreq() //center freq
                                      + pars->GlobalPar.PFilterVelocityScale
                                      / 127.0 * 6.0                                  //velocity sensing
                                      * (VelF(velocity,
                                              pars->GlobalPar.
                                              PFilterVelocityScaleFunction) - 1);

    if(pars->GlobalPar.PPunchStrength != 0) {
        NoteGlobalPar.Punch.Enabled      = 1;
        NoteGlobalPar.Punch.t = 1.0; //start from 1.0 and to 0.0
        NoteGlobalPar.Punch.initialvalue =
            ((pow(10, 1.5 * pars->GlobalPar.PPunchStrength / 127.0) - 1.0)
             * VelF(velocity,
                    pars->GlobalPar.PPunchVelocitySensing));
        REALTYPE time    =
            pow(10, 3.0 * pars->GlobalPar.PPunchTime / 127.0) / 10000.0;   //0.1 .. 100 ms
        REALTYPE stretch = pow(440.0 / freq,
                               pars->GlobalPar.PPunchStretch / 64.0);
        NoteGlobalPar.Punch.dt = 1.0 / (time * SAMPLE_RATE * stretch);
    }
    else
        NoteGlobalPar.Punch.Enabled = 0;

    for(int nvoice = 0; nvoice < NUM_VOICES; nvoice++) {
        pars->VoicePar[nvoice].OscilSmp->newrandseed(rand());
        NoteVoicePar[nvoice].OscilSmp = NULL;
        NoteVoicePar[nvoice].FMSmp    = NULL;
        NoteVoicePar[nvoice].VoiceOut = NULL;

        NoteVoicePar[nvoice].FMVoice  = -1;
        unison_size[nvoice] = 1;

        if(pars->VoicePar[nvoice].Enabled == 0) {
            NoteVoicePar[nvoice].Enabled = OFF;
            continue; //the voice is disabled
        }

        unison_stereo_spread[nvoice] =
            pars->VoicePar[nvoice].Unison_stereo_spread / 127.0;
        int unison = pars->VoicePar[nvoice].Unison_size;
        if(unison < 1)
            unison = 1;

        //compute unison
        unison_size[nvoice] = unison;

        unison_base_freq_rap[nvoice] = new REALTYPE[unison];
        unison_freq_rap[nvoice]      = new REALTYPE[unison];
        unison_invert_phase[nvoice]  = new bool[unison];
        REALTYPE unison_spread      = pars->getUnisonFrequencySpreadCents(
            nvoice);
        REALTYPE unison_real_spread = pow(2.0, (unison_spread * 0.5) / 1200.0);
        REALTYPE unison_vibratto_a  = pars->VoicePar[nvoice].Unison_vibratto
                                      / 127.0;                                  //0.0 .. 1.0


        switch(unison) {
        case 1:
            unison_base_freq_rap[nvoice][0] = 1.0;  //if the unison is not used, always make the only subvoice to have the default note
            break;
        case 2: {   //unison for 2 subvoices
            unison_base_freq_rap[nvoice][0] = 1.0 / unison_real_spread;
            unison_base_freq_rap[nvoice][1] = unison_real_spread;
        };
            break;
        default: {   //unison for more than 2 subvoices
            REALTYPE unison_values[unison];
            REALTYPE min = -1e-6, max = 1e-6;
            for(int k = 0; k < unison; k++) {
                REALTYPE step = (k / (REALTYPE) (unison - 1)) * 2.0 - 1.0;  //this makes the unison spread more uniform
                REALTYPE val  = step + (RND * 2.0 - 1.0) / (unison - 1);
                unison_values[k] = val;
                if(val > max)
                    max = val;
                if(val < min)
                    min = val;
            }
            REALTYPE diff = max - min;
            for(int k = 0; k < unison; k++) {
                unison_values[k] =
                    (unison_values[k] - (max + min) * 0.5) / diff;                 //the lowest value will be -1 and the highest will be 1
                unison_base_freq_rap[nvoice][k] =
                    pow(2.0, (unison_spread * unison_values[k]) / 1200);
            }
        };
        }

        //unison vibrattos
        if(unison > 1) {
            for(int k = 0; k < unison; k++) //reduce the frequency difference for larger vibrattos
                unison_base_freq_rap[nvoice][k] = 1.0
                                                  + (unison_base_freq_rap[
                                                         nvoice][k]
                                                     - 1.0)
                                                  * (1.0 - unison_vibratto_a);
            ;
        }
        unison_vibratto[nvoice].step      = new REALTYPE[unison];
        unison_vibratto[nvoice].position  = new REALTYPE[unison];
        unison_vibratto[nvoice].amplitude =
            (unison_real_spread - 1.0) * unison_vibratto_a;

        REALTYPE increments_per_second = SAMPLE_RATE
                                         / (REALTYPE)SOUND_BUFFER_SIZE;
        REALTYPE vibratto_base_period  = 0.25
                                         * pow(2.0,
                                               (1.0
                                                - pars->VoicePar[nvoice].
                                                Unison_vibratto_speed / 127.0) * 4.0);
        for(int k = 0; k < unison; k++) {
            unison_vibratto[nvoice].position[k] = RND * 1.8 - 0.9;
            REALTYPE vibratto_period = vibratto_base_period * pow(
                2.0,
                RND * 2.0
                - 1.0);                                                        //make period to vary randomly from 50% to 200% vibratto base period

            REALTYPE m = 4.0 / (vibratto_period * increments_per_second);
            if(RND < 0.5)
                m = -m;
            unison_vibratto[nvoice].step[k] = m;
        }

        if(unison == 1) { //no vibratto for a single voice
            unison_vibratto[nvoice].step[0]     = 0.0;
            unison_vibratto[nvoice].position[0] = 0.0;
            unison_vibratto[nvoice].amplitude   = 0.0;
        }

        //phase invert for unison
        unison_invert_phase[nvoice][0] = false;
        if(unison != 1) {
            int inv = pars->VoicePar[nvoice].Unison_invert_phase;
            switch(inv) {
            case 0: for(int k = 0; k < unison; k++)
                    unison_invert_phase[nvoice][k] = false;
                break;
            case 1: for(int k = 0; k < unison; k++)
                    unison_invert_phase[nvoice][k] = (RND > 0.5);
                break;
            default: for(int k = 0; k < unison; k++)
                    unison_invert_phase[nvoice][k] =
                        (k % inv == 0) ? true : false;
                break;
            }
        }


        oscfreqhi[nvoice]   = new int[unison];
        oscfreqlo[nvoice]   = new REALTYPE[unison];
        oscfreqhiFM[nvoice] = new unsigned int[unison];
        oscfreqloFM[nvoice] = new REALTYPE[unison];
        oscposhi[nvoice]    = new int[unison];
        oscposlo[nvoice]    = new REALTYPE[unison];
        oscposhiFM[nvoice]  = new unsigned int[unison];
        oscposloFM[nvoice]  = new REALTYPE[unison];

        NoteVoicePar[nvoice].Enabled     = ON;
        NoteVoicePar[nvoice].fixedfreq   = pars->VoicePar[nvoice].Pfixedfreq;
        NoteVoicePar[nvoice].fixedfreqET = pars->VoicePar[nvoice].PfixedfreqET;

        //use the Globalpars.detunetype if the detunetype is 0
        if(pars->VoicePar[nvoice].PDetuneType != 0) {
            NoteVoicePar[nvoice].Detune     = getdetune(
                pars->VoicePar[nvoice].PDetuneType,
                pars->VoicePar[nvoice].
                PCoarseDetune,
                8192);                                                                        //coarse detune
            NoteVoicePar[nvoice].FineDetune = getdetune(
                pars->VoicePar[nvoice].PDetuneType,
                0,
                pars->VoicePar[nvoice].PDetune);                               //fine detune
        }
        else {
            NoteVoicePar[nvoice].Detune     = getdetune(
                pars->GlobalPar.PDetuneType,
                pars->VoicePar[nvoice].
                PCoarseDetune,
                8192);                                                                        //coarse detune
            NoteVoicePar[nvoice].FineDetune = getdetune(
                pars->GlobalPar.PDetuneType,
                0,
                pars->VoicePar[nvoice].PDetune);                               //fine detune
        }
        if(pars->VoicePar[nvoice].PFMDetuneType != 0)
            NoteVoicePar[nvoice].FMDetune = getdetune(
                pars->VoicePar[nvoice].PFMDetuneType,
                pars->VoicePar[nvoice].
                PFMCoarseDetune,
                pars->VoicePar[nvoice].PFMDetune);
        else
            NoteVoicePar[nvoice].FMDetune = getdetune(
                pars->GlobalPar.PDetuneType,
                pars->VoicePar[nvoice].
                PFMCoarseDetune,
                pars->VoicePar[nvoice].PFMDetune);
        ;


        for(int k = 0; k < unison; k++) {
            oscposhi[nvoice][k]   = 0;
            oscposlo[nvoice][k]   = 0.0;
            oscposhiFM[nvoice][k] = 0;
            oscposloFM[nvoice][k] = 0.0;
        }

        NoteVoicePar[nvoice].OscilSmp =
            new REALTYPE[OSCIL_SIZE + OSCIL_SMP_EXTRA_SAMPLES];                        //the extra points contains the first point

        //Get the voice's oscil or external's voice oscil
        int vc = nvoice;
        if(pars->VoicePar[nvoice].Pextoscil != -1)
            vc = pars->VoicePar[nvoice].Pextoscil;
        if(!pars->GlobalPar.Hrandgrouping)
            pars->VoicePar[vc].OscilSmp->newrandseed(rand());
        int oscposhi_start =
            pars->VoicePar[vc].OscilSmp->get(NoteVoicePar[nvoice].OscilSmp,
                                             getvoicebasefreq(nvoice),
                                             pars->VoicePar[nvoice].Presonance);

        //I store the first elments to the last position for speedups
        for(int i = 0; i < OSCIL_SMP_EXTRA_SAMPLES; i++)
            NoteVoicePar[nvoice].OscilSmp[OSCIL_SIZE
                                          + i] =
                NoteVoicePar[nvoice].OscilSmp[i];

        oscposhi_start +=
            (int)((pars->VoicePar[nvoice].Poscilphase
                   - 64.0) / 128.0 * OSCIL_SIZE + OSCIL_SIZE * 4);
        oscposhi_start %= OSCIL_SIZE;

        for(int k = 0; k < unison; k++) {
            oscposhi[nvoice][k] = oscposhi_start;
            oscposhi_start      = (int)(RND * (OSCIL_SIZE - 1)); //put random starting point for other subvoices
        }

        NoteVoicePar[nvoice].FreqLfo = NULL;
        NoteVoicePar[nvoice].FreqEnvelope      = NULL;

        NoteVoicePar[nvoice].AmpLfo            = NULL;
        NoteVoicePar[nvoice].AmpEnvelope       = NULL;

        NoteVoicePar[nvoice].VoiceFilterL      = NULL;
        NoteVoicePar[nvoice].VoiceFilterR      = NULL;
        NoteVoicePar[nvoice].FilterEnvelope    = NULL;
        NoteVoicePar[nvoice].FilterLfo         = NULL;

        NoteVoicePar[nvoice].FilterCenterPitch =
            pars->VoicePar[nvoice].VoiceFilter->getfreq();
        NoteVoicePar[nvoice].filterbypass      =
            pars->VoicePar[nvoice].Pfilterbypass;

        switch(pars->VoicePar[nvoice].PFMEnabled) {
        case 1:
            NoteVoicePar[nvoice].FMEnabled = MORPH;
            break;
        case 2:
            NoteVoicePar[nvoice].FMEnabled = RING_MOD;
            break;
        case 3:
            NoteVoicePar[nvoice].FMEnabled = PHASE_MOD;
            break;
        case 4:
            NoteVoicePar[nvoice].FMEnabled = FREQ_MOD;
            break;
        case 5:
            NoteVoicePar[nvoice].FMEnabled = PITCH_MOD;
            break;
        default:
            NoteVoicePar[nvoice].FMEnabled = NONE;
        }

        NoteVoicePar[nvoice].FMVoice = pars->VoicePar[nvoice].PFMVoice;
        NoteVoicePar[nvoice].FMFreqEnvelope = NULL;
        NoteVoicePar[nvoice].FMAmpEnvelope  = NULL;

        //Compute the Voice's modulator volume (incl. damping)
        REALTYPE fmvoldamp = pow(440.0 / getvoicebasefreq(
                                     nvoice),
                                 pars->VoicePar[nvoice].PFMVolumeDamp / 64.0
                                 - 1.0);
        switch(NoteVoicePar[nvoice].FMEnabled) {
        case PHASE_MOD:
            fmvoldamp =
                pow(440.0 / getvoicebasefreq(
                        nvoice), pars->VoicePar[nvoice].PFMVolumeDamp / 64.0);
            NoteVoicePar[nvoice].FMVolume =
                (exp(pars->VoicePar[nvoice].PFMVolume / 127.0
                     * FM_AMP_MULTIPLIER) - 1.0) * fmvoldamp * 4.0;
            break;
        case FREQ_MOD:
            NoteVoicePar[nvoice].FMVolume =
                (exp(pars->VoicePar[nvoice].PFMVolume / 127.0
                     * FM_AMP_MULTIPLIER) - 1.0) * fmvoldamp * 4.0;
            break;
        //    case PITCH_MOD:NoteVoicePar[nvoice].FMVolume=(pars->VoicePar[nvoice].PFMVolume/127.0*8.0)*fmvoldamp;//???????????
        //	          break;
        default:
            if(fmvoldamp > 1.0)
                fmvoldamp = 1.0;
            NoteVoicePar[nvoice].FMVolume = pars->VoicePar[nvoice].PFMVolume
                                            / 127.0 * fmvoldamp;
        }

        //Voice's modulator velocity sensing
        NoteVoicePar[nvoice].FMVolume *=
            VelF(velocity,
                 partparams->VoicePar[nvoice].PFMVelocityScaleFunction);

        FMoldsmp[nvoice] = new REALTYPE [unison];
        for(int k = 0; k < unison; k++)
            FMoldsmp[nvoice][k] = 0.0;                     //this is for FM (integration)

        firsttick[nvoice] = 1;
        NoteVoicePar[nvoice].DelayTicks =
            (int)((exp(pars->VoicePar[nvoice].PDelay / 127.0
                       * log(50.0))
                   - 1.0) / SOUND_BUFFER_SIZE / 10.0 * SAMPLE_RATE);
    }

    max_unison = 1;
    for(int nvoice = 0; nvoice < NUM_VOICES; nvoice++)
        if(unison_size[nvoice] > max_unison)
            max_unison = unison_size[nvoice];
    ;

    tmpwave_unison = new REALTYPE *[max_unison];
    for(int k = 0; k < max_unison; k++) {
        tmpwave_unison[k] = new REALTYPE[SOUND_BUFFER_SIZE];
        memset(tmpwave_unison[k], 0, SOUND_BUFFER_SIZE * sizeof(REALTYPE));
    }

    initparameters();
    ready = 1;
}

// ADlegatonote: This function is (mostly) a copy of ADnote(...) and
// initparameters() stuck together with some lines removed so that it
// only alter the already playing note (to perform legato). It is
// possible I left stuff that is not required for this.
void ADnote::ADlegatonote(REALTYPE freq,
                          REALTYPE velocity,
                          int portamento_,
                          int midinote_,
                          bool externcall)
{
    ADnoteParameters *pars = partparams;
    //Controller *ctl_=ctl;

    // Manage legato stuff
    if(externcall)
        Legato.msg = LM_Norm;
    if(Legato.msg != LM_CatchUp) {
        Legato.lastfreq   = Legato.param.freq;
        Legato.param.freq = freq;
        Legato.param.vel  = velocity;
        Legato.param.portamento = portamento_;
        Legato.param.midinote   = midinote_;
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

    portamento = portamento_;
    midinote   = midinote_;
    basefreq   = freq;

    if(velocity > 1.0)
        velocity = 1.0;
    this->velocity = velocity;

    NoteGlobalPar.Detune      = getdetune(pars->GlobalPar.PDetuneType,
                                          pars->GlobalPar.PCoarseDetune,
                                          pars->GlobalPar.PDetune);
    bandwidthDetuneMultiplier = pars->getBandwidthDetuneMultiplier();

    if(pars->GlobalPar.PPanning == 0)
        NoteGlobalPar.Panning = RND;
    else
        NoteGlobalPar.Panning = pars->GlobalPar.PPanning / 128.0;


    NoteGlobalPar.FilterCenterPitch = pars->GlobalPar.GlobalFilter->getfreq() //center freq
                                      + pars->GlobalPar.PFilterVelocityScale
                                      / 127.0 * 6.0                                  //velocity sensing
                                      * (VelF(velocity,
                                              pars->GlobalPar.
                                              PFilterVelocityScaleFunction) - 1);


    for(int nvoice = 0; nvoice < NUM_VOICES; nvoice++) {
        if(NoteVoicePar[nvoice].Enabled == OFF)
            continue; //(gf) Stay the same as first note in legato.

        NoteVoicePar[nvoice].fixedfreq   = pars->VoicePar[nvoice].Pfixedfreq;
        NoteVoicePar[nvoice].fixedfreqET = pars->VoicePar[nvoice].PfixedfreqET;

        //use the Globalpars.detunetype if the detunetype is 0
        if(pars->VoicePar[nvoice].PDetuneType != 0) {
            NoteVoicePar[nvoice].Detune     = getdetune(
                pars->VoicePar[nvoice].PDetuneType,
                pars->VoicePar[nvoice].
                PCoarseDetune,
                8192);                                                                        //coarse detune
            NoteVoicePar[nvoice].FineDetune = getdetune(
                pars->VoicePar[nvoice].PDetuneType,
                0,
                pars->VoicePar[nvoice].PDetune);                               //fine detune
        }
        else {
            NoteVoicePar[nvoice].Detune     = getdetune(
                pars->GlobalPar.PDetuneType,
                pars->VoicePar[nvoice].
                PCoarseDetune,
                8192);                                                                        //coarse detune
            NoteVoicePar[nvoice].FineDetune = getdetune(
                pars->GlobalPar.PDetuneType,
                0,
                pars->VoicePar[nvoice].PDetune);                               //fine detune
        }
        if(pars->VoicePar[nvoice].PFMDetuneType != 0)
            NoteVoicePar[nvoice].FMDetune = getdetune(
                pars->VoicePar[nvoice].PFMDetuneType,
                pars->VoicePar[nvoice].
                PFMCoarseDetune,
                pars->VoicePar[nvoice].PFMDetune);
        else
            NoteVoicePar[nvoice].FMDetune = getdetune(
                pars->GlobalPar.PDetuneType,
                pars->VoicePar[nvoice].
                PFMCoarseDetune,
                pars->VoicePar[nvoice].PFMDetune);
        ;

        //Get the voice's oscil or external's voice oscil
        int vc = nvoice;
        if(pars->VoicePar[nvoice].Pextoscil != -1)
            vc = pars->VoicePar[nvoice].Pextoscil;
        if(!pars->GlobalPar.Hrandgrouping)
            pars->VoicePar[vc].OscilSmp->newrandseed(rand());

        pars->VoicePar[vc].OscilSmp->get(NoteVoicePar[nvoice].OscilSmp,
                                         getvoicebasefreq(nvoice),
                                         pars->VoicePar[nvoice].Presonance);                                                       //(gf)Modif of the above line.

        //I store the first elments to the last position for speedups
        for(int i = 0; i < OSCIL_SMP_EXTRA_SAMPLES; i++)
            NoteVoicePar[nvoice].OscilSmp[OSCIL_SIZE
                                          + i] =
                NoteVoicePar[nvoice].OscilSmp[i];


        NoteVoicePar[nvoice].FilterCenterPitch =
            pars->VoicePar[nvoice].VoiceFilter->getfreq();
        NoteVoicePar[nvoice].filterbypass      =
            pars->VoicePar[nvoice].Pfilterbypass;


        NoteVoicePar[nvoice].FMVoice = pars->VoicePar[nvoice].PFMVoice;

        //Compute the Voice's modulator volume (incl. damping)
        REALTYPE fmvoldamp = pow(440.0 / getvoicebasefreq(
                                     nvoice),
                                 pars->VoicePar[nvoice].PFMVolumeDamp / 64.0
                                 - 1.0);

        switch(NoteVoicePar[nvoice].FMEnabled) {
        case PHASE_MOD:
            fmvoldamp =
                pow(440.0 / getvoicebasefreq(
                        nvoice), pars->VoicePar[nvoice].PFMVolumeDamp / 64.0);
            NoteVoicePar[nvoice].FMVolume =
                (exp(pars->VoicePar[nvoice].PFMVolume / 127.0
                     * FM_AMP_MULTIPLIER) - 1.0) * fmvoldamp * 4.0;
            break;
        case FREQ_MOD:
            NoteVoicePar[nvoice].FMVolume =
                (exp(pars->VoicePar[nvoice].PFMVolume / 127.0
                     * FM_AMP_MULTIPLIER) - 1.0) * fmvoldamp * 4.0;
            break;
        //    case PITCH_MOD:NoteVoicePar[nvoice].FMVolume=(pars->VoicePar[nvoice].PFMVolume/127.0*8.0)*fmvoldamp;//???????????
        //	          break;
        default:
            if(fmvoldamp > 1.0)
                fmvoldamp = 1.0;
            NoteVoicePar[nvoice].FMVolume = pars->VoicePar[nvoice].PFMVolume
                                            / 127.0 * fmvoldamp;
        }

        //Voice's modulator velocity sensing
        NoteVoicePar[nvoice].FMVolume *=
            VelF(velocity,
                 partparams->VoicePar[nvoice].PFMVelocityScaleFunction);

        NoteVoicePar[nvoice].DelayTicks =
            (int)((exp(pars->VoicePar[nvoice].PDelay / 127.0
                       * log(50.0))
                   - 1.0) / SOUND_BUFFER_SIZE / 10.0 * SAMPLE_RATE);
    }

    ///    initparameters();

    ///////////////
    // Altered content of initparameters():

    int nvoice, i, tmp[NUM_VOICES];

    NoteGlobalPar.Volume = 4.0
                           * pow(0.1, 3.0
                                 * (1.0 - partparams->GlobalPar.PVolume / 96.0))  //-60 dB .. 0 dB
                           * VelF(
        velocity,
        partparams->GlobalPar.
        PAmpVelocityScaleFunction);                                                      //velocity sensing

    globalnewamplitude = NoteGlobalPar.Volume
                         * NoteGlobalPar.AmpEnvelope->envout_dB()
                         * NoteGlobalPar.AmpLfo->amplfoout();

    NoteGlobalPar.FilterQ = partparams->GlobalPar.GlobalFilter->getq();
    NoteGlobalPar.FilterFreqTracking =
        partparams->GlobalPar.GlobalFilter->getfreqtracking(basefreq);

    // Forbids the Modulation Voice to be greater or equal than voice
    for(i = 0; i < NUM_VOICES; i++)
        if(NoteVoicePar[i].FMVoice >= i)
            NoteVoicePar[i].FMVoice = -1;

    // Voice Parameter init
    for(nvoice = 0; nvoice < NUM_VOICES; nvoice++) {
        if(NoteVoicePar[nvoice].Enabled == 0)
            continue;

        NoteVoicePar[nvoice].noisetype = partparams->VoicePar[nvoice].Type;
        /* Voice Amplitude Parameters Init */
        NoteVoicePar[nvoice].Volume    =
            pow(0.1, 3.0 * (1.0 - partparams->VoicePar[nvoice].PVolume / 127.0))                  // -60 dB .. 0 dB
            * VelF(velocity,
                   partparams->VoicePar[nvoice].PAmpVelocityScaleFunction);                                //velocity

        if(partparams->VoicePar[nvoice].PVolumeminus != 0)
            NoteVoicePar[nvoice].Volume = -NoteVoicePar[nvoice].Volume;

        if(partparams->VoicePar[nvoice].PPanning == 0)
            NoteVoicePar[nvoice].Panning = RND; // random panning
        else
            NoteVoicePar[nvoice].Panning =
                partparams->VoicePar[nvoice].PPanning / 128.0;

        newamplitude[nvoice] = 1.0;
        if((partparams->VoicePar[nvoice].PAmpEnvelopeEnabled != 0)
           && (NoteVoicePar[nvoice].AmpEnvelope != NULL))
            newamplitude[nvoice] *= NoteVoicePar[nvoice].AmpEnvelope->envout_dB();
        ;

        if((partparams->VoicePar[nvoice].PAmpLfoEnabled != 0)
           && (NoteVoicePar[nvoice].AmpLfo != NULL))
            newamplitude[nvoice] *= NoteVoicePar[nvoice].AmpLfo->amplfoout();
        ;


        NoteVoicePar[nvoice].FilterFreqTracking =
            partparams->VoicePar[nvoice].VoiceFilter->getfreqtracking(basefreq);

        /* Voice Modulation Parameters Init */
        if((NoteVoicePar[nvoice].FMEnabled != NONE)
           && (NoteVoicePar[nvoice].FMVoice < 0)) {
            partparams->VoicePar[nvoice].FMSmp->newrandseed(rand());

            //Perform Anti-aliasing only on MORPH or RING MODULATION

            int vc = nvoice;
            if(partparams->VoicePar[nvoice].PextFMoscil != -1)
                vc = partparams->VoicePar[nvoice].PextFMoscil;

            REALTYPE tmp = 1.0;
            if((partparams->VoicePar[vc].FMSmp->Padaptiveharmonics != 0)
               || (NoteVoicePar[nvoice].FMEnabled == MORPH)
               || (NoteVoicePar[nvoice].FMEnabled == RING_MOD))
                tmp = getFMvoicebasefreq(nvoice);
            ;
            if(!partparams->GlobalPar.Hrandgrouping)
                partparams->VoicePar[vc].FMSmp->newrandseed(rand());

            ///oscposhiFM[nvoice]=(oscposhi[nvoice]+partparams->VoicePar[vc].FMSmp->get(NoteVoicePar[nvoice].FMSmp,tmp)) % OSCIL_SIZE;
            // /	oscposhi[nvoice]+partparams->VoicePar[vc].FMSmp->get(NoteVoicePar[nvoice].FMSmp,tmp); //(gf) Modif of the above line.
            for(int i = 0; i < OSCIL_SMP_EXTRA_SAMPLES; i++)
                NoteVoicePar[nvoice].FMSmp[OSCIL_SIZE
                                           + i] = NoteVoicePar[nvoice].FMSmp[i];
            ///oscposhiFM[nvoice]+=(int)((partparams->VoicePar[nvoice].PFMoscilphase-64.0)/128.0*OSCIL_SIZE+OSCIL_SIZE*4);
            ///oscposhiFM[nvoice]%=OSCIL_SIZE;
        }

        FMnewamplitude[nvoice] = NoteVoicePar[nvoice].FMVolume
                                 * ctl->fmamp.relamp;

        if((partparams->VoicePar[nvoice].PFMAmpEnvelopeEnabled != 0)
           && (NoteVoicePar[nvoice].FMAmpEnvelope != NULL))
            FMnewamplitude[nvoice] *=
                NoteVoicePar[nvoice].FMAmpEnvelope->envout_dB();
        ;
    }

    for(nvoice = 0; nvoice < NUM_VOICES; nvoice++) {
        for(i = nvoice + 1; i < NUM_VOICES; i++)
            tmp[i] = 0;
        for(i = nvoice + 1; i < NUM_VOICES; i++)
            if((NoteVoicePar[i].FMVoice == nvoice) && (tmp[i] == 0))
                tmp[i] = 1;
        ;
    }
    ///////////////

    // End of the ADlegatonote function.
}


/*
 * Kill a voice of ADnote
 */
void ADnote::KillVoice(int nvoice)
{
    delete [] oscfreqhi[nvoice];
    delete [] oscfreqlo[nvoice];
    delete [] oscfreqhiFM[nvoice];
    delete [] oscfreqloFM[nvoice];
    delete [] oscposhi[nvoice];
    delete [] oscposlo[nvoice];
    delete [] oscposhiFM[nvoice];
    delete [] oscposloFM[nvoice];

    delete [] NoteVoicePar[nvoice].OscilSmp;
    delete [] unison_base_freq_rap[nvoice];
    delete [] unison_freq_rap[nvoice];
    delete [] unison_invert_phase[nvoice];
    delete [] FMoldsmp[nvoice];
    delete [] unison_vibratto[nvoice].step;
    delete [] unison_vibratto[nvoice].position;

    if(NoteVoicePar[nvoice].FreqEnvelope != NULL)
        delete (NoteVoicePar[nvoice].FreqEnvelope);
    NoteVoicePar[nvoice].FreqEnvelope = NULL;

    if(NoteVoicePar[nvoice].FreqLfo != NULL)
        delete (NoteVoicePar[nvoice].FreqLfo);
    NoteVoicePar[nvoice].FreqLfo = NULL;

    if(NoteVoicePar[nvoice].AmpEnvelope != NULL)
        delete (NoteVoicePar[nvoice].AmpEnvelope);
    NoteVoicePar[nvoice].AmpEnvelope = NULL;

    if(NoteVoicePar[nvoice].AmpLfo != NULL)
        delete (NoteVoicePar[nvoice].AmpLfo);
    NoteVoicePar[nvoice].AmpLfo = NULL;

    if(NoteVoicePar[nvoice].VoiceFilterL != NULL)
        delete (NoteVoicePar[nvoice].VoiceFilterL);
    NoteVoicePar[nvoice].VoiceFilterL = NULL;

    if(NoteVoicePar[nvoice].VoiceFilterR != NULL)
        delete (NoteVoicePar[nvoice].VoiceFilterR);
    NoteVoicePar[nvoice].VoiceFilterR = NULL;

    if(NoteVoicePar[nvoice].FilterEnvelope != NULL)
        delete (NoteVoicePar[nvoice].FilterEnvelope);
    NoteVoicePar[nvoice].FilterEnvelope = NULL;

    if(NoteVoicePar[nvoice].FilterLfo != NULL)
        delete (NoteVoicePar[nvoice].FilterLfo);
    NoteVoicePar[nvoice].FilterLfo = NULL;

    if(NoteVoicePar[nvoice].FMFreqEnvelope != NULL)
        delete (NoteVoicePar[nvoice].FMFreqEnvelope);
    NoteVoicePar[nvoice].FMFreqEnvelope = NULL;

    if(NoteVoicePar[nvoice].FMAmpEnvelope != NULL)
        delete (NoteVoicePar[nvoice].FMAmpEnvelope);
    NoteVoicePar[nvoice].FMAmpEnvelope = NULL;

    if((NoteVoicePar[nvoice].FMEnabled != NONE)
       && (NoteVoicePar[nvoice].FMVoice < 0))
        delete [] NoteVoicePar[nvoice].FMSmp;

    if(NoteVoicePar[nvoice].VoiceOut != NULL)
        memset(NoteVoicePar[nvoice].VoiceOut, 0, SOUND_BUFFER_SIZE
                * sizeof(REALTYPE));//do not delete, yet: perhaps is used by another voice

    NoteVoicePar[nvoice].Enabled = OFF;
}

/*
 * Kill the note
 */
void ADnote::KillNote()
{
    int nvoice;
    for(nvoice = 0; nvoice < NUM_VOICES; nvoice++) {
        if(NoteVoicePar[nvoice].Enabled == ON)
            KillVoice(nvoice);

        //delete VoiceOut
        if(NoteVoicePar[nvoice].VoiceOut != NULL)
            delete (NoteVoicePar[nvoice].VoiceOut);
        NoteVoicePar[nvoice].VoiceOut = NULL;
    }

    delete (NoteGlobalPar.FreqEnvelope);
    delete (NoteGlobalPar.FreqLfo);
    delete (NoteGlobalPar.AmpEnvelope);
    delete (NoteGlobalPar.AmpLfo);
    delete (NoteGlobalPar.GlobalFilterL);
    if(stereo != 0)
        delete (NoteGlobalPar.GlobalFilterR);
    delete (NoteGlobalPar.FilterEnvelope);
    delete (NoteGlobalPar.FilterLfo);

    NoteEnabled = OFF;
}

ADnote::~ADnote()
{
    if(NoteEnabled == ON)
        KillNote();
    delete [] tmpwavel;
    delete [] tmpwaver;
    delete [] bypassl;
    delete [] bypassr;
    for(int k = 0; k < max_unison; k++)
        delete[] tmpwave_unison[k];
    delete[] tmpwave_unison;
}


/*
 * Init the parameters
 */
void ADnote::initparameters()
{
    int nvoice, i, tmp[NUM_VOICES];

    // Global Parameters
    NoteGlobalPar.FreqEnvelope = new Envelope(
        partparams->GlobalPar.FreqEnvelope,
        basefreq);
    NoteGlobalPar.FreqLfo      = new LFO(partparams->GlobalPar.FreqLfo,
                                         basefreq);

    NoteGlobalPar.AmpEnvelope  = new Envelope(partparams->GlobalPar.AmpEnvelope,
                                              basefreq);
    NoteGlobalPar.AmpLfo = new LFO(partparams->GlobalPar.AmpLfo, basefreq);

    NoteGlobalPar.Volume = 4.0
                           * pow(0.1, 3.0
                                 * (1.0 - partparams->GlobalPar.PVolume / 96.0))  //-60 dB .. 0 dB
                           * VelF(
        velocity,
        partparams->GlobalPar.
        PAmpVelocityScaleFunction);                                                      //velocity sensing

    NoteGlobalPar.AmpEnvelope->envout_dB(); //discard the first envelope output
    globalnewamplitude = NoteGlobalPar.Volume
                         * NoteGlobalPar.AmpEnvelope->envout_dB()
                         * NoteGlobalPar.AmpLfo->amplfoout();

    NoteGlobalPar.GlobalFilterL = new Filter(partparams->GlobalPar.GlobalFilter);
    if(stereo != 0)
        NoteGlobalPar.GlobalFilterR = new Filter(
            partparams->GlobalPar.GlobalFilter);

    NoteGlobalPar.FilterEnvelope     = new Envelope(
        partparams->GlobalPar.FilterEnvelope,
        basefreq);
    NoteGlobalPar.FilterLfo          = new LFO(partparams->GlobalPar.FilterLfo,
                                               basefreq);
    NoteGlobalPar.FilterQ = partparams->GlobalPar.GlobalFilter->getq();
    NoteGlobalPar.FilterFreqTracking =
        partparams->GlobalPar.GlobalFilter->getfreqtracking(basefreq);

    // Forbids the Modulation Voice to be greater or equal than voice
    for(i = 0; i < NUM_VOICES; i++)
        if(NoteVoicePar[i].FMVoice >= i)
            NoteVoicePar[i].FMVoice = -1;

    // Voice Parameter init
    for(nvoice = 0; nvoice < NUM_VOICES; nvoice++) {
        if(NoteVoicePar[nvoice].Enabled == 0)
            continue;

        NoteVoicePar[nvoice].noisetype = partparams->VoicePar[nvoice].Type;
        /* Voice Amplitude Parameters Init */
        NoteVoicePar[nvoice].Volume    =
            pow(0.1, 3.0 * (1.0 - partparams->VoicePar[nvoice].PVolume / 127.0))                  // -60 dB .. 0 dB
            * VelF(velocity,
                   partparams->VoicePar[nvoice].PAmpVelocityScaleFunction);                                //velocity

        if(partparams->VoicePar[nvoice].PVolumeminus != 0)
            NoteVoicePar[nvoice].Volume = -NoteVoicePar[nvoice].Volume;

        if(partparams->VoicePar[nvoice].PPanning == 0)
            NoteVoicePar[nvoice].Panning = RND; // random panning
        else
            NoteVoicePar[nvoice].Panning =
                partparams->VoicePar[nvoice].PPanning / 128.0;

        newamplitude[nvoice] = 1.0;
        if(partparams->VoicePar[nvoice].PAmpEnvelopeEnabled != 0) {
            NoteVoicePar[nvoice].AmpEnvelope = new Envelope(
                partparams->VoicePar[nvoice].AmpEnvelope,
                basefreq);
            NoteVoicePar[nvoice].AmpEnvelope->envout_dB(); //discard the first envelope sample
            newamplitude[nvoice] *= NoteVoicePar[nvoice].AmpEnvelope->envout_dB();
        }

        if(partparams->VoicePar[nvoice].PAmpLfoEnabled != 0) {
            NoteVoicePar[nvoice].AmpLfo = new LFO(
                partparams->VoicePar[nvoice].AmpLfo,
                basefreq);
            newamplitude[nvoice] *= NoteVoicePar[nvoice].AmpLfo->amplfoout();
        }

        /* Voice Frequency Parameters Init */
        if(partparams->VoicePar[nvoice].PFreqEnvelopeEnabled != 0)
            NoteVoicePar[nvoice].FreqEnvelope = new Envelope(
                partparams->VoicePar[nvoice].FreqEnvelope,
                basefreq);

        if(partparams->VoicePar[nvoice].PFreqLfoEnabled != 0)
            NoteVoicePar[nvoice].FreqLfo = new LFO(
                partparams->VoicePar[nvoice].FreqLfo,
                basefreq);

        /* Voice Filter Parameters Init */
        if(partparams->VoicePar[nvoice].PFilterEnabled != 0) {
            NoteVoicePar[nvoice].VoiceFilterL = new Filter(
                partparams->VoicePar[nvoice].VoiceFilter);
            NoteVoicePar[nvoice].VoiceFilterR = new Filter(
                partparams->VoicePar[nvoice].VoiceFilter);
        }

        if(partparams->VoicePar[nvoice].PFilterEnvelopeEnabled != 0)
            NoteVoicePar[nvoice].FilterEnvelope = new Envelope(
                partparams->VoicePar[nvoice].FilterEnvelope,
                basefreq);

        if(partparams->VoicePar[nvoice].PFilterLfoEnabled != 0)
            NoteVoicePar[nvoice].FilterLfo =
                new LFO(partparams->VoicePar[nvoice].FilterLfo, basefreq);

        NoteVoicePar[nvoice].FilterFreqTracking =
            partparams->VoicePar[nvoice].VoiceFilter->getfreqtracking(basefreq);

        /* Voice Modulation Parameters Init */
        if((NoteVoicePar[nvoice].FMEnabled != NONE)
           && (NoteVoicePar[nvoice].FMVoice < 0)) {
            partparams->VoicePar[nvoice].FMSmp->newrandseed(rand());
            NoteVoicePar[nvoice].FMSmp =
                new REALTYPE[OSCIL_SIZE + OSCIL_SMP_EXTRA_SAMPLES];

            //Perform Anti-aliasing only on MORPH or RING MODULATION

            int vc = nvoice;
            if(partparams->VoicePar[nvoice].PextFMoscil != -1)
                vc = partparams->VoicePar[nvoice].PextFMoscil;

            REALTYPE tmp = 1.0;
            if((partparams->VoicePar[vc].FMSmp->Padaptiveharmonics != 0)
               || (NoteVoicePar[nvoice].FMEnabled == MORPH)
               || (NoteVoicePar[nvoice].FMEnabled == RING_MOD))
                tmp = getFMvoicebasefreq(nvoice);
            ;
            if(!partparams->GlobalPar.Hrandgrouping)
                partparams->VoicePar[vc].FMSmp->newrandseed(rand());

            for(int k = 0; k < unison_size[nvoice]; k++)
                oscposhiFM[nvoice][k] =
                    (oscposhi[nvoice][k]
                     + partparams->VoicePar[vc].FMSmp->get(NoteVoicePar[nvoice]
                                                           .
                                                           FMSmp,
                                                           tmp)) % OSCIL_SIZE;
            ;
            for(int i = 0; i < OSCIL_SMP_EXTRA_SAMPLES; i++)
                NoteVoicePar[nvoice].FMSmp[OSCIL_SIZE
                                           + i] = NoteVoicePar[nvoice].FMSmp[i];
            int oscposhiFM_add =
                (int)((partparams->VoicePar[nvoice].PFMoscilphase
                       - 64.0) / 128.0 * OSCIL_SIZE + OSCIL_SIZE * 4);
            for(int k = 0; k < unison_size[nvoice]; k++) {
                oscposhiFM[nvoice][k] += oscposhiFM_add;
                oscposhiFM[nvoice][k] %= OSCIL_SIZE;
            }
        }

        if(partparams->VoicePar[nvoice].PFMFreqEnvelopeEnabled != 0)
            NoteVoicePar[nvoice].FMFreqEnvelope = new Envelope(
                partparams->VoicePar[nvoice].FMFreqEnvelope,
                basefreq);

        FMnewamplitude[nvoice] = NoteVoicePar[nvoice].FMVolume
                                 * ctl->fmamp.relamp;

        if(partparams->VoicePar[nvoice].PFMAmpEnvelopeEnabled != 0) {
            NoteVoicePar[nvoice].FMAmpEnvelope = new Envelope(
                partparams->VoicePar[nvoice].FMAmpEnvelope,
                basefreq);
            FMnewamplitude[nvoice] *=
                NoteVoicePar[nvoice].FMAmpEnvelope->envout_dB();
        }
    }

    for(nvoice = 0; nvoice < NUM_VOICES; nvoice++) {
        for(i = nvoice + 1; i < NUM_VOICES; i++)
            tmp[i] = 0;
        for(i = nvoice + 1; i < NUM_VOICES; i++)
            if((NoteVoicePar[i].FMVoice == nvoice) && (tmp[i] == 0)) {
                NoteVoicePar[nvoice].VoiceOut = new REALTYPE[SOUND_BUFFER_SIZE];
                tmp[i] = 1;
            }
        ;
        if(NoteVoicePar[nvoice].VoiceOut != NULL)
            memset(NoteVoicePar[nvoice].VoiceOut, 0, SOUND_BUFFER_SIZE * sizeof(REALTYPE));
    }
}


/*
 * Computes the relative frequency of each unison voice and it's vibratto
 * This must be called before setfreq* functions
 */
void ADnote::compute_unison_freq_rap(int nvoice) {
    if(unison_size[nvoice] == 1) { //no unison
        unison_freq_rap[nvoice][0] = 1.0;
        return;
    }
    REALTYPE relbw = ctl->bandwidth.relbw * bandwidthDetuneMultiplier;
    for(int k = 0; k < unison_size[nvoice]; k++) {
        REALTYPE pos  = unison_vibratto[nvoice].position[k];
        REALTYPE step = unison_vibratto[nvoice].step[k];
        pos += step;
        if(pos <= -1.0) {
            pos  = -1.0;
            step = -step;
        }
        if(pos >= 1.0) {
            pos  = 1.0;
            step = -step;
        }
        REALTYPE vibratto_val = (pos - 0.333333333 * pos * pos * pos) * 1.5; //make the vibratto lfo smoother
        unison_freq_rap[nvoice][k] = 1.0
                                     + ((unison_base_freq_rap[nvoice][k]
                                         - 1.0) + vibratto_val
                                        * unison_vibratto[nvoice].amplitude)
                                     * relbw;

        unison_vibratto[nvoice].position[k] = pos;
        step = unison_vibratto[nvoice].step[k] = step;
    }
}


/*
 * Computes the frequency of an oscillator
 */
void ADnote::setfreq(int nvoice, REALTYPE in_freq)
{
    for(int k = 0; k < unison_size[nvoice]; k++) {
        REALTYPE freq  = fabs(in_freq) * unison_freq_rap[nvoice][k];
        REALTYPE speed = freq * REALTYPE(OSCIL_SIZE) / (REALTYPE) SAMPLE_RATE;
        if(speed > OSCIL_SIZE)
            speed = OSCIL_SIZE;

        F2I(speed, oscfreqhi[nvoice][k]);
        oscfreqlo[nvoice][k] = speed - floor(speed);
    }
}

/*
 * Computes the frequency of an modullator oscillator
 */
void ADnote::setfreqFM(int nvoice, REALTYPE in_freq)
{
    for(int k = 0; k < unison_size[nvoice]; k++) {
        REALTYPE freq  = fabs(in_freq) * unison_freq_rap[nvoice][k];
        REALTYPE speed = freq * REALTYPE(OSCIL_SIZE) / (REALTYPE) SAMPLE_RATE;
        if(speed > OSCIL_SIZE)
            speed = OSCIL_SIZE;

        F2I(speed, oscfreqhiFM[nvoice][k]);
        oscfreqloFM[nvoice][k] = speed - floor(speed);
    }
}

/*
 * Get Voice base frequency
 */
REALTYPE ADnote::getvoicebasefreq(int nvoice) const
{
    REALTYPE detune = NoteVoicePar[nvoice].Detune / 100.0
                      + NoteVoicePar[nvoice].FineDetune / 100.0
                      * ctl->bandwidth.relbw * bandwidthDetuneMultiplier
                      + NoteGlobalPar.Detune / 100.0;

    if(NoteVoicePar[nvoice].fixedfreq == 0)
        return this->basefreq * pow(2, detune / 12.0);
    else { //the fixed freq is enabled
        REALTYPE fixedfreq   = 440.0;
        int      fixedfreqET = NoteVoicePar[nvoice].fixedfreqET;
        if(fixedfreqET != 0) { //if the frequency varies according the keyboard note
            REALTYPE tmp =
                (midinote
                 - 69.0) / 12.0 * (pow(2.0, (fixedfreqET - 1) / 63.0) - 1.0);
            if(fixedfreqET <= 64)
                fixedfreq *= pow(2.0, tmp);
            else
                fixedfreq *= pow(3.0, tmp);
        }
        return fixedfreq * pow(2.0, detune / 12.0);
    }
}

/*
 * Get Voice's Modullator base frequency
 */
REALTYPE ADnote::getFMvoicebasefreq(int nvoice) const
{
    REALTYPE detune = NoteVoicePar[nvoice].FMDetune / 100.0;
    return getvoicebasefreq(nvoice) * pow(2, detune / 12.0);
}

/*
 * Computes all the parameters for each tick
 */
void ADnote::computecurrentparameters()
{
    int      nvoice;
    REALTYPE voicefreq, voicepitch, filterpitch, filterfreq, FMfreq,
             FMrelativepitch, globalpitch, globalfilterpitch;
    globalpitch = 0.01 * (NoteGlobalPar.FreqEnvelope->envout()
                          + NoteGlobalPar.FreqLfo->lfoout()
                          * ctl->modwheel.relmod);
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
    if(stereo != 0)
        NoteGlobalPar.GlobalFilterR->setfreq_and_q(tmpfilterfreq, globalfilterq);

    //compute the portamento, if it is used by this note
    REALTYPE portamentofreqrap = 1.0;
    if(portamento != 0) { //this voice use portamento
        portamentofreqrap = ctl->portamento.freqrap;
        if(ctl->portamento.used == 0) //the portamento has finished
            portamento = 0; //this note is no longer "portamented"
        ;
    }

    //compute parameters for all voices
    for(nvoice = 0; nvoice < NUM_VOICES; nvoice++) {
        if(NoteVoicePar[nvoice].Enabled != ON)
            continue;
        NoteVoicePar[nvoice].DelayTicks -= 1;
        if(NoteVoicePar[nvoice].DelayTicks > 0)
            continue;

        compute_unison_freq_rap(nvoice);

        /*******************/
        /* Voice Amplitude */
        /*******************/
        oldamplitude[nvoice] = newamplitude[nvoice];
        newamplitude[nvoice] = 1.0;

        if(NoteVoicePar[nvoice].AmpEnvelope != NULL)
            newamplitude[nvoice] *= NoteVoicePar[nvoice].AmpEnvelope->envout_dB();

        if(NoteVoicePar[nvoice].AmpLfo != NULL)
            newamplitude[nvoice] *= NoteVoicePar[nvoice].AmpLfo->amplfoout();

        /****************/
        /* Voice Filter */
        /****************/
        if(NoteVoicePar[nvoice].VoiceFilterL != NULL) {
            filterpitch = NoteVoicePar[nvoice].FilterCenterPitch;

            if(NoteVoicePar[nvoice].FilterEnvelope != NULL)
                filterpitch += NoteVoicePar[nvoice].FilterEnvelope->envout();

            if(NoteVoicePar[nvoice].FilterLfo != NULL)
                filterpitch += NoteVoicePar[nvoice].FilterLfo->lfoout();

            filterfreq = filterpitch + NoteVoicePar[nvoice].FilterFreqTracking;
            filterfreq = NoteVoicePar[nvoice].VoiceFilterL->getrealfreq(
                filterfreq);

            NoteVoicePar[nvoice].VoiceFilterL->setfreq(filterfreq);
            if(stereo && NoteVoicePar[nvoice].VoiceFilterR)
                NoteVoicePar[nvoice].VoiceFilterR->setfreq(filterfreq);
        }

        if(NoteVoicePar[nvoice].noisetype == 0) { //compute only if the voice isn't noise
            /*******************/
            /* Voice Frequency */
            /*******************/
            voicepitch = 0.0;
            if(NoteVoicePar[nvoice].FreqLfo != NULL)
                voicepitch += NoteVoicePar[nvoice].FreqLfo->lfoout() / 100.0
                              * ctl->bandwidth.relbw;

            if(NoteVoicePar[nvoice].FreqEnvelope != NULL)
                voicepitch += NoteVoicePar[nvoice].FreqEnvelope->envout()
                              / 100.0;
            voicefreq  = getvoicebasefreq(nvoice)
                         * pow(2, (voicepitch + globalpitch) / 12.0);               //Hz frequency
            voicefreq *= ctl->pitchwheel.relfreq; //change the frequency by the controller
            setfreq(nvoice, voicefreq * portamentofreqrap);

            /***************/
            /*  Modulator */
            /***************/
            if(NoteVoicePar[nvoice].FMEnabled != NONE) {
                FMrelativepitch = NoteVoicePar[nvoice].FMDetune / 100.0;
                if(NoteVoicePar[nvoice].FMFreqEnvelope != NULL)
                    FMrelativepitch +=
                        NoteVoicePar[nvoice].FMFreqEnvelope->envout() / 100;
                FMfreq =
                    pow(2.0, FMrelativepitch
                        / 12.0) * voicefreq * portamentofreqrap;
                setfreqFM(nvoice, FMfreq);

                FMoldamplitude[nvoice] = FMnewamplitude[nvoice];
                FMnewamplitude[nvoice] = NoteVoicePar[nvoice].FMVolume
                                         * ctl->fmamp.relamp;
                if(NoteVoicePar[nvoice].FMAmpEnvelope != NULL)
                    FMnewamplitude[nvoice] *=
                        NoteVoicePar[nvoice].FMAmpEnvelope->envout_dB();
            }
        }
    }
    time += (REALTYPE)SOUND_BUFFER_SIZE / (REALTYPE)SAMPLE_RATE;
}


/*
 * Fadein in a way that removes clicks but keep sound "punchy"
 */
inline void ADnote::fadein(REALTYPE *smps) const
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

/*
 * Computes the Oscillator (Without Modulation) - LinearInterpolation
 */
inline void ADnote::ComputeVoiceOscillator_LinearInterpolation(int nvoice)
{
    int      i, poshi;
    REALTYPE poslo;

    for(int k = 0; k < unison_size[nvoice]; k++) {
        poshi = oscposhi[nvoice][k];
        poslo = oscposlo[nvoice][k];
        int freqhi = oscfreqhi[nvoice][k];
        REALTYPE  freqlo = oscfreqlo[nvoice][k];
        REALTYPE *smps   = NoteVoicePar[nvoice].OscilSmp;
        REALTYPE *tw     = tmpwave_unison[k];
        for(i = 0; i < SOUND_BUFFER_SIZE; i++) {
            tw[i]  = smps[poshi] * (1.0 - poslo) + smps[poshi + 1] * poslo;
            poslo += freqlo;
            if(poslo >= 1.0) {
                poslo -= 1.0;
                poshi++;
            }
            poshi += freqhi;
            poshi &= OSCIL_SIZE - 1;
        }
        oscposhi[nvoice][k] = poshi;
        oscposlo[nvoice][k] = poslo;
    }
}



/*
 * Computes the Oscillator (Without Modulation) - CubicInterpolation
 *
 The differences from the Linear are to little to deserve to be used. This is because I am using a large OSCIL_SIZE (>512)
inline void ADnote::ComputeVoiceOscillator_CubicInterpolation(int nvoice){
    int i,poshi;
    REALTYPE poslo;

    poshi=oscposhi[nvoice];
    poslo=oscposlo[nvoice];
    REALTYPE *smps=NoteVoicePar[nvoice].OscilSmp;
    REALTYPE xm1,x0,x1,x2,a,b,c;
    for (i=0;i<SOUND_BUFFER_SIZE;i++){
    xm1=smps[poshi];
    x0=smps[poshi+1];
    x1=smps[poshi+2];
    x2=smps[poshi+3];
    a=(3.0 * (x0-x1) - xm1 + x2) / 2.0;
    b = 2.0*x1 + xm1 - (5.0*x0 + x2) / 2.0;
    c = (x1 - xm1) / 2.0;
    tmpwave[i]=(((a * poslo) + b) * poslo + c) * poslo + x0;
    printf("a\n");
    //tmpwave[i]=smps[poshi]*(1.0-poslo)+smps[poshi+1]*poslo;
    poslo+=oscfreqlo[nvoice];
    if (poslo>=1.0) {
            poslo-=1.0;
        poshi++;
    };
        poshi+=oscfreqhi[nvoice];
        poshi&=OSCIL_SIZE-1;
    };
    oscposhi[nvoice]=poshi;
    oscposlo[nvoice]=poslo;
};
*/
/*
 * Computes the Oscillator (Morphing)
 */
inline void ADnote::ComputeVoiceOscillatorMorph(int nvoice)
{
    int      i;
    REALTYPE amp;
    ComputeVoiceOscillator_LinearInterpolation(nvoice);
    if(FMnewamplitude[nvoice] > 1.0)
        FMnewamplitude[nvoice] = 1.0;
    if(FMoldamplitude[nvoice] > 1.0)
        FMoldamplitude[nvoice] = 1.0;

    if(NoteVoicePar[nvoice].FMVoice >= 0) {
        //if I use VoiceOut[] as modullator
        int FMVoice = NoteVoicePar[nvoice].FMVoice;
        for(int k = 0; k < unison_size[nvoice]; k++) {
            REALTYPE *tw = tmpwave_unison[k];
            for(i = 0; i < SOUND_BUFFER_SIZE; i++) {
                amp   = INTERPOLATE_AMPLITUDE(FMoldamplitude[nvoice],
                                              FMnewamplitude[nvoice],
                                              i,
                                              SOUND_BUFFER_SIZE);
                tw[i] = tw[i]
                        * (1.0 - amp) + amp * NoteVoicePar[FMVoice].VoiceOut[i];
            }
        }
    }
    else {
        for(int k = 0; k < unison_size[nvoice]; k++) {
            int poshiFM = oscposhiFM[nvoice][k];
            REALTYPE  posloFM  = oscposloFM[nvoice][k];
            int       freqhiFM = oscfreqhiFM[nvoice][k];
            REALTYPE  freqloFM = oscfreqloFM[nvoice][k];
            REALTYPE *tw = tmpwave_unison[k];

            for(i = 0; i < SOUND_BUFFER_SIZE; i++) {
                amp   = INTERPOLATE_AMPLITUDE(FMoldamplitude[nvoice],
                                              FMnewamplitude[nvoice],
                                              i,
                                              SOUND_BUFFER_SIZE);
                tw[i] = tw[i] * (1.0 - amp) + amp
                        * (NoteVoicePar[nvoice].FMSmp[poshiFM] * (1 - posloFM)
                           + NoteVoicePar[nvoice].FMSmp[poshiFM + 1] * posloFM);
                posloFM += freqloFM;
                if(posloFM >= 1.0) {
                    posloFM -= 1.0;
                    poshiFM++;
                }
                poshiFM += freqhiFM;
                poshiFM &= OSCIL_SIZE - 1;
            }
            oscposhiFM[nvoice][k] = poshiFM;
            oscposloFM[nvoice][k] = posloFM;
        }
    }
}

/*
 * Computes the Oscillator (Ring Modulation)
 */
inline void ADnote::ComputeVoiceOscillatorRingModulation(int nvoice)
{
    int      i;
    REALTYPE amp;
    ComputeVoiceOscillator_LinearInterpolation(nvoice);
    if(FMnewamplitude[nvoice] > 1.0)
        FMnewamplitude[nvoice] = 1.0;
    if(FMoldamplitude[nvoice] > 1.0)
        FMoldamplitude[nvoice] = 1.0;
    if(NoteVoicePar[nvoice].FMVoice >= 0) {
        // if I use VoiceOut[] as modullator
        for(int k = 0; k < unison_size[nvoice]; k++) {
            REALTYPE *tw = tmpwave_unison[k];
            for(i = 0; i < SOUND_BUFFER_SIZE; i++) {
                amp = INTERPOLATE_AMPLITUDE(FMoldamplitude[nvoice],
                                            FMnewamplitude[nvoice],
                                            i,
                                            SOUND_BUFFER_SIZE);
                int FMVoice = NoteVoicePar[nvoice].FMVoice;
                for(i = 0; i < SOUND_BUFFER_SIZE; i++)
                    tw[i] *=
                        (1.0 - amp) + amp * NoteVoicePar[FMVoice].VoiceOut[i];
            }
        }
    }
    else {
        for(int k = 0; k < unison_size[nvoice]; k++) {
            int poshiFM = oscposhiFM[nvoice][k];
            REALTYPE  posloFM  = oscposloFM[nvoice][k];
            int       freqhiFM = oscfreqhiFM[nvoice][k];
            REALTYPE  freqloFM = oscfreqloFM[nvoice][k];
            REALTYPE *tw = tmpwave_unison[k];

            for(i = 0; i < SOUND_BUFFER_SIZE; i++) {
                amp    = INTERPOLATE_AMPLITUDE(FMoldamplitude[nvoice],
                                               FMnewamplitude[nvoice],
                                               i,
                                               SOUND_BUFFER_SIZE);
                tw[i] *= (NoteVoicePar[nvoice].FMSmp[poshiFM] * (1.0 - posloFM)
                          + NoteVoicePar[nvoice].FMSmp[poshiFM
                                                       + 1] * posloFM) * amp
                         + (1.0 - amp);
                posloFM += freqloFM;
                if(posloFM >= 1.0) {
                    posloFM -= 1.0;
                    poshiFM++;
                }
                poshiFM += freqhiFM;
                poshiFM &= OSCIL_SIZE - 1;
            }
            oscposhiFM[nvoice][k] = poshiFM;
            oscposloFM[nvoice][k] = posloFM;
        }
    }
}



/*
 * Computes the Oscillator (Phase Modulation or Frequency Modulation)
 */
inline void ADnote::ComputeVoiceOscillatorFrequencyModulation(int nvoice,
                                                              int FMmode)
{
    int      carposhi    = 0;
    int      i, FMmodfreqhi = 0;
    REALTYPE FMmodfreqlo = 0, carposlo = 0;

    if(NoteVoicePar[nvoice].FMVoice >= 0) {
        //if I use VoiceOut[] as modulator
        for(int k = 0; k < unison_size[nvoice]; k++) {
            REALTYPE *tw = tmpwave_unison[k];
            memcpy(tw, NoteVoicePar[NoteVoicePar[nvoice].FMVoice].VoiceOut,
                    SOUND_BUFFER_SIZE * sizeof(REALTYPE));
        }
    }
    else {
        //Compute the modulator and store it in tmpwave_unison[][]
        for(int k = 0; k < unison_size[nvoice]; k++) {
            int poshiFM = oscposhiFM[nvoice][k];
            REALTYPE  posloFM  = oscposloFM[nvoice][k];
            int       freqhiFM = oscfreqhiFM[nvoice][k];
            REALTYPE  freqloFM = oscfreqloFM[nvoice][k];
            REALTYPE *tw = tmpwave_unison[k];

            for(i = 0; i < SOUND_BUFFER_SIZE; i++) {
                tw[i] =
                    (NoteVoicePar[nvoice].FMSmp[poshiFM] * (1.0 - posloFM)
                     + NoteVoicePar[nvoice].FMSmp[poshiFM + 1] * posloFM);
                posloFM += freqloFM;
                if(posloFM >= 1.0) {
                    posloFM = fmod(posloFM, 1.0);
                    poshiFM++;
                }
                poshiFM += freqhiFM;
                poshiFM &= OSCIL_SIZE - 1;
            }
            oscposhiFM[nvoice][k] = poshiFM;
            oscposloFM[nvoice][k] = posloFM;
        }
    }
    // Amplitude interpolation
    if(ABOVE_AMPLITUDE_THRESHOLD(FMoldamplitude[nvoice],
                                 FMnewamplitude[nvoice])) {
        for(int k = 0; k < unison_size[nvoice]; k++) {
            REALTYPE *tw = tmpwave_unison[k];
            for(i = 0; i < SOUND_BUFFER_SIZE; i++)
                tw[i] *= INTERPOLATE_AMPLITUDE(FMoldamplitude[nvoice],
                                               FMnewamplitude[nvoice],
                                               i,
                                               SOUND_BUFFER_SIZE);
            ;
        }
    }
    else {
        for(int k = 0; k < unison_size[nvoice]; k++) {
            REALTYPE *tw = tmpwave_unison[k];
            for(i = 0; i < SOUND_BUFFER_SIZE; i++)
                tw[i] *= FMnewamplitude[nvoice];
        }
    }


    //normalize: makes all sample-rates, oscil_sizes to produce same sound
    if(FMmode != 0) { //Frequency modulation
        REALTYPE normalize = OSCIL_SIZE / 262144.0 * 44100.0
                             / (REALTYPE)SAMPLE_RATE;
        for(int k = 0; k < unison_size[nvoice]; k++) {
            REALTYPE *tw    = tmpwave_unison[k];
            REALTYPE  fmold = FMoldsmp[nvoice][k];
            for(i = 0; i < SOUND_BUFFER_SIZE; i++) {
                fmold = fmod(fmold + tw[i] * normalize, OSCIL_SIZE);
                tw[i] = fmold;
            }
            FMoldsmp[nvoice][k] = fmold;
        }
    }
    else {  //Phase modulation
        REALTYPE normalize = OSCIL_SIZE / 262144.0;
        for(int k = 0; k < unison_size[nvoice]; k++) {
            REALTYPE *tw = tmpwave_unison[k];
            for(i = 0; i < SOUND_BUFFER_SIZE; i++)
                tw[i] *= normalize;
        }
    }

    //do the modulation
    for(int k = 0; k < unison_size[nvoice]; k++) {
        REALTYPE *tw     = tmpwave_unison[k];
        int      poshi  = oscposhi[nvoice][k];
        REALTYPE poslo  = oscposlo[nvoice][k];
        int      freqhi = oscfreqhi[nvoice][k];
        REALTYPE freqlo = oscfreqlo[nvoice][k];

        for(i = 0; i < SOUND_BUFFER_SIZE; i++) {
            F2I(tw[i], FMmodfreqhi);
            FMmodfreqlo = fmod(tw[i] + 0.0000000001, 1.0);
            if(FMmodfreqhi < 0)
                FMmodfreqlo++;

            //carrier
            carposhi = poshi + FMmodfreqhi;
            carposlo = poslo + FMmodfreqlo;

            if(carposlo >= 1.0) {
                carposhi++;
                carposlo = fmod(carposlo, 1.0);
            }
            carposhi &= (OSCIL_SIZE - 1);

            tw[i]     = NoteVoicePar[nvoice].OscilSmp[carposhi]
                        * (1.0 - carposlo)
                        + NoteVoicePar[nvoice].OscilSmp[carposhi
                                                        + 1] * carposlo;

            poslo += freqlo;
            if(poslo >= 1.0) {
                poslo = fmod(poslo, 1.0);
                poshi++;
            }

            poshi += freqhi;
            poshi &= OSCIL_SIZE - 1;
        }
        oscposhi[nvoice][k] = poshi;
        oscposlo[nvoice][k] = poslo;
    }
}


/*Calculeaza Oscilatorul cu PITCH MODULATION*/
inline void ADnote::ComputeVoiceOscillatorPitchModulation(int nvoice)
{
//TODO
}

/*
 * Computes the Noise
 */
inline void ADnote::ComputeVoiceNoise(int nvoice)
{
    for(int k = 0; k < unison_size[nvoice]; k++) {
        REALTYPE *tw = tmpwave_unison[k];
        for(int i = 0; i < SOUND_BUFFER_SIZE; i++)
            tw[i] = RND * 2.0 - 1.0;
    }
}



/*
 * Compute the ADnote samples
 * Returns 0 if the note is finished
 */
int ADnote::noteout(REALTYPE *outl, REALTYPE *outr)
{
    int i, nvoice;

    memcpy(outl, denormalkillbuf, SOUND_BUFFER_SIZE * sizeof(REALTYPE));
    memcpy(outr, denormalkillbuf, SOUND_BUFFER_SIZE * sizeof(REALTYPE));

    if(NoteEnabled == OFF)
        return 0;

    memset(bypassl, 0, SOUND_BUFFER_SIZE * sizeof(REALTYPE));
    memset(bypassr, 0, SOUND_BUFFER_SIZE * sizeof(REALTYPE));
    computecurrentparameters();

    for(nvoice = 0; nvoice < NUM_VOICES; nvoice++) {
        if((NoteVoicePar[nvoice].Enabled != ON)
           || (NoteVoicePar[nvoice].DelayTicks > 0))
            continue;
        if(NoteVoicePar[nvoice].noisetype == 0) //voice mode=sound
            switch(NoteVoicePar[nvoice].FMEnabled) {
            case MORPH:
                ComputeVoiceOscillatorMorph(nvoice);
                break;
            case RING_MOD:
                ComputeVoiceOscillatorRingModulation(nvoice);
                break;
            case PHASE_MOD:
                ComputeVoiceOscillatorFrequencyModulation(nvoice, 0);
                break;
            case FREQ_MOD:
                ComputeVoiceOscillatorFrequencyModulation(nvoice, 1);
                break;
            //case PITCH_MOD:ComputeVoiceOscillatorPitchModulation(nvoice);break;
            default:
                ComputeVoiceOscillator_LinearInterpolation(nvoice);
                //if (config.cfg.Interpolation) ComputeVoiceOscillator_CubicInterpolation(nvoice);
            }
        else
            ComputeVoiceNoise(nvoice);
        // Voice Processing


        //mix subvoices into voice
        memset(tmpwavel, 0, SOUND_BUFFER_SIZE * sizeof(REALTYPE));
        if(stereo)
            memset(tmpwaver, 0, SOUND_BUFFER_SIZE * sizeof(REALTYPE));
        for(int k = 0; k < unison_size[nvoice]; k++) {
            REALTYPE *tw = tmpwave_unison[k];
            if(stereo) {
                REALTYPE stereo_pos    = 0;
                if(unison_size[nvoice] > 1)
                    stereo_pos = k
                                 / (REALTYPE)(unison_size[nvoice]
                                              - 1) * 2.0 - 1.0;
                REALTYPE stereo_spread = unison_stereo_spread[nvoice] * 2.0; //between 0 and 2.0
                if(stereo_spread > 1.0) {
                    REALTYPE stereo_pos_1 = (stereo_pos >= 0.0) ? 1.0 : -1.0;
                    stereo_pos =
                        (2.0
                         - stereo_spread) * stereo_pos
                        + (stereo_spread - 1.0) * stereo_pos_1;
                }
                else
                    stereo_pos *= stereo_spread;
                ;
                if(unison_size[nvoice] == 1)
                    stereo_pos = 0.0;
                REALTYPE panning = (stereo_pos + 1.0) * 0.5;


                REALTYPE lvol = (1.0 - panning) * 2.0;
                if(lvol > 1.0)
                    lvol = 1.0;

                REALTYPE rvol = panning * 2.0;
                if(rvol > 1.0)
                    rvol = 1.0;

                if(unison_invert_phase[nvoice][k]) {
                    lvol = -lvol;
                    rvol = -rvol;
                }

                for(i = 0; i < SOUND_BUFFER_SIZE; i++)
                    tmpwavel[i] += tw[i] * lvol;
                for(i = 0; i < SOUND_BUFFER_SIZE; i++)
                    tmpwaver[i] += tw[i] * rvol;
            }
            else
                for(i = 0; i < SOUND_BUFFER_SIZE; i++)
                    tmpwavel[i] += tw[i];
            ;
        }


        REALTYPE unison_amplitude = 1.0 / sqrt(unison_size[nvoice]); //reduce the amplitude for large unison sizes
        // Amplitude
        REALTYPE oldam = oldamplitude[nvoice] * unison_amplitude;
        REALTYPE newam = newamplitude[nvoice] * unison_amplitude;

        if(ABOVE_AMPLITUDE_THRESHOLD(oldam, newam)) {
            int rest = SOUND_BUFFER_SIZE;
            //test if the amplitude if raising and the difference is high
            if((newam > oldam) && ((newam - oldam) > 0.25)) {
                rest = 10;
                if(rest > SOUND_BUFFER_SIZE)
                    rest = SOUND_BUFFER_SIZE;
                for(int i = 0; i < SOUND_BUFFER_SIZE - rest; i++)
                    tmpwavel[i] *= oldam;
                if(stereo)
                    for(int i = 0; i < SOUND_BUFFER_SIZE - rest; i++)
                        tmpwaver[i] *= oldam;
            }
            // Amplitude interpolation
            for(i = 0; i < rest; i++) {
                REALTYPE amp = INTERPOLATE_AMPLITUDE(oldam, newam, i, rest);
                tmpwavel[i + (SOUND_BUFFER_SIZE - rest)] *= amp;
                if(stereo)
                    tmpwaver[i + (SOUND_BUFFER_SIZE - rest)] *= amp;
            }
        }
        else {
            for(i = 0; i < SOUND_BUFFER_SIZE; i++)
                tmpwavel[i] *= newam;
            if(stereo)
                for(i = 0; i < SOUND_BUFFER_SIZE; i++)
                    tmpwaver[i] *= newam;
        }

        // Fade in
        if(firsttick[nvoice] != 0) {
            fadein(&tmpwavel[0]);
            if(stereo)
                fadein(&tmpwaver[0]);
            firsttick[nvoice] = 0;
        }


        // Filter
        if(NoteVoicePar[nvoice].VoiceFilterL != NULL)
            NoteVoicePar[nvoice].VoiceFilterL->filterout(&tmpwavel[0]);
        if((stereo) && (NoteVoicePar[nvoice].VoiceFilterR != NULL))
            NoteVoicePar[nvoice].VoiceFilterR->filterout(&tmpwaver[0]);

        //check if the amplitude envelope is finished, if yes, the voice will be fadeout
        if(NoteVoicePar[nvoice].AmpEnvelope != NULL) {
            if(NoteVoicePar[nvoice].AmpEnvelope->finished() != 0) {
                for(i = 0; i < SOUND_BUFFER_SIZE; i++)
                    tmpwavel[i] *= 1.0 - (REALTYPE)i
                                   / (REALTYPE)SOUND_BUFFER_SIZE;
                if(stereo)
                    for(i = 0; i < SOUND_BUFFER_SIZE; i++)
                        tmpwaver[i] *= 1.0 - (REALTYPE)i
                                       / (REALTYPE)SOUND_BUFFER_SIZE;
            }
            //the voice is killed later
        }


        // Put the ADnote samples in VoiceOut (without appling Global volume, because I wish to use this voice as a modullator)
        if(NoteVoicePar[nvoice].VoiceOut != NULL) {
            if(stereo)
                for(i = 0; i < SOUND_BUFFER_SIZE; i++)
                    NoteVoicePar[nvoice].VoiceOut[i] = tmpwavel[i]
                                                       + tmpwaver[i];
            else   //mono
                for(i = 0; i < SOUND_BUFFER_SIZE; i++)
                    NoteVoicePar[nvoice].VoiceOut[i] = tmpwavel[i];
            ;
        }


        // Add the voice that do not bypass the filter to out
        if(NoteVoicePar[nvoice].filterbypass == 0) { //no bypass
            if(stereo) {
                for(i = 0; i < SOUND_BUFFER_SIZE; i++) { //stereo
                    outl[i] += tmpwavel[i] * NoteVoicePar[nvoice].Volume
                               * NoteVoicePar[nvoice].Panning * 2.0;
                    outr[i] += tmpwaver[i] * NoteVoicePar[nvoice].Volume
                               * (1.0 - NoteVoicePar[nvoice].Panning) * 2.0;
                }
            }
            else
                for(i = 0; i < SOUND_BUFFER_SIZE; i++)
                    outl[i] += tmpwavel[i] * NoteVoicePar[nvoice].Volume;                          //mono
            ;
        }
        else {  //bypass the filter
            if(stereo) {
                for(i = 0; i < SOUND_BUFFER_SIZE; i++) { //stereo
                    bypassl[i] += tmpwavel[i] * NoteVoicePar[nvoice].Volume
                                  * NoteVoicePar[nvoice].Panning * 2.0;
                    bypassr[i] += tmpwaver[i] * NoteVoicePar[nvoice].Volume
                                  * (1.0 - NoteVoicePar[nvoice].Panning) * 2.0;
                }
            }
            else
                for(i = 0; i < SOUND_BUFFER_SIZE; i++)
                    bypassl[i] += tmpwavel[i] * NoteVoicePar[nvoice].Volume;                          //mono
            ;
        }
        // chech if there is necesary to proces the voice longer (if the Amplitude envelope isn't finished)
        if(NoteVoicePar[nvoice].AmpEnvelope != NULL)
            if(NoteVoicePar[nvoice].AmpEnvelope->finished() != 0)
                KillVoice(nvoice);
        ;
    }


    //Processing Global parameters
    NoteGlobalPar.GlobalFilterL->filterout(&outl[0]);

    if(stereo == 0) { //set the right channel=left channel
        memcpy(outr, outl, SOUND_BUFFER_SIZE * sizeof(REALTYPE));
        memcpy(bypassr, bypassl, SOUND_BUFFER_SIZE * sizeof(REALTYPE));
    }
    else
        NoteGlobalPar.GlobalFilterR->filterout(&outr[0]);

    for(i = 0; i < SOUND_BUFFER_SIZE; i++) {
        outl[i] += bypassl[i];
        outr[i] += bypassr[i];
    }

    if(ABOVE_AMPLITUDE_THRESHOLD(globaloldamplitude, globalnewamplitude)) {
        // Amplitude Interpolation
        for(i = 0; i < SOUND_BUFFER_SIZE; i++) {
            REALTYPE tmpvol = INTERPOLATE_AMPLITUDE(globaloldamplitude,
                                                    globalnewamplitude,
                                                    i,
                                                    SOUND_BUFFER_SIZE);
            outl[i] *= tmpvol * NoteGlobalPar.Panning;
            outr[i] *= tmpvol * (1.0 - NoteGlobalPar.Panning);
        }
    }
    else {
        for(i = 0; i < SOUND_BUFFER_SIZE; i++) {
            outl[i] *= globalnewamplitude * NoteGlobalPar.Panning;
            outr[i] *= globalnewamplitude * (1.0 - NoteGlobalPar.Panning);
        }
    }

//Apply the punch
    if(NoteGlobalPar.Punch.Enabled != 0) {
        for(i = 0; i < SOUND_BUFFER_SIZE; i++) {
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


    // Apply legato-specific sound signal modifications
    if(Legato.silent)    // Silencer
        if(Legato.msg != LM_FadeIn) {
            memset(outl, 0, SOUND_BUFFER_SIZE * sizeof(REALTYPE));
            memset(outr, 0, SOUND_BUFFER_SIZE * sizeof(REALTYPE));
        }
    switch(Legato.msg) {
    case LM_CatchUp:  // Continue the catch-up...
        if(Legato.decounter == -10)
            Legato.decounter = Legato.fade.length;
        for(i = 0; i < SOUND_BUFFER_SIZE; i++) { //Yea, could be done without the loop...
            Legato.decounter--;
            if(Legato.decounter < 1) {
                // Catching-up done, we can finally set
                // the note to the actual parameters.
                Legato.decounter = -10;
                Legato.msg = LM_ToNorm;
                ADlegatonote(Legato.param.freq,
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
        for(i = 0; i < SOUND_BUFFER_SIZE; i++) {
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
        for(i = 0; i < SOUND_BUFFER_SIZE; i++) {
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
                ADlegatonote(catchupfreq,
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
        for(i = 0; i < SOUND_BUFFER_SIZE; i++) { //fade-out
            REALTYPE tmp = 1.0 - (REALTYPE)i / (REALTYPE)SOUND_BUFFER_SIZE;
            outl[i] *= tmp;
            outr[i] *= tmp;
        }
        KillNote();
    }
    return 1;
}


/*
 * Relase the key (NoteOff)
 */
void ADnote::relasekey()
{
    int nvoice;
    for(nvoice = 0; nvoice < NUM_VOICES; nvoice++) {
        if(NoteVoicePar[nvoice].Enabled == 0)
            continue;
        if(NoteVoicePar[nvoice].AmpEnvelope != NULL)
            NoteVoicePar[nvoice].AmpEnvelope->relasekey();
        if(NoteVoicePar[nvoice].FreqEnvelope != NULL)
            NoteVoicePar[nvoice].FreqEnvelope->relasekey();
        if(NoteVoicePar[nvoice].FilterEnvelope != NULL)
            NoteVoicePar[nvoice].FilterEnvelope->relasekey();
        if(NoteVoicePar[nvoice].FMFreqEnvelope != NULL)
            NoteVoicePar[nvoice].FMFreqEnvelope->relasekey();
        if(NoteVoicePar[nvoice].FMAmpEnvelope != NULL)
            NoteVoicePar[nvoice].FMAmpEnvelope->relasekey();
    }
    NoteGlobalPar.FreqEnvelope->relasekey();
    NoteGlobalPar.FilterEnvelope->relasekey();
    NoteGlobalPar.AmpEnvelope->relasekey();
}

/*
 * Check if the note is finished
 */
int ADnote::finished() const
{
    if(NoteEnabled == ON)
        return 0;
    else
        return 1;
}

