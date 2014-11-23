/*
  ZynAddSubFX - a software synthesizer

  Dump.cpp - It dumps the notes to a text file

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
#include <stdlib.h>
#include <time.h>
#include "Util.h"
#include "Dump.h"

Dump dump;

Dump::Dump()
{
    file = NULL;
    tick = 0;
    k    = 0;
    keyspressed = 0;
}

Dump::~Dump()
{
    if(file != NULL) {
        int duration = tick * synth->buffersize_f / synth->samplerate_f;
        fprintf(
            file,
            "\n# statistics: duration = %d seconds; keyspressed = %d\n\n\n\n",
            duration,
            keyspressed);
        fclose(file);
    }
}

void Dump::startnow()
{
    if(file != NULL)
        return;            //the file is already open

    if(config.cfg.DumpNotesToFile != 0) {
        if(config.cfg.DumpAppend != 0)
            file = fopen(config.cfg.DumpFile.c_str(), "a");
        else
            file = fopen(config.cfg.DumpFile.c_str(), "w");
        if(file == NULL)
            return;
        if(config.cfg.DumpAppend != 0)
            fprintf(file, "%s", "#************************************\n");

        time_t tm = time(NULL);

        fprintf(file, "#date/time = %s\n", ctime(&tm));
        fprintf(file, "#1 tick = %g milliseconds\n",
                synth->buffersize_f * 1000.0f / synth->samplerate_f);
        fprintf(file, "SAMPLERATE = %d\n", synth->samplerate);
        fprintf(file, "TICKSIZE = %d #samples\n", synth->buffersize);
        fprintf(file, "\n\nSTART\n");
    }
}

void Dump::inctick()
{
    tick++;
}


void Dump::dumpnote(char chan, char note, char vel)
{
    if(file == NULL)
        return;
    if(note == 0)
        return;
    if(vel == 0)
        fprintf(file, "n %d -> %d %d \n", tick, chan, note);    //note off
    else
        fprintf(file, "N %d -> %d %d %d \n", tick, chan, note, vel);  //note on

    if(vel != 0)
        keyspressed++;
#ifndef JACKAUDIOOUT
    if(k++ > 25) {
        fflush(file);
        k = 0;
    }
#endif
}

void Dump::dumpcontroller(char chan, unsigned int type, int par)
{
    if(file == NULL)
        return;
    switch(type) {
        case C_pitchwheel:
            fprintf(file, "P %d -> %d %d\n", tick, chan, par);
            break;
        default:
            fprintf(file, "C %d -> %d %d %d\n", tick, chan, type, par);
            break;
    }
#ifndef JACKAUDIOOUT
    if(k++ > 25) {
        fflush(file);
        k = 0;
    }
#endif
}
