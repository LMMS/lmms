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

#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cassert>
#include <stdint.h>

#include "../globals.h"
#include "../Misc/Util.h"
#include "../DSP/Filter.h"
#include "OscilGen.h"
#include "ADnote.h"


ADnote::ADnote(ADnoteParameters *pars,
               Controller *ctl_,
               float freq,
               float velocity,
               int portamento_,
               int midinote_,
               bool besilent)
    :SynthNote(freq, velocity, portamento_, midinote_, besilent)
{
    tmpwavel = new float [synth->buffersize];
    tmpwaver = new float [synth->buffersize];
    bypassl  = new float [synth->buffersize];
    bypassr  = new float [synth->buffersize];

    partparams = pars;
    ctl = ctl_;
    portamento  = portamento_;
    midinote    = midinote_;
    NoteEnabled = ON;
    basefreq    = freq;
    if(velocity > 1.0f)
        velocity = 1.0f;
    this->velocity = velocity;
    time   = 0.0f;
    stereo = pars->GlobalPar.PStereo;

    NoteGlobalPar.Detune = getdetune(pars->GlobalPar.PDetuneType,
                                     pars->GlobalPar.PCoarseDetune,
                                     pars->GlobalPar.PDetune);
    bandwidthDetuneMultiplier = pars->getBandwidthDetuneMultiplier();

    if(pars->GlobalPar.PPanning == 0)
        NoteGlobalPar.Panning = RND;
    else
        NoteGlobalPar.Panning = pars->GlobalPar.PPanning / 128.0f;


    NoteGlobalPar.FilterCenterPitch = pars->GlobalPar.GlobalFilter->getfreq() //center freq
                                      + pars->GlobalPar.PFilterVelocityScale
                                      / 127.0f * 6.0f                                  //velocity sensing
                                      * (VelF(velocity,
                                              pars->GlobalPar.
                                              PFilterVelocityScaleFunction) - 1);

    if(pars->GlobalPar.PPunchStrength != 0) {
        NoteGlobalPar.Punch.Enabled = 1;
        NoteGlobalPar.Punch.t = 1.0f; //start from 1.0f and to 0.0f
        NoteGlobalPar.Punch.initialvalue =
            ((powf(10, 1.5f * pars->GlobalPar.PPunchStrength / 127.0f) - 1.0f)
             * VelF(velocity,
                    pars->GlobalPar.PPunchVelocitySensing));
        float time =
            powf(10, 3.0f * pars->GlobalPar.PPunchTime / 127.0f) / 10000.0f;   //0.1f .. 100 ms
        float stretch = powf(440.0f / freq,
                             pars->GlobalPar.PPunchStretch / 64.0f);
        NoteGlobalPar.Punch.dt = 1.0f / (time * synth->samplerate_f * stretch);
    }
    else
        NoteGlobalPar.Punch.Enabled = 0;

    for(int nvoice = 0; nvoice < NUM_VOICES; ++nvoice) {
        pars->VoicePar[nvoice].OscilSmp->newrandseed(prng());
        NoteVoicePar[nvoice].OscilSmp = NULL;
        NoteVoicePar[nvoice].FMSmp    = NULL;
        NoteVoicePar[nvoice].VoiceOut = NULL;

        NoteVoicePar[nvoice].FMVoice = -1;
        unison_size[nvoice] = 1;

        if(pars->VoicePar[nvoice].Enabled == 0) {
            NoteVoicePar[nvoice].Enabled = OFF;
            continue; //the voice is disabled
        }

        unison_stereo_spread[nvoice] =
            pars->VoicePar[nvoice].Unison_stereo_spread / 127.0f;
        int unison = pars->VoicePar[nvoice].Unison_size;
        if(unison < 1)
            unison = 1;

        //compute unison
        unison_size[nvoice] = unison;

        unison_base_freq_rap[nvoice] = new float[unison];
        unison_freq_rap[nvoice]      = new float[unison];
        unison_invert_phase[nvoice]  = new bool[unison];
        float unison_spread = pars->getUnisonFrequencySpreadCents(
            nvoice);
        float unison_real_spread = powf(2.0f, (unison_spread * 0.5f) / 1200.0f);
        float unison_vibratto_a  = pars->VoicePar[nvoice].Unison_vibratto
                                   / 127.0f;                                     //0.0f .. 1.0f


        switch(unison) {
            case 1:
                unison_base_freq_rap[nvoice][0] = 1.0f; //if the unison is not used, always make the only subvoice to have the default note
                break;
            case 2: { //unison for 2 subvoices
                unison_base_freq_rap[nvoice][0] = 1.0f / unison_real_spread;
                unison_base_freq_rap[nvoice][1] = unison_real_spread;
            };
                break;
            default: { //unison for more than 2 subvoices
                float unison_values[unison];
                float min = -1e-6, max = 1e-6;
                for(int k = 0; k < unison; ++k) {
                    float step = (k / (float) (unison - 1)) * 2.0f - 1.0f; //this makes the unison spread more uniform
                    float val  = step + (RND * 2.0f - 1.0f) / (unison - 1);
                    unison_values[k] = val;
                    if (min > val) {
                        min = val;
                    }
                    if (max < val) {
                        max = val;
                    }
                }
                float diff = max - min;
                for(int k = 0; k < unison; ++k) {
                    unison_values[k] =
                        (unison_values[k] - (max + min) * 0.5f) / diff;             //the lowest value will be -1 and the highest will be 1
                    unison_base_freq_rap[nvoice][k] =
                        powf(2.0f, (unison_spread * unison_values[k]) / 1200);
                }
            };
        }

        //unison vibrattos
        if(unison > 1)
            for(int k = 0; k < unison; ++k) //reduce the frequency difference for larger vibrattos
                unison_base_freq_rap[nvoice][k] = 1.0f
                                                  + (unison_base_freq_rap[
                                                         nvoice][k] - 1.0f)
                                                  * (1.0f - unison_vibratto_a);
        unison_vibratto[nvoice].step      = new float[unison];
        unison_vibratto[nvoice].position  = new float[unison];
        unison_vibratto[nvoice].amplitude =
            (unison_real_spread - 1.0f) * unison_vibratto_a;

        float increments_per_second = synth->samplerate_f / synth->buffersize_f;
        float vibratto_base_period  = 0.25f
                                      * powf(
            2.0f,
            (1.0f
             - pars->VoicePar[nvoice].
             Unison_vibratto_speed
             / 127.0f) * 4.0f);
        for(int k = 0; k < unison; ++k) {
            unison_vibratto[nvoice].position[k] = RND * 1.8f - 0.9f;
            //make period to vary randomly from 50% to 200% vibratto base period
            float vibratto_period = vibratto_base_period
                                    * powf(2.0f, RND * 2.0f - 1.0f);

            float m = 4.0f / (vibratto_period * increments_per_second);
            if(RND < 0.5f)
                m = -m;
            unison_vibratto[nvoice].step[k] = m;
        }

        if(unison == 1) { //no vibratto for a single voice
            unison_vibratto[nvoice].step[0]     = 0.0f;
            unison_vibratto[nvoice].position[0] = 0.0f;
            unison_vibratto[nvoice].amplitude   = 0.0f;
        }

        //phase invert for unison
        unison_invert_phase[nvoice][0] = false;
        if(unison != 1) {
            int inv = pars->VoicePar[nvoice].Unison_invert_phase;
            switch(inv) {
                case 0: for(int k = 0; k < unison; ++k)
                        unison_invert_phase[nvoice][k] = false;
                    break;
                case 1: for(int k = 0; k < unison; ++k)
                        unison_invert_phase[nvoice][k] = (RND > 0.5f);
                    break;
                default: for(int k = 0; k < unison; ++k)
                        unison_invert_phase[nvoice][k] =
                            (k % inv == 0) ? true : false;
                    break;
            }
        }


        oscfreqhi[nvoice]   = new int[unison];
        oscfreqlo[nvoice]   = new float[unison];
        oscfreqhiFM[nvoice] = new unsigned int[unison];
        oscfreqloFM[nvoice] = new float[unison];
        oscposhi[nvoice]    = new int[unison];
        oscposlo[nvoice]    = new float[unison];
        oscposhiFM[nvoice]  = new unsigned int[unison];
        oscposloFM[nvoice]  = new float[unison];

        NoteVoicePar[nvoice].Enabled     = ON;
        NoteVoicePar[nvoice].fixedfreq   = pars->VoicePar[nvoice].Pfixedfreq;
        NoteVoicePar[nvoice].fixedfreqET = pars->VoicePar[nvoice].PfixedfreqET;

        //use the Globalpars.detunetype if the detunetype is 0
        if(pars->VoicePar[nvoice].PDetuneType != 0) {
            NoteVoicePar[nvoice].Detune = getdetune(
                pars->VoicePar[nvoice].PDetuneType,
                pars->VoicePar[nvoice].
                PCoarseDetune,
                8192); //coarse detune
            NoteVoicePar[nvoice].FineDetune = getdetune(
                pars->VoicePar[nvoice].PDetuneType,
                0,
                pars->VoicePar[nvoice].PDetune); //fine detune
        }
        else {
            NoteVoicePar[nvoice].Detune = getdetune(
                pars->GlobalPar.PDetuneType,
                pars->VoicePar[nvoice].
                PCoarseDetune,
                8192); //coarse detune
            NoteVoicePar[nvoice].FineDetune = getdetune(
                pars->GlobalPar.PDetuneType,
                0,
                pars->VoicePar[nvoice].PDetune); //fine detune
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



        for(int k = 0; k < unison; ++k) {
            oscposhi[nvoice][k]   = 0;
            oscposlo[nvoice][k]   = 0.0f;
            oscposhiFM[nvoice][k] = 0;
            oscposloFM[nvoice][k] = 0.0f;
        }

        //the extra points contains the first point
        NoteVoicePar[nvoice].OscilSmp =
            new float[synth->oscilsize + OSCIL_SMP_EXTRA_SAMPLES];

        //Get the voice's oscil or external's voice oscil
        int vc = nvoice;
        if(pars->VoicePar[nvoice].Pextoscil != -1)
            vc = pars->VoicePar[nvoice].Pextoscil;
        if(!pars->GlobalPar.Hrandgrouping)
            pars->VoicePar[vc].OscilSmp->newrandseed(prng());
        int oscposhi_start =
            pars->VoicePar[vc].OscilSmp->get(NoteVoicePar[nvoice].OscilSmp,
                                             getvoicebasefreq(nvoice),
                                             pars->VoicePar[nvoice].Presonance);

        //I store the first elments to the last position for speedups
        for(int i = 0; i < OSCIL_SMP_EXTRA_SAMPLES; ++i)
            NoteVoicePar[nvoice].OscilSmp[synth->oscilsize
                                          + i] =
                NoteVoicePar[nvoice].OscilSmp[i];

        oscposhi_start +=
            (int)((pars->VoicePar[nvoice].Poscilphase
                   - 64.0f) / 128.0f * synth->oscilsize + synth->oscilsize * 4);
        oscposhi_start %= synth->oscilsize;

        for(int k = 0; k < unison; ++k) {
            oscposhi[nvoice][k] = oscposhi_start;
            //put random starting point for other subvoices
            oscposhi_start      =
                (int)(RND * pars->VoicePar[nvoice].Unison_phase_randomness /
                        127.0f * (synth->oscilsize - 1));
        }

        NoteVoicePar[nvoice].FreqLfo      = NULL;
        NoteVoicePar[nvoice].FreqEnvelope = NULL;

        NoteVoicePar[nvoice].AmpLfo      = NULL;
        NoteVoicePar[nvoice].AmpEnvelope = NULL;

        NoteVoicePar[nvoice].VoiceFilterL   = NULL;
        NoteVoicePar[nvoice].VoiceFilterR   = NULL;
        NoteVoicePar[nvoice].FilterEnvelope = NULL;
        NoteVoicePar[nvoice].FilterLfo      = NULL;

        NoteVoicePar[nvoice].FilterCenterPitch =
            pars->VoicePar[nvoice].VoiceFilter->getfreq();
        NoteVoicePar[nvoice].filterbypass =
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
        float fmvoldamp = powf(440.0f / getvoicebasefreq(
                                   nvoice),
                               pars->VoicePar[nvoice].PFMVolumeDamp / 64.0f
                               - 1.0f);
        switch(NoteVoicePar[nvoice].FMEnabled) {
            case PHASE_MOD:
                fmvoldamp =
                    powf(440.0f / getvoicebasefreq(
                             nvoice), pars->VoicePar[nvoice].PFMVolumeDamp
                         / 64.0f);
                NoteVoicePar[nvoice].FMVolume =
                    (expf(pars->VoicePar[nvoice].PFMVolume / 127.0f
                          * FM_AMP_MULTIPLIER) - 1.0f) * fmvoldamp * 4.0f;
                break;
            case FREQ_MOD:
                NoteVoicePar[nvoice].FMVolume =
                    (expf(pars->VoicePar[nvoice].PFMVolume / 127.0f
                          * FM_AMP_MULTIPLIER) - 1.0f) * fmvoldamp * 4.0f;
                break;
            //    case PITCH_MOD:NoteVoicePar[nvoice].FMVolume=(pars->VoicePar[nvoice].PFMVolume/127.0f*8.0f)*fmvoldamp;//???????????
            //	          break;
            default:
                if(fmvoldamp > 1.0f)
                    fmvoldamp = 1.0f;
                NoteVoicePar[nvoice].FMVolume =
                    pars->VoicePar[nvoice].PFMVolume
                    / 127.0f * fmvoldamp;
        }

        //Voice's modulator velocity sensing
        NoteVoicePar[nvoice].FMVolume *=
            VelF(velocity,
                 partparams->VoicePar[nvoice].PFMVelocityScaleFunction);

        FMoldsmp[nvoice] = new float [unison];
        for(int k = 0; k < unison; ++k)
            FMoldsmp[nvoice][k] = 0.0f;                     //this is for FM (integration)

        firsttick[nvoice] = 1;
        NoteVoicePar[nvoice].DelayTicks =
            (int)((expf(pars->VoicePar[nvoice].PDelay / 127.0f
                        * logf(50.0f))
                   - 1.0f) / synth->buffersize_f / 10.0f * synth->samplerate_f);
    }

    max_unison = 1;
    for(int nvoice = 0; nvoice < NUM_VOICES; ++nvoice)
        if(unison_size[nvoice] > max_unison)
            max_unison = unison_size[nvoice];


    tmpwave_unison = new float *[max_unison];
    for(int k = 0; k < max_unison; ++k) {
        tmpwave_unison[k] = new float[synth->buffersize];
        memset(tmpwave_unison[k], 0, synth->bufferbytes);
    }

    initparameters();
}

// ADlegatonote: This function is (mostly) a copy of ADnote(...) and
// initparameters() stuck together with some lines removed so that it
// only alter the already playing note (to perform legato). It is
// possible I left stuff that is not required for this.
void ADnote::legatonote(float freq, float velocity, int portamento_,
                        int midinote_, bool externcall)
{
    ADnoteParameters *pars = partparams;

    // Manage legato stuff
    if(legato.update(freq, velocity, portamento_, midinote_, externcall))
        return;

    portamento = portamento_;
    midinote   = midinote_;
    basefreq   = freq;

    if(velocity > 1.0f)
        velocity = 1.0f;
    this->velocity = velocity;

    NoteGlobalPar.Detune = getdetune(pars->GlobalPar.PDetuneType,
                                     pars->GlobalPar.PCoarseDetune,
                                     pars->GlobalPar.PDetune);
    bandwidthDetuneMultiplier = pars->getBandwidthDetuneMultiplier();

    if(pars->GlobalPar.PPanning == 0)
        NoteGlobalPar.Panning = RND;
    else
        NoteGlobalPar.Panning = pars->GlobalPar.PPanning / 128.0f;

    //center freq
    NoteGlobalPar.FilterCenterPitch = pars->GlobalPar.GlobalFilter->getfreq()
                                      + pars->GlobalPar.PFilterVelocityScale
                                      / 127.0f * 6.0f          //velocity sensing
                                      * (VelF(velocity,
                                              pars->GlobalPar.
                                              PFilterVelocityScaleFunction) - 1);


    for(int nvoice = 0; nvoice < NUM_VOICES; ++nvoice) {
        if(NoteVoicePar[nvoice].Enabled == OFF)
            continue;  //(gf) Stay the same as first note in legato.

        NoteVoicePar[nvoice].fixedfreq   = pars->VoicePar[nvoice].Pfixedfreq;
        NoteVoicePar[nvoice].fixedfreqET = pars->VoicePar[nvoice].PfixedfreqET;

        //use the Globalpars.detunetype if the detunetype is 0
        if(pars->VoicePar[nvoice].PDetuneType != 0) {
            NoteVoicePar[nvoice].Detune = getdetune(
                pars->VoicePar[nvoice].PDetuneType,
                pars->VoicePar[nvoice].PCoarseDetune,
                8192); //coarse detune
            NoteVoicePar[nvoice].FineDetune = getdetune(
                pars->VoicePar[nvoice].PDetuneType,
                0,
                pars->VoicePar[nvoice].PDetune); //fine detune
        }
        else {
            NoteVoicePar[nvoice].Detune = getdetune(
                pars->GlobalPar.PDetuneType,
                pars->VoicePar[nvoice].PCoarseDetune,
                8192); //coarse detune
            NoteVoicePar[nvoice].FineDetune = getdetune(
                pars->GlobalPar.PDetuneType,
                0,
                pars->VoicePar[nvoice].PDetune); //fine detune
        }
        if(pars->VoicePar[nvoice].PFMDetuneType != 0)
            NoteVoicePar[nvoice].FMDetune = getdetune(
                pars->VoicePar[nvoice].PFMDetuneType,
                pars->VoicePar[nvoice].PFMCoarseDetune,
                pars->VoicePar[nvoice].PFMDetune);
        else
            NoteVoicePar[nvoice].FMDetune = getdetune(
                pars->GlobalPar.PDetuneType,
                pars->VoicePar[nvoice].PFMCoarseDetune,
                pars->VoicePar[nvoice].PFMDetune);


        //Get the voice's oscil or external's voice oscil
        int vc = nvoice;
        if(pars->VoicePar[nvoice].Pextoscil != -1)
            vc = pars->VoicePar[nvoice].Pextoscil;
        if(!pars->GlobalPar.Hrandgrouping)
            pars->VoicePar[vc].OscilSmp->newrandseed(prng());

        pars->VoicePar[vc].OscilSmp->get(NoteVoicePar[nvoice].OscilSmp,
                                         getvoicebasefreq(nvoice),
                                         pars->VoicePar[nvoice].Presonance); //(gf)Modif of the above line.

        //I store the first elments to the last position for speedups
        for(int i = 0; i < OSCIL_SMP_EXTRA_SAMPLES; ++i)
            NoteVoicePar[nvoice].OscilSmp[synth->oscilsize
                                          + i] =
                NoteVoicePar[nvoice].OscilSmp[i];


        NoteVoicePar[nvoice].FilterCenterPitch =
            pars->VoicePar[nvoice].VoiceFilter->getfreq();
        NoteVoicePar[nvoice].filterbypass =
            pars->VoicePar[nvoice].Pfilterbypass;


        NoteVoicePar[nvoice].FMVoice = pars->VoicePar[nvoice].PFMVoice;

        //Compute the Voice's modulator volume (incl. damping)
        float fmvoldamp = powf(440.0f / getvoicebasefreq(nvoice),
                               pars->VoicePar[nvoice].PFMVolumeDamp / 64.0f
                               - 1.0f);

        switch(NoteVoicePar[nvoice].FMEnabled) {
            case PHASE_MOD:
                fmvoldamp =
                    powf(440.0f / getvoicebasefreq(
                             nvoice), pars->VoicePar[nvoice].PFMVolumeDamp
                         / 64.0f);
                NoteVoicePar[nvoice].FMVolume =
                    (expf(pars->VoicePar[nvoice].PFMVolume / 127.0f
                          * FM_AMP_MULTIPLIER) - 1.0f) * fmvoldamp * 4.0f;
                break;
            case FREQ_MOD:
                NoteVoicePar[nvoice].FMVolume =
                    (expf(pars->VoicePar[nvoice].PFMVolume / 127.0f
                          * FM_AMP_MULTIPLIER) - 1.0f) * fmvoldamp * 4.0f;
                break;
            //    case PITCH_MOD:NoteVoicePar[nvoice].FMVolume=(pars->VoicePar[nvoice].PFMVolume/127.0f*8.0f)*fmvoldamp;//???????????
            //	          break;
            default:
                if(fmvoldamp > 1.0f)
                    fmvoldamp = 1.0f;
                NoteVoicePar[nvoice].FMVolume =
                    pars->VoicePar[nvoice].PFMVolume
                    / 127.0f * fmvoldamp;
        }

        //Voice's modulator velocity sensing
        NoteVoicePar[nvoice].FMVolume *=
            VelF(velocity,
                 partparams->VoicePar[nvoice].PFMVelocityScaleFunction);

        NoteVoicePar[nvoice].DelayTicks =
            (int)((expf(pars->VoicePar[nvoice].PDelay / 127.0f
                        * logf(50.0f))
                   - 1.0f) / synth->buffersize_f / 10.0f * synth->samplerate_f);
    }

    ///    initparameters();

    ///////////////
    // Altered content of initparameters():

    int tmp[NUM_VOICES];

    NoteGlobalPar.Volume = 4.0f
                           * powf(0.1f, 3.0f
                                  * (1.0f - partparams->GlobalPar.PVolume
                                     / 96.0f))                                      //-60 dB .. 0 dB
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
    for(int i = 0; i < NUM_VOICES; ++i)
        if(NoteVoicePar[i].FMVoice >= i)
            NoteVoicePar[i].FMVoice = -1;

    // Voice Parameter init
    for(unsigned nvoice = 0; nvoice < NUM_VOICES; ++nvoice) {
        if(NoteVoicePar[nvoice].Enabled == 0)
            continue;

        NoteVoicePar[nvoice].noisetype = partparams->VoicePar[nvoice].Type;
        /* Voice Amplitude Parameters Init */
        NoteVoicePar[nvoice].Volume =
            powf(0.1f, 3.0f
                 * (1.0f - partparams->VoicePar[nvoice].PVolume / 127.0f))             // -60 dB .. 0 dB
            * VelF(velocity,
                   partparams->VoicePar[nvoice].PAmpVelocityScaleFunction); //velocity

        if(partparams->VoicePar[nvoice].PVolumeminus != 0)
            NoteVoicePar[nvoice].Volume = -NoteVoicePar[nvoice].Volume;

        if(partparams->VoicePar[nvoice].PPanning == 0)
            NoteVoicePar[nvoice].Panning = RND;  // random panning
        else
            NoteVoicePar[nvoice].Panning =
                partparams->VoicePar[nvoice].PPanning / 128.0f;

        newamplitude[nvoice] = 1.0f;
        if((partparams->VoicePar[nvoice].PAmpEnvelopeEnabled != 0)
           && (NoteVoicePar[nvoice].AmpEnvelope != NULL))
            newamplitude[nvoice] *= NoteVoicePar[nvoice].AmpEnvelope->envout_dB();


        if((partparams->VoicePar[nvoice].PAmpLfoEnabled != 0)
           && (NoteVoicePar[nvoice].AmpLfo != NULL))
            newamplitude[nvoice] *= NoteVoicePar[nvoice].AmpLfo->amplfoout();



        NoteVoicePar[nvoice].FilterFreqTracking =
            partparams->VoicePar[nvoice].VoiceFilter->getfreqtracking(basefreq);

        /* Voice Modulation Parameters Init */
        if((NoteVoicePar[nvoice].FMEnabled != NONE)
           && (NoteVoicePar[nvoice].FMVoice < 0)) {
            partparams->VoicePar[nvoice].FMSmp->newrandseed(prng());

            //Perform Anti-aliasing only on MORPH or RING MODULATION

            int vc = nvoice;
            if(partparams->VoicePar[nvoice].PextFMoscil != -1)
                vc = partparams->VoicePar[nvoice].PextFMoscil;

            if(!partparams->GlobalPar.Hrandgrouping)
                partparams->VoicePar[vc].FMSmp->newrandseed(prng());

            for(int i = 0; i < OSCIL_SMP_EXTRA_SAMPLES; ++i)
                NoteVoicePar[nvoice].FMSmp[synth->oscilsize + i] =
                    NoteVoicePar[nvoice].FMSmp[i];
        }

        FMnewamplitude[nvoice] = NoteVoicePar[nvoice].FMVolume
                                 * ctl->fmamp.relamp;

        if((partparams->VoicePar[nvoice].PFMAmpEnvelopeEnabled != 0)
           && (NoteVoicePar[nvoice].FMAmpEnvelope != NULL))
            FMnewamplitude[nvoice] *=
                NoteVoicePar[nvoice].FMAmpEnvelope->envout_dB();
    }

    for(int nvoice = 0; nvoice < NUM_VOICES; ++nvoice) {
        for(unsigned i = nvoice + 1; i < NUM_VOICES; ++i)
            tmp[i] = 0;
        for(unsigned i = nvoice + 1; i < NUM_VOICES; ++i)
            if((NoteVoicePar[i].FMVoice == nvoice) && (tmp[i] == 0))
                tmp[i] = 1;
    }
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

    delete [] unison_base_freq_rap[nvoice];
    delete [] unison_freq_rap[nvoice];
    delete [] unison_invert_phase[nvoice];
    delete [] FMoldsmp[nvoice];
    delete [] unison_vibratto[nvoice].step;
    delete [] unison_vibratto[nvoice].position;

    NoteVoicePar[nvoice].kill();
}

/*
 * Kill the note
 */
void ADnote::KillNote()
{
    for(unsigned nvoice = 0; nvoice < NUM_VOICES; ++nvoice) {
        if(NoteVoicePar[nvoice].Enabled == ON)
            KillVoice(nvoice);

        if(NoteVoicePar[nvoice].VoiceOut)
            delete NoteVoicePar[nvoice].VoiceOut;
        NoteVoicePar[nvoice].VoiceOut = NULL;
    }

    NoteGlobalPar.kill();

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
    for(int k = 0; k < max_unison; ++k)
        delete[] tmpwave_unison[k];
    delete[] tmpwave_unison;
}


/*
 * Init the parameters
 */
void ADnote::initparameters()
{
    int tmp[NUM_VOICES];

    // Global Parameters
    NoteGlobalPar.initparameters(partparams->GlobalPar, basefreq, velocity,
                                 stereo);

    NoteGlobalPar.AmpEnvelope->envout_dB(); //discard the first envelope output
    globalnewamplitude = NoteGlobalPar.Volume
                         * NoteGlobalPar.AmpEnvelope->envout_dB()
                         * NoteGlobalPar.AmpLfo->amplfoout();

    // Forbids the Modulation Voice to be greater or equal than voice
    for(int i = 0; i < NUM_VOICES; ++i)
        if(NoteVoicePar[i].FMVoice >= i)
            NoteVoicePar[i].FMVoice = -1;

    // Voice Parameter init
    for(int nvoice = 0; nvoice < NUM_VOICES; ++nvoice) {
        Voice &vce = NoteVoicePar[nvoice];
        ADnoteVoiceParam &param = partparams->VoicePar[nvoice];

        if(vce.Enabled == 0)
            continue;

        vce.noisetype = param.Type;
        /* Voice Amplitude Parameters Init */
        vce.Volume = powf(0.1f, 3.0f * (1.0f - param.PVolume / 127.0f)) // -60dB..0dB
                     * VelF(velocity, param.PAmpVelocityScaleFunction);

        if(param.PVolumeminus)
            vce.Volume = -vce.Volume;

        if(param.PPanning == 0)
            vce.Panning = RND;  // random panning
        else
            vce.Panning = param.PPanning / 128.0f;

        newamplitude[nvoice] = 1.0f;
        if(param.PAmpEnvelopeEnabled) {
            vce.AmpEnvelope = new Envelope(param.AmpEnvelope, basefreq);
            vce.AmpEnvelope->envout_dB(); //discard the first envelope sample
            newamplitude[nvoice] *= vce.AmpEnvelope->envout_dB();
        }

        if(param.PAmpLfoEnabled) {
            vce.AmpLfo = new LFO(param.AmpLfo, basefreq);
            newamplitude[nvoice] *= vce.AmpLfo->amplfoout();
        }

        /* Voice Frequency Parameters Init */
        if(param.PFreqEnvelopeEnabled != 0)
            vce.FreqEnvelope = new Envelope(param.FreqEnvelope, basefreq);

        if(param.PFreqLfoEnabled != 0)
            vce.FreqLfo = new LFO(param.FreqLfo, basefreq);

        /* Voice Filter Parameters Init */
        if(param.PFilterEnabled != 0) {
            vce.VoiceFilterL = Filter::generate(param.VoiceFilter);
            vce.VoiceFilterR = Filter::generate(param.VoiceFilter);
        }

        if(param.PFilterEnvelopeEnabled != 0)
            vce.FilterEnvelope = new Envelope(param.FilterEnvelope, basefreq);

        if(param.PFilterLfoEnabled != 0)
            vce.FilterLfo = new LFO(param.FilterLfo, basefreq);

        vce.FilterFreqTracking =
            param.VoiceFilter->getfreqtracking(basefreq);

        /* Voice Modulation Parameters Init */
        if((vce.FMEnabled != NONE) && (vce.FMVoice < 0)) {
            param.FMSmp->newrandseed(prng());
            vce.FMSmp = new float[synth->oscilsize + OSCIL_SMP_EXTRA_SAMPLES];

            //Perform Anti-aliasing only on MORPH or RING MODULATION

            int vc = nvoice;
            if(param.PextFMoscil != -1)
                vc = param.PextFMoscil;

            float tmp = 1.0f;
            if((partparams->VoicePar[vc].FMSmp->Padaptiveharmonics != 0)
               || (vce.FMEnabled == MORPH)
               || (vce.FMEnabled == RING_MOD))
                tmp = getFMvoicebasefreq(nvoice);

            if(!partparams->GlobalPar.Hrandgrouping)
                partparams->VoicePar[vc].FMSmp->newrandseed(prng());

            for(int k = 0; k < unison_size[nvoice]; ++k)
                oscposhiFM[nvoice][k] = (oscposhi[nvoice][k]
                                         + partparams->VoicePar[vc].FMSmp->get(
                                             vce.FMSmp, tmp))
                                        % synth->oscilsize;

            for(int i = 0; i < OSCIL_SMP_EXTRA_SAMPLES; ++i)
                vce.FMSmp[synth->oscilsize + i] = vce.FMSmp[i];
            int oscposhiFM_add =
                (int)((param.PFMoscilphase
                       - 64.0f) / 128.0f * synth->oscilsize
                      + synth->oscilsize * 4);
            for(int k = 0; k < unison_size[nvoice]; ++k) {
                oscposhiFM[nvoice][k] += oscposhiFM_add;
                oscposhiFM[nvoice][k] %= synth->oscilsize;
            }
        }

        if(param.PFMFreqEnvelopeEnabled != 0)
            vce.FMFreqEnvelope = new Envelope(param.FMFreqEnvelope, basefreq);

        FMnewamplitude[nvoice] = vce.FMVolume * ctl->fmamp.relamp;

        if(param.PFMAmpEnvelopeEnabled != 0) {
            vce.FMAmpEnvelope = new Envelope(param.FMAmpEnvelope,
                                             basefreq);
            FMnewamplitude[nvoice] *= vce.FMAmpEnvelope->envout_dB();
        }
    }

    for(int nvoice = 0; nvoice < NUM_VOICES; ++nvoice) {
        for(int i = nvoice + 1; i < NUM_VOICES; ++i)
            tmp[i] = 0;
        for(int i = nvoice + 1; i < NUM_VOICES; ++i)
            if((NoteVoicePar[i].FMVoice == nvoice) && (tmp[i] == 0)) {
                NoteVoicePar[nvoice].VoiceOut = new float[synth->buffersize];
                tmp[i] = 1;
            }

        if(NoteVoicePar[nvoice].VoiceOut)
            memset(NoteVoicePar[nvoice].VoiceOut, 0, synth->bufferbytes);
    }
}


/*
 * Computes the relative frequency of each unison voice and it's vibratto
 * This must be called before setfreq* functions
 */
void ADnote::compute_unison_freq_rap(int nvoice) {
    if(unison_size[nvoice] == 1) { //no unison
        unison_freq_rap[nvoice][0] = 1.0f;
        return;
    }
    float relbw = ctl->bandwidth.relbw * bandwidthDetuneMultiplier;
    for(int k = 0; k < unison_size[nvoice]; ++k) {
        float pos  = unison_vibratto[nvoice].position[k];
        float step = unison_vibratto[nvoice].step[k];
        pos += step;
        if(pos <= -1.0f) {
            pos  = -1.0f;
            step = -step;
        }
        if(pos >= 1.0f) {
            pos  = 1.0f;
            step = -step;
        }
        float vibratto_val = (pos - 0.333333333f * pos * pos * pos) * 1.5f; //make the vibratto lfo smoother
        unison_freq_rap[nvoice][k] = 1.0f
                                     + ((unison_base_freq_rap[nvoice][k]
                                         - 1.0f) + vibratto_val
                                        * unison_vibratto[nvoice].amplitude)
                                     * relbw;

        unison_vibratto[nvoice].position[k] = pos;
        step = unison_vibratto[nvoice].step[k] = step;
    }
}


/*
 * Computes the frequency of an oscillator
 */
void ADnote::setfreq(int nvoice, float in_freq)
{
    for(int k = 0; k < unison_size[nvoice]; ++k) {
        float freq  = fabs(in_freq) * unison_freq_rap[nvoice][k];
        float speed = freq * synth->oscilsize_f / synth->samplerate_f;
        if(speed > synth->oscilsize_f)
            speed = synth->oscilsize_f;

        F2I(speed, oscfreqhi[nvoice][k]);
        oscfreqlo[nvoice][k] = speed - floor(speed);
    }
}

/*
 * Computes the frequency of an modullator oscillator
 */
void ADnote::setfreqFM(int nvoice, float in_freq)
{
    for(int k = 0; k < unison_size[nvoice]; ++k) {
        float freq  = fabs(in_freq) * unison_freq_rap[nvoice][k];
        float speed = freq * synth->oscilsize_f / synth->samplerate_f;
        if(speed > synth->samplerate_f)
            speed = synth->samplerate_f;

        F2I(speed, oscfreqhiFM[nvoice][k]);
        oscfreqloFM[nvoice][k] = speed - floor(speed);
    }
}

/*
 * Get Voice base frequency
 */
float ADnote::getvoicebasefreq(int nvoice) const
{
    float detune = NoteVoicePar[nvoice].Detune / 100.0f
                   + NoteVoicePar[nvoice].FineDetune / 100.0f
                   * ctl->bandwidth.relbw * bandwidthDetuneMultiplier
                   + NoteGlobalPar.Detune / 100.0f;

    if(NoteVoicePar[nvoice].fixedfreq == 0)
        return this->basefreq * powf(2, detune / 12.0f);
    else { //the fixed freq is enabled
        float fixedfreq   = 440.0f;
        int   fixedfreqET = NoteVoicePar[nvoice].fixedfreqET;
        if(fixedfreqET != 0) { //if the frequency varies according the keyboard note
            float tmp =
                (midinote
                 - 69.0f) / 12.0f
                * (powf(2.0f, (fixedfreqET - 1) / 63.0f) - 1.0f);
            if(fixedfreqET <= 64)
                fixedfreq *= powf(2.0f, tmp);
            else
                fixedfreq *= powf(3.0f, tmp);
        }
        return fixedfreq * powf(2.0f, detune / 12.0f);
    }
}

/*
 * Get Voice's Modullator base frequency
 */
float ADnote::getFMvoicebasefreq(int nvoice) const
{
    float detune = NoteVoicePar[nvoice].FMDetune / 100.0f;
    return getvoicebasefreq(nvoice) * powf(2, detune / 12.0f);
}

/*
 * Computes all the parameters for each tick
 */
void ADnote::computecurrentparameters()
{
    int   nvoice;
    float voicefreq, voicepitch, filterpitch, filterfreq, FMfreq,
          FMrelativepitch, globalpitch, globalfilterpitch;
    globalpitch = 0.01f * (NoteGlobalPar.FreqEnvelope->envout()
                           + NoteGlobalPar.FreqLfo->lfoout()
                           * ctl->modwheel.relmod);
    globaloldamplitude = globalnewamplitude;
    globalnewamplitude = NoteGlobalPar.Volume
                         * NoteGlobalPar.AmpEnvelope->envout_dB()
                         * NoteGlobalPar.AmpLfo->amplfoout();

    globalfilterpitch = NoteGlobalPar.FilterEnvelope->envout()
                        + NoteGlobalPar.FilterLfo->lfoout()
                        + NoteGlobalPar.FilterCenterPitch;

    float tmpfilterfreq = globalfilterpitch + ctl->filtercutoff.relfreq
                          + NoteGlobalPar.FilterFreqTracking;

    tmpfilterfreq = Filter::getrealfreq(tmpfilterfreq);

    float globalfilterq = NoteGlobalPar.FilterQ * ctl->filterq.relq;
    NoteGlobalPar.GlobalFilterL->setfreq_and_q(tmpfilterfreq, globalfilterq);
    if(stereo != 0)
        NoteGlobalPar.GlobalFilterR->setfreq_and_q(tmpfilterfreq, globalfilterq);

    //compute the portamento, if it is used by this note
    float portamentofreqrap = 1.0f;
    if(portamento != 0) { //this voice use portamento
        portamentofreqrap = ctl->portamento.freqrap;
        if(ctl->portamento.used == 0) //the portamento has finished
            portamento = 0;  //this note is no longer "portamented"
    }

    //compute parameters for all voices
    for(nvoice = 0; nvoice < NUM_VOICES; ++nvoice) {
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
        newamplitude[nvoice] = 1.0f;

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
            filterfreq = Filter::getrealfreq(filterfreq);

            NoteVoicePar[nvoice].VoiceFilterL->setfreq(filterfreq);
            if(stereo && NoteVoicePar[nvoice].VoiceFilterR)
                NoteVoicePar[nvoice].VoiceFilterR->setfreq(filterfreq);
        }

        if(NoteVoicePar[nvoice].noisetype == 0) { //compute only if the voice isn't noise
            /*******************/
            /* Voice Frequency */
            /*******************/
            voicepitch = 0.0f;
            if(NoteVoicePar[nvoice].FreqLfo != NULL)
                voicepitch += NoteVoicePar[nvoice].FreqLfo->lfoout() / 100.0f
                              * ctl->bandwidth.relbw;

            if(NoteVoicePar[nvoice].FreqEnvelope != NULL)
                voicepitch += NoteVoicePar[nvoice].FreqEnvelope->envout()
                              / 100.0f;
            voicefreq = getvoicebasefreq(nvoice)
                        * powf(2, (voicepitch + globalpitch) / 12.0f);                //Hz frequency
            voicefreq *= ctl->pitchwheel.relfreq; //change the frequency by the controller
            setfreq(nvoice, voicefreq * portamentofreqrap);

            /***************/
            /*  Modulator */
            /***************/
            if(NoteVoicePar[nvoice].FMEnabled != NONE) {
                FMrelativepitch = NoteVoicePar[nvoice].FMDetune / 100.0f;
                if(NoteVoicePar[nvoice].FMFreqEnvelope != NULL)
                    FMrelativepitch +=
                        NoteVoicePar[nvoice].FMFreqEnvelope->envout() / 100;
                FMfreq =
                    powf(2.0f, FMrelativepitch
                         / 12.0f) * voicefreq * portamentofreqrap;
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
    time += synth->buffersize_f / synth->samplerate_f;
}


/*
 * Fadein in a way that removes clicks but keep sound "punchy"
 */
inline void ADnote::fadein(float *smps) const
{
    int zerocrossings = 0;
    for(int i = 1; i < synth->buffersize; ++i)
        if((smps[i - 1] < 0.0f) && (smps[i] > 0.0f))
            zerocrossings++;  //this is only the possitive crossings

    float tmp = (synth->buffersize_f - 1.0f) / (zerocrossings + 1) / 3.0f;
    if(tmp < 8.0f)
        tmp = 8.0f;

    int n;
    F2I(tmp, n); //how many samples is the fade-in
    if(n > synth->buffersize)
        n = synth->buffersize;
    for(int i = 0; i < n; ++i) { //fade-in
        float tmp = 0.5f - cosf((float)i / (float) n * PI) * 0.5f;
        smps[i] *= tmp;
    }
}

/*
 * Computes the Oscillator (Without Modulation) - LinearInterpolation
 */

/* As the code here is a bit odd due to optimization, here is what happens
 * First the current possition and frequency are retrieved from the running
 * state. These are broken up into high and low portions to indicate how many
 * samples are skipped in one step and how many fractional samples are skipped.
 * Outside of this method the fractional samples are just handled with floating
 * point code, but that's a bit slower than it needs to be. In this code the low
 * portions are known to exist between 0.0 and 1.0 and it is known that they are
 * stored in single precision floating point IEEE numbers. This implies that
 * a maximum of 24 bits are significant. The below code does your standard
 * linear interpolation that you'll see throughout this codebase, but by
 * sticking to integers for tracking the overflow of the low portion, around 15%
 * of the execution time was shaved off in the ADnote test.
 */
inline void ADnote::ComputeVoiceOscillator_LinearInterpolation(int nvoice)
{
    for(int k = 0; k < unison_size[nvoice]; ++k) {
        int    poshi  = oscposhi[nvoice][k];
        int    poslo  = oscposlo[nvoice][k] * (1<<24);
        int    freqhi = oscfreqhi[nvoice][k];
        int    freqlo = oscfreqlo[nvoice][k] * (1<<24);
        float *smps   = NoteVoicePar[nvoice].OscilSmp;
        float *tw     = tmpwave_unison[k];
        assert(oscfreqlo[nvoice][k] < 1.0f);
        for(int i = 0; i < synth->buffersize; ++i) {
            tw[i]  = (smps[poshi] * ((1<<24) - poslo) + smps[poshi + 1] * poslo)/(1.0f*(1<<24));
            poslo += freqlo;
            poshi += freqhi + (poslo>>24);
            poslo &= 0xffffff;
            poshi &= synth->oscilsize - 1;
        }
        oscposhi[nvoice][k] = poshi;
        oscposlo[nvoice][k] = poslo/(1.0f*(1<<24));
    }
}



/*
 * Computes the Oscillator (Without Modulation) - CubicInterpolation
 *
 The differences from the Linear are to little to deserve to be used. This is because I am using a large synth->oscilsize (>512)
inline void ADnote::ComputeVoiceOscillator_CubicInterpolation(int nvoice){
    int i,poshi;
    float poslo;

    poshi=oscposhi[nvoice];
    poslo=oscposlo[nvoice];
    float *smps=NoteVoicePar[nvoice].OscilSmp;
    float xm1,x0,x1,x2,a,b,c;
    for (i=0;i<synth->buffersize;i++){
    xm1=smps[poshi];
    x0=smps[poshi+1];
    x1=smps[poshi+2];
    x2=smps[poshi+3];
    a=(3.0f * (x0-x1) - xm1 + x2) / 2.0f;
    b = 2.0f*x1 + xm1 - (5.0f*x0 + x2) / 2.0f;
    c = (x1 - xm1) / 2.0f;
    tmpwave[i]=(((a * poslo) + b) * poslo + c) * poslo + x0;
    printf("a\n");
    //tmpwave[i]=smps[poshi]*(1.0f-poslo)+smps[poshi+1]*poslo;
    poslo+=oscfreqlo[nvoice];
    if (poslo>=1.0f) {
            poslo-=1.0f;
        poshi++;
    };
        poshi+=oscfreqhi[nvoice];
        poshi&=synth->oscilsize-1;
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
    int   i;
    float amp;
    ComputeVoiceOscillator_LinearInterpolation(nvoice);
    if(FMnewamplitude[nvoice] > 1.0f)
        FMnewamplitude[nvoice] = 1.0f;
    if(FMoldamplitude[nvoice] > 1.0f)
        FMoldamplitude[nvoice] = 1.0f;

    if(NoteVoicePar[nvoice].FMVoice >= 0) {
        //if I use VoiceOut[] as modullator
        int FMVoice = NoteVoicePar[nvoice].FMVoice;
        for(int k = 0; k < unison_size[nvoice]; ++k) {
            float *tw = tmpwave_unison[k];
            for(i = 0; i < synth->buffersize; ++i) {
                amp = INTERPOLATE_AMPLITUDE(FMoldamplitude[nvoice],
                                            FMnewamplitude[nvoice],
                                            i,
                                            synth->buffersize);
                tw[i] = tw[i]
                        * (1.0f
                           - amp) + amp * NoteVoicePar[FMVoice].VoiceOut[i];
            }
        }
    }
    else
        for(int k = 0; k < unison_size[nvoice]; ++k) {
            int    poshiFM  = oscposhiFM[nvoice][k];
            float  posloFM  = oscposloFM[nvoice][k];
            int    freqhiFM = oscfreqhiFM[nvoice][k];
            float  freqloFM = oscfreqloFM[nvoice][k];
            float *tw = tmpwave_unison[k];

            for(i = 0; i < synth->buffersize; ++i) {
                amp = INTERPOLATE_AMPLITUDE(FMoldamplitude[nvoice],
                                            FMnewamplitude[nvoice],
                                            i,
                                            synth->buffersize);
                tw[i] = tw[i] * (1.0f - amp) + amp
                        * (NoteVoicePar[nvoice].FMSmp[poshiFM] * (1 - posloFM)
                           + NoteVoicePar[nvoice].FMSmp[poshiFM + 1] * posloFM);
                posloFM += freqloFM;
                if(posloFM >= 1.0f) {
                    posloFM -= 1.0f;
                    poshiFM++;
                }
                poshiFM += freqhiFM;
                poshiFM &= synth->oscilsize - 1;
            }
            oscposhiFM[nvoice][k] = poshiFM;
            oscposloFM[nvoice][k] = posloFM;
        }
}

/*
 * Computes the Oscillator (Ring Modulation)
 */
inline void ADnote::ComputeVoiceOscillatorRingModulation(int nvoice)
{
    int   i;
    float amp;
    ComputeVoiceOscillator_LinearInterpolation(nvoice);
    if(FMnewamplitude[nvoice] > 1.0f)
        FMnewamplitude[nvoice] = 1.0f;
    if(FMoldamplitude[nvoice] > 1.0f)
        FMoldamplitude[nvoice] = 1.0f;
    if(NoteVoicePar[nvoice].FMVoice >= 0)
        // if I use VoiceOut[] as modullator
        for(int k = 0; k < unison_size[nvoice]; ++k) {
            float *tw = tmpwave_unison[k];
            for(i = 0; i < synth->buffersize; ++i) {
                amp = INTERPOLATE_AMPLITUDE(FMoldamplitude[nvoice],
                                            FMnewamplitude[nvoice],
                                            i,
                                            synth->buffersize);
                int FMVoice = NoteVoicePar[nvoice].FMVoice;
                tw[i] *= (1.0f - amp) + amp * NoteVoicePar[FMVoice].VoiceOut[i];
            }
        }
    else
        for(int k = 0; k < unison_size[nvoice]; ++k) {
            int    poshiFM  = oscposhiFM[nvoice][k];
            float  posloFM  = oscposloFM[nvoice][k];
            int    freqhiFM = oscfreqhiFM[nvoice][k];
            float  freqloFM = oscfreqloFM[nvoice][k];
            float *tw = tmpwave_unison[k];

            for(i = 0; i < synth->buffersize; ++i) {
                amp = INTERPOLATE_AMPLITUDE(FMoldamplitude[nvoice],
                                            FMnewamplitude[nvoice],
                                            i,
                                            synth->buffersize);
                tw[i] *= (NoteVoicePar[nvoice].FMSmp[poshiFM] * (1.0f - posloFM)
                          + NoteVoicePar[nvoice].FMSmp[poshiFM
                                                       + 1] * posloFM) * amp
                         + (1.0f - amp);
                posloFM += freqloFM;
                if(posloFM >= 1.0f) {
                    posloFM -= 1.0f;
                    poshiFM++;
                }
                poshiFM += freqhiFM;
                poshiFM &= synth->oscilsize - 1;
            }
            oscposhiFM[nvoice][k] = poshiFM;
            oscposloFM[nvoice][k] = posloFM;
        }
}



/*
 * Computes the Oscillator (Phase Modulation or Frequency Modulation)
 */
inline void ADnote::ComputeVoiceOscillatorFrequencyModulation(int nvoice,
                                                              int FMmode)
{
    int   carposhi = 0;
    int   i, FMmodfreqhi = 0;
    float FMmodfreqlo = 0, carposlo = 0;

    if(NoteVoicePar[nvoice].FMVoice >= 0)
        //if I use VoiceOut[] as modulator
        for(int k = 0; k < unison_size[nvoice]; ++k) {
            float *tw = tmpwave_unison[k];
            memcpy(tw, NoteVoicePar[NoteVoicePar[nvoice].FMVoice].VoiceOut,
                   synth->bufferbytes);
        }
    else
        //Compute the modulator and store it in tmpwave_unison[][]
        for(int k = 0; k < unison_size[nvoice]; ++k) {
            int    poshiFM  = oscposhiFM[nvoice][k];
            float  posloFM  = oscposloFM[nvoice][k];
            int    freqhiFM = oscfreqhiFM[nvoice][k];
            float  freqloFM = oscfreqloFM[nvoice][k];
            float *tw = tmpwave_unison[k];

            for(i = 0; i < synth->buffersize; ++i) {
                tw[i] =
                    (NoteVoicePar[nvoice].FMSmp[poshiFM] * (1.0f - posloFM)
                     + NoteVoicePar[nvoice].FMSmp[poshiFM + 1] * posloFM);
                posloFM += freqloFM;
                if(posloFM >= 1.0f) {
                    posloFM = fmod(posloFM, 1.0f);
                    poshiFM++;
                }
                poshiFM += freqhiFM;
                poshiFM &= synth->oscilsize - 1;
            }
            oscposhiFM[nvoice][k] = poshiFM;
            oscposloFM[nvoice][k] = posloFM;
        }
    // Amplitude interpolation
    if(ABOVE_AMPLITUDE_THRESHOLD(FMoldamplitude[nvoice],
                                 FMnewamplitude[nvoice]))
        for(int k = 0; k < unison_size[nvoice]; ++k) {
            float *tw = tmpwave_unison[k];
            for(i = 0; i < synth->buffersize; ++i)
                tw[i] *= INTERPOLATE_AMPLITUDE(FMoldamplitude[nvoice],
                                               FMnewamplitude[nvoice],
                                               i,
                                               synth->buffersize);
        }
    else
        for(int k = 0; k < unison_size[nvoice]; ++k) {
            float *tw = tmpwave_unison[k];
            for(i = 0; i < synth->buffersize; ++i)
                tw[i] *= FMnewamplitude[nvoice];
        }


    //normalize: makes all sample-rates, oscil_sizes to produce same sound
    if(FMmode != 0) { //Frequency modulation
        float normalize = synth->oscilsize_f / 262144.0f * 44100.0f
                          / synth->samplerate_f;
        for(int k = 0; k < unison_size[nvoice]; ++k) {
            float *tw    = tmpwave_unison[k];
            float  fmold = FMoldsmp[nvoice][k];
            for(i = 0; i < synth->buffersize; ++i) {
                fmold = fmod(fmold + tw[i] * normalize, synth->oscilsize);
                tw[i] = fmold;
            }
            FMoldsmp[nvoice][k] = fmold;
        }
    }
    else {  //Phase modulation
        float normalize = synth->oscilsize_f / 262144.0f;
        for(int k = 0; k < unison_size[nvoice]; ++k) {
            float *tw = tmpwave_unison[k];
            for(i = 0; i < synth->buffersize; ++i)
                tw[i] *= normalize;
        }
    }

    //do the modulation
    for(int k = 0; k < unison_size[nvoice]; ++k) {
        float *tw     = tmpwave_unison[k];
        int    poshi  = oscposhi[nvoice][k];
        float  poslo  = oscposlo[nvoice][k];
        int    freqhi = oscfreqhi[nvoice][k];
        float  freqlo = oscfreqlo[nvoice][k];

        for(i = 0; i < synth->buffersize; ++i) {
            F2I(tw[i], FMmodfreqhi);
            FMmodfreqlo = fmod(tw[i] + 0.0000000001f, 1.0f);
            if(FMmodfreqhi < 0)
                FMmodfreqlo++;

            //carrier
            carposhi = poshi + FMmodfreqhi;
            carposlo = poslo + FMmodfreqlo;

            if(carposlo >= 1.0f) {
                carposhi++;
                carposlo = fmod(carposlo, 1.0f);
            }
            carposhi &= (synth->oscilsize - 1);

            tw[i] = NoteVoicePar[nvoice].OscilSmp[carposhi]
                    * (1.0f - carposlo)
                    + NoteVoicePar[nvoice].OscilSmp[carposhi
                                                    + 1] * carposlo;

            poslo += freqlo;
            if(poslo >= 1.0f) {
                poslo = fmod(poslo, 1.0f);
                poshi++;
            }

            poshi += freqhi;
            poshi &= synth->oscilsize - 1;
        }
        oscposhi[nvoice][k] = poshi;
        oscposlo[nvoice][k] = poslo;
    }
}


/*Calculeaza Oscilatorul cu PITCH MODULATION*/
inline void ADnote::ComputeVoiceOscillatorPitchModulation(int /*nvoice*/)
{
//TODO
}

/*
 * Computes the Noise
 */
inline void ADnote::ComputeVoiceNoise(int nvoice)
{
    for(int k = 0; k < unison_size[nvoice]; ++k) {
        float *tw = tmpwave_unison[k];
        for(int i = 0; i < synth->buffersize; ++i)
            tw[i] = RND * 2.0f - 1.0f;
    }
}



/*
 * Compute the ADnote samples
 * Returns 0 if the note is finished
 */
int ADnote::noteout(float *outl, float *outr)
{
    memcpy(outl, denormalkillbuf, synth->bufferbytes);
    memcpy(outr, denormalkillbuf, synth->bufferbytes);

    if(NoteEnabled == OFF)
        return 0;

    memset(bypassl, 0, synth->bufferbytes);
    memset(bypassr, 0, synth->bufferbytes);
    computecurrentparameters();

    for(unsigned nvoice = 0; nvoice < NUM_VOICES; ++nvoice) {
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
        memset(tmpwavel, 0, synth->bufferbytes);
        if(stereo)
            memset(tmpwaver, 0, synth->bufferbytes);
        for(int k = 0; k < unison_size[nvoice]; ++k) {
            float *tw = tmpwave_unison[k];
            if(stereo) {
                float stereo_pos = 0;
                if(unison_size[nvoice] > 1)
                    stereo_pos = k
                                 / (float)(unison_size[nvoice]
                                           - 1) * 2.0f - 1.0f;
                float stereo_spread = unison_stereo_spread[nvoice] * 2.0f; //between 0 and 2.0f
                if(stereo_spread > 1.0f) {
                    float stereo_pos_1 = (stereo_pos >= 0.0f) ? 1.0f : -1.0f;
                    stereo_pos =
                        (2.0f
                         - stereo_spread) * stereo_pos
                        + (stereo_spread - 1.0f) * stereo_pos_1;
                }
                else
                    stereo_pos *= stereo_spread;

                if(unison_size[nvoice] == 1)
                    stereo_pos = 0.0f;
                float panning = (stereo_pos + 1.0f) * 0.5f;


                float lvol = (1.0f - panning) * 2.0f;
                if(lvol > 1.0f)
                    lvol = 1.0f;

                float rvol = panning * 2.0f;
                if(rvol > 1.0f)
                    rvol = 1.0f;

                if(unison_invert_phase[nvoice][k]) {
                    lvol = -lvol;
                    rvol = -rvol;
                }

                for(int i = 0; i < synth->buffersize; ++i)
                    tmpwavel[i] += tw[i] * lvol;
                for(int i = 0; i < synth->buffersize; ++i)
                    tmpwaver[i] += tw[i] * rvol;
            }
            else
                for(int i = 0; i < synth->buffersize; ++i)
                    tmpwavel[i] += tw[i];
        }


        float unison_amplitude = 1.0f / sqrt(unison_size[nvoice]); //reduce the amplitude for large unison sizes
        // Amplitude
        float oldam = oldamplitude[nvoice] * unison_amplitude;
        float newam = newamplitude[nvoice] * unison_amplitude;

        if(ABOVE_AMPLITUDE_THRESHOLD(oldam, newam)) {
            int rest = synth->buffersize;
            //test if the amplitude if raising and the difference is high
            if((newam > oldam) && ((newam - oldam) > 0.25f)) {
                rest = 10;
                if(rest > synth->buffersize)
                    rest = synth->buffersize;
                for(int i = 0; i < synth->buffersize - rest; ++i)
                    tmpwavel[i] *= oldam;
                if(stereo)
                    for(int i = 0; i < synth->buffersize - rest; ++i)
                        tmpwaver[i] *= oldam;
            }
            // Amplitude interpolation
            for(int i = 0; i < rest; ++i) {
                float amp = INTERPOLATE_AMPLITUDE(oldam, newam, i, rest);
                tmpwavel[i + (synth->buffersize - rest)] *= amp;
                if(stereo)
                    tmpwaver[i + (synth->buffersize - rest)] *= amp;
            }
        }
        else {
            for(int i = 0; i < synth->buffersize; ++i)
                tmpwavel[i] *= newam;
            if(stereo)
                for(int i = 0; i < synth->buffersize; ++i)
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
        if(NoteVoicePar[nvoice].AmpEnvelope != NULL)
            if(NoteVoicePar[nvoice].AmpEnvelope->finished() != 0) {
                for(int i = 0; i < synth->buffersize; ++i)
                    tmpwavel[i] *= 1.0f - (float)i / synth->buffersize_f;
                if(stereo)
                    for(int i = 0; i < synth->buffersize; ++i)
                        tmpwaver[i] *= 1.0f - (float)i / synth->buffersize_f;
            }
        //the voice is killed later


        // Put the ADnote samples in VoiceOut (without appling Global volume, because I wish to use this voice as a modullator)
        if(NoteVoicePar[nvoice].VoiceOut != NULL) {
            if(stereo)
                for(int i = 0; i < synth->buffersize; ++i)
                    NoteVoicePar[nvoice].VoiceOut[i] = tmpwavel[i]
                                                       + tmpwaver[i];
            else   //mono
                for(int i = 0; i < synth->buffersize; ++i)
                    NoteVoicePar[nvoice].VoiceOut[i] = tmpwavel[i];
        }


        // Add the voice that do not bypass the filter to out
        if(NoteVoicePar[nvoice].filterbypass == 0) { //no bypass
            if(stereo)
                for(int i = 0; i < synth->buffersize; ++i) { //stereo
                    outl[i] += tmpwavel[i] * NoteVoicePar[nvoice].Volume
                               * NoteVoicePar[nvoice].Panning * 2.0f;
                    outr[i] += tmpwaver[i] * NoteVoicePar[nvoice].Volume
                               * (1.0f - NoteVoicePar[nvoice].Panning) * 2.0f;
                }
            else
                for(int i = 0; i < synth->buffersize; ++i) //mono
                    outl[i] += tmpwavel[i] * NoteVoicePar[nvoice].Volume;
        }
        else {  //bypass the filter
            if(stereo)
                for(int i = 0; i < synth->buffersize; ++i) { //stereo
                    bypassl[i] += tmpwavel[i] * NoteVoicePar[nvoice].Volume
                                  * NoteVoicePar[nvoice].Panning * 2.0f;
                    bypassr[i] += tmpwaver[i] * NoteVoicePar[nvoice].Volume
                                  * (1.0f
                                     - NoteVoicePar[nvoice].Panning) * 2.0f;
                }
            else
                for(int i = 0; i < synth->buffersize; ++i) //mono
                    bypassl[i] += tmpwavel[i] * NoteVoicePar[nvoice].Volume;
        }
        // chech if there is necesary to proces the voice longer (if the Amplitude envelope isn't finished)
        if(NoteVoicePar[nvoice].AmpEnvelope != NULL)
            if(NoteVoicePar[nvoice].AmpEnvelope->finished() != 0)
                KillVoice(nvoice);
    }


    //Processing Global parameters
    NoteGlobalPar.GlobalFilterL->filterout(&outl[0]);

    if(stereo == 0) { //set the right channel=left channel
        memcpy(outr, outl, synth->bufferbytes);
        memcpy(bypassr, bypassl, synth->bufferbytes);
    }
    else
        NoteGlobalPar.GlobalFilterR->filterout(&outr[0]);

    for(int i = 0; i < synth->buffersize; ++i) {
        outl[i] += bypassl[i];
        outr[i] += bypassr[i];
    }

    if(ABOVE_AMPLITUDE_THRESHOLD(globaloldamplitude, globalnewamplitude))
        // Amplitude Interpolation
        for(int i = 0; i < synth->buffersize; ++i) {
            float tmpvol = INTERPOLATE_AMPLITUDE(globaloldamplitude,
                                                 globalnewamplitude,
                                                 i,
                                                 synth->buffersize);
            outl[i] *= tmpvol * NoteGlobalPar.Panning;
            outr[i] *= tmpvol * (1.0f - NoteGlobalPar.Panning);
        }
    else
        for(int i = 0; i < synth->buffersize; ++i) {
            outl[i] *= globalnewamplitude * NoteGlobalPar.Panning;
            outr[i] *= globalnewamplitude * (1.0f - NoteGlobalPar.Panning);
        }

    //Apply the punch
    if(NoteGlobalPar.Punch.Enabled != 0)
        for(int i = 0; i < synth->buffersize; ++i) {
            float punchamp = NoteGlobalPar.Punch.initialvalue
                             * NoteGlobalPar.Punch.t + 1.0f;
            outl[i] *= punchamp;
            outr[i] *= punchamp;
            NoteGlobalPar.Punch.t -= NoteGlobalPar.Punch.dt;
            if(NoteGlobalPar.Punch.t < 0.0f) {
                NoteGlobalPar.Punch.Enabled = 0;
                break;
            }
        }


    // Apply legato-specific sound signal modifications
    legato.apply(*this, outl, outr);


    // Check if the global amplitude is finished.
    // If it does, disable the note
    if(NoteGlobalPar.AmpEnvelope->finished()) {
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
 * Relase the key (NoteOff)
 */
void ADnote::relasekey()
{
    for(int nvoice = 0; nvoice < NUM_VOICES; ++nvoice)
        NoteVoicePar[nvoice].releasekey();
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

void ADnote::Voice::releasekey()
{
    if(!Enabled)
        return;
    if(AmpEnvelope)
        AmpEnvelope->relasekey();
    if(FreqEnvelope)
        FreqEnvelope->relasekey();
    if(FilterEnvelope)
        FilterEnvelope->relasekey();
    if(FMFreqEnvelope)
        FMFreqEnvelope->relasekey();
    if(FMAmpEnvelope)
        FMAmpEnvelope->relasekey();
}

template<class T>
static inline void nullify(T &t) {delete t; t = NULL; }
template<class T>
static inline void arrayNullify(T &t) {delete [] t; t = NULL; }

void ADnote::Voice::kill()
{
    arrayNullify(OscilSmp);
    nullify(FreqEnvelope);
    nullify(FreqLfo);
    nullify(AmpEnvelope);
    nullify(AmpLfo);
    nullify(VoiceFilterL);
    nullify(VoiceFilterR);
    nullify(FilterEnvelope);
    nullify(FilterLfo);
    nullify(FMFreqEnvelope);
    nullify(FMAmpEnvelope);

    if((FMEnabled != NONE) && (FMVoice < 0)) {
        delete[] FMSmp;
        FMSmp = NULL;
    }

    if(VoiceOut)
        memset(VoiceOut, 0, synth->bufferbytes);
    //do not delete, yet: perhaps is used by another voice

    Enabled = OFF;
}

void ADnote::Global::kill()
{
    nullify(FreqEnvelope);
    nullify(FreqLfo);
    nullify(AmpEnvelope);
    nullify(AmpLfo);
    nullify(GlobalFilterL);
    nullify(GlobalFilterR);
    nullify(FilterEnvelope);
    nullify(FilterLfo);
}

void ADnote::Global::initparameters(const ADnoteGlobalParam &param,
                                    float basefreq, float velocity,
                                    bool stereo)
{
    FreqEnvelope = new Envelope(param.FreqEnvelope, basefreq);
    FreqLfo      = new LFO(param.FreqLfo, basefreq);

    AmpEnvelope = new Envelope(param.AmpEnvelope, basefreq);
    AmpLfo      = new LFO(param.AmpLfo, basefreq);

    Volume = 4.0f * powf(0.1f, 3.0f * (1.0f - param.PVolume / 96.0f)) //-60 dB .. 0 dB
             * VelF(velocity, param.PAmpVelocityScaleFunction);     //sensing

    GlobalFilterL = Filter::generate(param.GlobalFilter);
    if(stereo)
        GlobalFilterR = Filter::generate(param.GlobalFilter);
    else
        GlobalFilterR = NULL;

    FilterEnvelope = new Envelope(param.FilterEnvelope, basefreq);
    FilterLfo      = new LFO(param.FilterLfo, basefreq);
    FilterQ = param.GlobalFilter->getq();
    FilterFreqTracking = param.GlobalFilter->getfreqtracking(basefreq);
}
