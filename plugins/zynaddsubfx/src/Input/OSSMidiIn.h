/*
  ZynAddSubFX - a software synthesizer

  OSSMidiIn.h - Midi input for Open Sound System
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

#ifndef OSS_MIDI_IN_H
#define OSS_MIDI_IN_H

#include "MidiIn.h"

class OSSMidiIn:public MidiIn
{
    public:
        OSSMidiIn();
        ~OSSMidiIn();
        unsigned char getmidibyte();
        unsigned char readbyte();

        //Midi parser
        void getmidicmd(MidiCmdType &cmdtype,
                        unsigned char &cmdchan,
                        int *cmdparams);
        unsigned char cmdtype; //the Message Type (noteon,noteof,sysex..)
        unsigned char cmdchan; //the channel number

    private:
        int midi_handle;
        unsigned char lastmidicmd; //last byte (>=80) received from the Midi
};


#endif

