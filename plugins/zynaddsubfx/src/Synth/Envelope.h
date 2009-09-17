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

#include <math.h>
#include "../globals.h"
#include "../Params/EnvelopeParams.h"

/**Implementation of a general Envelope*/
class Envelope
{
public:

    /**Constructor*/
    Envelope(EnvelopeParams *envpars,REALTYPE basefreq);
    /**Destructor*/
    ~Envelope();
    void relasekey();
    REALTYPE envout();
    REALTYPE envout_dB();
    /**Determines the status of the Envelope
     *
     *\todo see if this can be changed to use a boolean
     * @return returns 1 if the envelope is finished*/
    int finished();
private:
    int envpoints;
    int envsustain;//"-1" means disabled
    REALTYPE envdt[MAX_ENVELOPE_POINTS];//millisecons
    REALTYPE envval[MAX_ENVELOPE_POINTS];// [0.0 .. 1.0]
    REALTYPE envstretch;
    int linearenvelope;

    int currentpoint; //current envelope point (starts from 1)
    int forcedrelase;
    char keyreleased; //if the key was released /** \todo figure out WHY IS THIS A CHAR*/
    char envfinish; /** \todo figure out WHY IS THIS A CHAR*/
    REALTYPE t;   // the time from the last point
    REALTYPE inct;// the time increment
    REALTYPE envoutval;//used to do the forced release
};


#endif

