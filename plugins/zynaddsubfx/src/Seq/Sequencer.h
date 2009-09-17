/*
  ZynAddSubFX - a software synthesizer

  Sequencer.h - The Sequencer
  Copyright (C) 2003-2005 Nasca Octavian Paul
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

#ifndef SEQUENCER_H
#define SEQUENCER_H

#include "../globals.h"
#include "MIDIEvents.h"
#include "MIDIFile.h"

/**The Sequencer
 * \todo restructure some of this code*/
class Sequencer:public MIDIEvents
{
public:
    /**Constructor*/
    Sequencer();
    /**Destructor*/
    ~Sequencer();

    //these functions are called by the master and are ignored if the recorder/player are stopped
    void recordnote(char chan, char note, char vel);
    void recordcontroller(char chan,unsigned int type,int par);

    /**Gets an event \todo better description
     *
     * this is only for player
     * @return 1 if this must be called at least once more
     *         0 if there are no more notes for the current time
     *        -1 if there are no notes*/
    int getevent(char ntrack, int *midich,int *type,int *par1, int *par2);

    /**Imports a given midifile
     * @return 0 if ok or -1 if there is a error loading file*/
    int importmidifile(const char *filename);

    void startplay();
    void stopplay();


    int play;
    int playspeed;//viteza de rulare (0.1x-10x), 0=1.0x, 128=10x
    void setplayspeed(int speed);

private:

    MIDIFile midifile;

    /* Timer */
    struct timestruct {
        double abs;//the time from the begining of the track
        double rel;//the time difference between the last and the current event
        double last;//the time of the last event (absolute, since 1 Jan 1970)
        //these must be double, because the float's precision is too low
        //and all these represent the time in seconds
    } playtime[NUM_MIDI_TRACKS];

    void resettime(timestruct *t);
    void updatecounter(timestruct *t);//this updates the timer values

    /* Player only*/

    struct {
        event ev;
        double time;
    } nextevent[NUM_MIDI_TRACKS];

    double realplayspeed;

};

#endif

