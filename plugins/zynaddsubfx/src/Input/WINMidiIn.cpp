/*
  ZynAddSubFX - a software synthesizer

  WINMidiIn.C - Midi input for Windows
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
#include <windows.h>
#include <mmsystem.h>
#include <pthread.h>

#include "WINMidiIn.h"
#include "MidiIn.h"
#include "../Misc/Util.h"

Master *winmaster;
HMIDIIN winmidiinhandle;
MidiIn  midictl; //used to convert the controllers to ZynAddSubFX controllers

void CALLBACK WinMidiInProc(HMIDIIN hMidiIn, UINT wMsg, DWORD dwInstance,
                            DWORD dwParam1, DWORD dwParam2)
{
    int midicommand = MidiNull;
    if(wMsg == MIM_DATA) {
        int cmd, par1, par2;
        cmd  = dwParam1 & 0xff;
        if(cmd == 0xfe)
            return;
        par1 = (dwParam1 >> 8) & 0xff;
        par2 = dwParam1 >> 16;
        //printf("%x %x %x\n",cmd,par1,par2);fflush(stdout);
        int cmdchan = cmd & 0x0f;
        int cmdtype = (cmd >> 4) & 0x0f;

        int tmp     = 0;
        pthread_mutex_lock(&winmaster->mutex);
        switch(cmdtype) {
        case (0x8): //noteon
            winmaster->NoteOff(cmdchan, par1);
            break;
        case (0x9): //noteoff
            winmaster->NoteOn(cmdchan, par1, par2 & 0xff);
            break;
        case (0xb): //controller
            winmaster->SetController(cmdchan, midictl.getcontroller(
                                         par1), par2 & 0xff);
            break;
        case (0xe): //pitch wheel
            tmp = (par1 + par2 * (long int) 128) - 8192;
            winmaster->SetController(cmdchan, C_pitchwheel, tmp);
            break;
        default:
            break;
        }
        pthread_mutex_unlock(&winmaster->mutex);
    }
}

void InitWinMidi(Master *master_)
{
    winmaster = master_;

    long int result =
        midiInOpen(&winmidiinhandle,
                   config.cfg.WindowsMidiInId,
                   (DWORD)WinMidiInProc,
                   0,
                   CALLBACK_FUNCTION);
    result = midiInStart(winmidiinhandle);
}

void StopWinMidi()
{
    midiInStop(winmidiinhandle);
    midiInClose(winmidiinhandle);
}

