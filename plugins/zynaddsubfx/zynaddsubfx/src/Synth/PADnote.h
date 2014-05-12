/*
  ZynAddSubFX - a software synthesizer

  PADnote.h - The "pad" synthesizer
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
#ifndef PAD_NOTE_H
#define PAD_NOTE_H

#include "SynthNote.h"
#include "../globals.h"
#include "../Params/PADnoteParameters.h"
#include "../Params/Controller.h"
#include "Envelope.h"
#include "LFO.h"
#include "../Params/Controller.h"

/**The "pad" synthesizer*/
class PADnote:public SynthNote
{
    public:
        PADnote(PADnoteParameters *parameters,
                Controller *ctl_,
                float freq,
                float velocity,
                int portamento_,
                int midinote,
                bool besilent);
        ~PADnote();

        void legatonote(float freq, float velocity, int portamento_,
                        int midinote, bool externcall);

        int noteout(float *outl, float *outr);
        int finished() const;
        void relasekey();
    private:
        void setup(float freq, float velocity, int portamento_,
                   int midinote, bool legato = false);
        void fadein(float *smps);
        void computecurrentparameters();
        bool finished_;
        PADnoteParameters *pars;

        int   poshi_l, poshi_r;
        float poslo;

        float basefreq;
        bool  firsttime, released;

        int nsample, portamento;

        int Compute_Linear(float *outl,
                           float *outr,
                           int freqhi,
                           float freqlo);
        int Compute_Cubic(float *outl,
                          float *outr,
                          int freqhi,
                          float freqlo);


        struct {
            /******************************************
            *     FREQUENCY GLOBAL PARAMETERS        *
            ******************************************/
            float Detune;  //cents

            Envelope *FreqEnvelope;
            LFO      *FreqLfo;

            /********************************************
             *     AMPLITUDE GLOBAL PARAMETERS          *
             ********************************************/
            float Volume;  // [ 0 .. 1 ]

            float Panning;  // [ 0 .. 1 ]

            Envelope *AmpEnvelope;
            LFO      *AmpLfo;

            struct {
                int   Enabled;
                float initialvalue, dt, t;
            } Punch;

            /******************************************
            *        FILTER GLOBAL PARAMETERS        *
            ******************************************/
            class Filter * GlobalFilterL, *GlobalFilterR;

            float FilterCenterPitch;  //octaves
            float FilterQ;
            float FilterFreqTracking;

            Envelope *FilterEnvelope;

            LFO *FilterLfo;
        } NoteGlobalPar;


        float globaloldamplitude, globalnewamplitude, velocity, realfreq;
        Controller *ctl;
};


#endif
