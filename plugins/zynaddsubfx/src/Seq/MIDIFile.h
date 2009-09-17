/*
  ZynAddSubFX - a software synthesizer

  MIDIFile.h - MIDI file loader
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

#ifndef MIDIFILE_H
#define MIDIFILE_H

#include "../globals.h"
#include "MIDIEvents.h"

/**MIDI file loader*/
class MIDIFile
{
public:
    MIDIFile();
    ~MIDIFile();

    /**Loads the given file
         * @param filename The name of the file to load
         * @return -1 if there is an error, otherwise 0*/
    int loadfile(const char *filename);

    //returns -1 if there is an error, otherwise 0
    int parsemidifile(MIDIEvents *me_);

private:
    MIDIEvents *me;

    unsigned char *midifile;
    int midifilesize,midifilek;
    bool midieof;

    //returns -1 if there is an error, otherwise 0
    int parsetrack(int ntrack);

    void parsenoteoff(char ntrack,char chan,unsigned int dt);
    void parsenoteon(char ntrack,char chan,unsigned int dt);
    void parsecontrolchange(char ntrack,char chan,unsigned int dt);
    void parsepitchwheel(char ntrack,char chan, unsigned int dt);
    void parsemetaevent(unsigned char mtype,unsigned char mlength);

    void add_dt(char ntrack, unsigned int dt);

    void clearmidifile();

    //convert the delta-time to internal format
    unsigned int convertdt(unsigned int dt);

    /* Low Level MIDIfile functions */

    //get a byte from the midifile
    unsigned char getbyte();

    //peek the current byte from the midifile
    unsigned char peekbyte();

    //get a set of 4 bytes from the midifile
    unsigned int getint32();

    //get a word of 2 bytes from the midifile
    unsigned short int getint16();

    //read a variable length quantity
    unsigned int getvarint32();

    //skip some bytes
    void skipnbytes(int n);

    struct {
        double tick;//how many seconds one tick has

    }data;

};

#endif

