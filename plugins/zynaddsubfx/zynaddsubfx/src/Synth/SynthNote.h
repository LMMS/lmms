/*
  ZynAddSubFX - a software synthesizer

  Note.h - Abstract Base Class for synthesizers
  Copyright (C) 2010-2010 Mark McCurry
  Author: Mark McCurry

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
#ifndef SYNTH_NOTE_H
#define SYNTH_NOTE_H
#include "../globals.h"
#include "../Params/FilterParams.h"

class SynthNote
{
    public:
        SynthNote(float freq, float vel, int port, int note, bool quiet);
        virtual ~SynthNote() {}

        /**Compute Output Samples
         * @return 0 if note is finished*/
        virtual int noteout(float *outl, float *outr) = 0;

        //TODO fix this spelling error [noisey commit]
        /**Release the key for the note and start release portion of envelopes.*/
        virtual void relasekey() = 0;

        /**Return if note is finished.
         * @return finished=1 unfinished=0*/
        virtual int finished() const = 0;

        virtual void legatonote(float freq, float velocity,
                                int portamento_, int midinote_,
                                bool externcall) = 0;
        /* For polyphonic aftertouch needed */
        void setVelocity(float velocity_);
    protected:
        // Legato transitions
        class Legato
        {
            public:
                Legato(float freq, float vel, int port,
                       int note, bool quiet);

                void apply(SynthNote &note, float *outl, float *outr);
                int update(float freq, float velocity, int portamento_,
                           int midinote_, bool externalcall);

            private:
                bool      silent;
                float     lastfreq;
                LegatoMsg msg;
                int       decounter;
                struct { // Fade In/Out vars
                    int   length;
                    float m, step;
                } fade;
                struct { // Note parameters
                    float freq, vel;
                    int   portamento, midinote;
                } param;

            public: /* Some get routines for legatonote calls (aftertouch feature)*/
                float getFreq() {return param.freq; }
                float getVelocity() {return param.vel; }
                int getPortamento() {return param.portamento; }
                int getMidinote() {return param.midinote; }
                void setSilent(bool silent_) {silent = silent_; }
                void setDecounter(int decounter_) {decounter = decounter_; }
        } legato;
};

#endif
