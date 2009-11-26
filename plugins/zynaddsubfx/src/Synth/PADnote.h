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

#include "../globals.h"
#include "../Params/PADnoteParameters.h"
#include "../Params/Controller.h"
#include "Envelope.h"
#include "LFO.h"
#include "../DSP/Filter.h"
#include "../Params/Controller.h"

/**The "pad" synthesizer*/
class PADnote
{
    public:
        PADnote(PADnoteParameters *parameters,
                Controller *ctl_,
                REALTYPE freq,
                REALTYPE velocity,
                int portamento_,
                int midinote,
                bool besilent);
        ~PADnote();

        void PADlegatonote(REALTYPE freq,
                           REALTYPE velocity,
                           int portamento_,
                           int midinote,
                           bool externcall);

        int noteout(REALTYPE *outl, REALTYPE *outr);
        int finished();
        void relasekey();

        int ready;

    private:
        void fadein(REALTYPE *smps);
        void computecurrentparameters();
        bool finished_;
        PADnoteParameters *pars;

        int      poshi_l, poshi_r;
        REALTYPE poslo;

        REALTYPE basefreq;
        bool     firsttime, released;

        int nsample, portamento;

        int Compute_Linear(REALTYPE *outl,
                           REALTYPE *outr,
                           int freqhi,
                           REALTYPE freqlo);
        int Compute_Cubic(REALTYPE *outl,
                          REALTYPE *outr,
                          int freqhi,
                          REALTYPE freqlo);


        struct {
            /******************************************
            *     FREQUENCY GLOBAL PARAMETERS        *
            ******************************************/
            REALTYPE  Detune; //cents

            Envelope *FreqEnvelope;
            LFO      *FreqLfo;

            /********************************************
             *     AMPLITUDE GLOBAL PARAMETERS          *
             ********************************************/
            REALTYPE  Volume; // [ 0 .. 1 ]

            REALTYPE  Panning; // [ 0 .. 1 ]

            Envelope *AmpEnvelope;
            LFO      *AmpLfo;

            struct {
                int      Enabled;
                REALTYPE initialvalue, dt, t;
            } Punch;

            /******************************************
            *        FILTER GLOBAL PARAMETERS        *
            ******************************************/
            Filter   *GlobalFilterL, *GlobalFilterR;

            REALTYPE  FilterCenterPitch; //octaves
            REALTYPE  FilterQ;
            REALTYPE  FilterFreqTracking;

            Envelope *FilterEnvelope;

            LFO      *FilterLfo;
        } NoteGlobalPar;


        REALTYPE    globaloldamplitude, globalnewamplitude, velocity, realfreq;
        REALTYPE   *tmpwave;
        Controller *ctl;

        // Legato vars
        struct {
            bool      silent;
            REALTYPE  lastfreq;
            LegatoMsg msg;
            int decounter;
            struct { // Fade In/Out vars
                int      length;
                REALTYPE m, step;
            } fade;
            struct { // Note parameters
                REALTYPE freq, vel;
                int      portamento, midinote;
            } param;
        } Legato;
};


#endif

