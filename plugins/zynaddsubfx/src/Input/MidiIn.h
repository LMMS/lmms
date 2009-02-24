/*
  ZynAddSubFX - a software synthesizer
 
  MidiIn.h - This class is inherited by all the Midi input classes
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

#ifndef MIDI_IN_H
#define MIDI_IN_H

#include "../globals.h"

enum MidiCmdType{MidiNull,MidiNoteOFF,MidiNoteON,MidiController};
#define MP_MAX_BYTES 4000  //in case of loooong SYS_EXes

class MidiIn{
    public:
	virtual void getmidicmd(MidiCmdType &cmdtype,unsigned char &cmdchan,int *cmdparams){};
	virtual ~MidiIn(){};
	int getcontroller(unsigned char b);
    protected:
	int inputok;//1 if I can read midi bytes from input ports
};


#endif

