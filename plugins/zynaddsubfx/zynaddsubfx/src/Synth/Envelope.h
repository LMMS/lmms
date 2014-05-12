/*
  ZynAddSubFX - a software synthesizer

  Envelope.h - Envelope implementation
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

#ifndef ENVELOPE_H
#define ENVELOPE_H

#include "../globals.h"
#include "../Params/EnvelopeParams.h"

/**Implementation of a general Envelope*/
class Envelope
{
    public:

        /**Constructor*/
        Envelope(class EnvelopeParams *envpars, float basefreq);
        /**Destructor*/
        ~Envelope();
        void relasekey();
        float envout();
        float envout_dB();
        /**Determines the status of the Envelope
         * @return returns 1 if the envelope is finished*/
        bool finished() const;
    private:
        int   envpoints;
        int   envsustain;    //"-1" means disabled
        float envdt[MAX_ENVELOPE_POINTS]; //millisecons
        float envval[MAX_ENVELOPE_POINTS]; // [0.0f .. 1.0f]
        float envstretch;
        int   linearenvelope;

        int   currentpoint;    //current envelope point (starts from 1)
        int   forcedrelase;
        bool  keyreleased;    //if the key was released
        bool  envfinish;
        float t; // the time from the last point
        float inct; // the time increment
        float envoutval; //used to do the forced release
};


#endif
