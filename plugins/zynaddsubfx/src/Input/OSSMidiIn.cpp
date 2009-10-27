/*
  ZynAddSubFX - a software synthesizer

  OSSMidiIn.C - Midi input for Open Sound System
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
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/soundcard.h>

#include "OSSMidiIn.h"
#include "../Misc/Util.h"

OSSMidiIn::OSSMidiIn()
{
    inputok     = false;
    midi_handle = open(config.cfg.LinuxOSSSeqInDev, O_RDONLY, 0);
    if(midi_handle != -1)
        inputok = true;

    lastmidicmd = 0;
    cmdtype     = 0;
    cmdchan     = 0;
}

OSSMidiIn::~OSSMidiIn()
{
    close(midi_handle);
}

unsigned char OSSMidiIn::readbyte()
{
    unsigned char tmp[4];
    read(midi_handle, &tmp[0], 1);
    while(tmp[0] != SEQ_MIDIPUTC) {
        read(midi_handle, &tmp[0], 4);
    }
    return tmp[1];
}

unsigned char OSSMidiIn::getmidibyte()
{
    unsigned char b;
    do {
        b = readbyte();
    } while(b == 0xfe); //drops the Active Sense Messages
    return b;
}

/*
 * Get the midi command,channel and parameters
 */
void OSSMidiIn::getmidicmd(MidiCmdType &cmdtype,
                           unsigned char &cmdchan,
                           int *cmdparams)
{
    unsigned char tmp, i;
    if(inputok == false) {
        cmdtype = MidiNull;
        return;
    }
    i = 0;
    if(lastmidicmd == 0) { //asteapta prima data pana cand vine prima comanda midi
        while(tmp < 0x80)
            tmp = getmidibyte();
        lastmidicmd = tmp;
    }

    tmp = getmidibyte();

    if(tmp >= 0x80) {
        lastmidicmd = tmp;
        tmp = getmidibyte();
    }

    if((lastmidicmd >= 0x80) && (lastmidicmd <= 0x8f)) { //Note OFF
        cmdtype      = MidiNoteOFF;
        cmdchan      = lastmidicmd % 16;
        cmdparams[0] = tmp; //note number
    }

    if((lastmidicmd >= 0x90) && (lastmidicmd <= 0x9f)) { //Note ON
        cmdtype      = MidiNoteON;
        cmdchan      = lastmidicmd % 16;
        cmdparams[0] = tmp; //note number
        cmdparams[1] = getmidibyte(); //velocity
        if(cmdparams[1] == 0)
            cmdtype = MidiNoteOFF;               //if velocity==0 then is note off
    }
    if((lastmidicmd >= 0xB0) && (lastmidicmd <= 0xBF)) { //Controllers
        cmdtype      = MidiController;
        cmdchan      = lastmidicmd % 16;
        cmdparams[0] = getcontroller(tmp);
        cmdparams[1] = getmidibyte();
    }
    if((lastmidicmd >= 0xE0) && (lastmidicmd <= 0xEF)) { //Pitch Wheel
        cmdtype      = MidiController;
        cmdchan      = lastmidicmd % 16;
        cmdparams[0] = C_pitchwheel;
        cmdparams[1] = (tmp + getmidibyte() * (int) 128) - 8192; //hope this is correct
    }
}

