/*
  ZynAddSubFX - a software synthesizer

  Sequencer.cpp - The Sequencer
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

#include <math.h>
#include <stdlib.h>
#include <stdio.h>

#include <sys/time.h>
#include <time.h>

#include "Sequencer.h"



Sequencer::Sequencer()
{
    play = 0;
    for(int i = 0; i < NUM_MIDI_TRACKS; i++) {
        miditrack[i].track.first    = NULL;
        miditrack[i].track.current  = NULL;
        miditrack[i].track.size     = 0;
        miditrack[i].track.length   = 0.0;
        miditrack[i].record.first   = NULL;
        miditrack[i].record.current = NULL;
        miditrack[i].record.size    = 0;
        miditrack[i].record.length  = 0.0;

        nextevent[i].time = 0.0;
        resettime(&playtime[i]);
    }

    setplayspeed(0);
}

Sequencer::~Sequencer()
{
    for(int i = 0; i < NUM_MIDI_TRACKS; i++) {
        deletelist(&miditrack[i].track);
        deletelist(&miditrack[i].record);
    }
}


int Sequencer::importmidifile(const char *filename)
{
    if(midifile.loadfile(filename) < 0)
        return -1;

    for(int i = 0; i < NUM_MIDI_TRACKS; i++)
        deletelist(&miditrack[i].record);
    ;
    if(midifile.parsemidifile(this) < 0)
        return -1;

    //copy the "record" track to the main track
    for(int i = 0; i < NUM_MIDI_TRACKS; i++) {
        deletelist(&miditrack[i].track);
        miditrack[i].track = miditrack[i].record;
        deletelistreference(&miditrack[i].record);
    }
    return 0;
}



void Sequencer::startplay()
{
    if(play != 0)
        return;
    for(int i = 0; i < NUM_MIDI_TRACKS; i++)
        resettime(&playtime[i]);

    for(int i = 0; i < NUM_MIDI_TRACKS; i++)
        rewindlist(&miditrack[i].track);
    ;
    play = 1;
}
void Sequencer::stopplay()
{
    if(play == 0)
        return;
    play = 0;
}

// ************ Player stuff ***************

int Sequencer::getevent(char ntrack,
                        int *midich,
                        int *type,
                        int *par1,
                        int *par2)
{
    *type = 0;
    if(play == 0)
        return -1;

    //test
//    if (ntrack!=0) return(-1);

    updatecounter(&playtime[(int)ntrack]);

//    printf("%g %g\n",nextevent[ntrack].time,playtime[ntrack].abs);

    if(nextevent[(int)ntrack].time < playtime[(int)ntrack].abs)
        readevent(&miditrack[(int)ntrack].track, &nextevent[(int)ntrack].ev);
    else
        return -1;
    if(nextevent[(int)ntrack].ev.type == -1)
        return -1;
//    printf("********************************\n");

    //sa pun aici o protectie. a.i. daca distanta dintre timpul curent si eveliment e prea mare (>1sec) sa elimin nota

    if(ntrack == 1)
        printf("_ %f %.2f  (%d)\n", nextevent[(int)ntrack].time,
               playtime[(int)ntrack].abs, nextevent[(int)ntrack].ev.par2);

    *type   = nextevent[(int)ntrack].ev.type;
    *par1   = nextevent[(int)ntrack].ev.par1;
    *par2   = nextevent[(int)ntrack].ev.par2;
    *midich = nextevent[(int)ntrack].ev.channel;


    double dt = nextevent[(int)ntrack].ev.deltatime * 0.0001 * realplayspeed;
    printf("zzzzzzzzzzzzzz[%d] %d\n",
           ntrack,
           nextevent[(int)ntrack].ev.deltatime);
    nextevent[(int)ntrack].time += dt;

//    printf("%f   -  %d %d \n",nextevent[ntrack].time,par1,par2);
    return 0; //?? sau 1
}

/************** Timer stuff ***************/

void Sequencer::resettime(timestruct *t)
{
    t->abs = 0.0;
    t->rel = 0.0;

    timeval tval;

    t->last = 0.0;
#ifndef OS_WINDOWS
    if(gettimeofday(&tval, NULL) == 0)
        t->last = tval.tv_sec + tval.tv_usec * 0.000001;
#endif
}

void Sequencer::updatecounter(timestruct *t)
{
    timeval tval;
    double  current = 0.0;
#ifndef OS_WINDOWS
    if(gettimeofday(&tval, NULL) == 0)
        current = tval.tv_sec + tval.tv_usec * 0.000001;
#endif

    t->rel  = current - t->last;
    t->abs += t->rel;
    t->last = current;

//    printf("%f %f %f\n",t->last,t->abs,t->rel);
}

void Sequencer::setplayspeed(int speed)
{
    playspeed     = speed;
    realplayspeed = pow(10.0, speed / 128.0);
}

