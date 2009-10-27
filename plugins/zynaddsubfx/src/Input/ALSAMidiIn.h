/*
  ZynAddSubFX - a software synthesizer

  ALSAMidiIn.h - Midi input for ALSA (this creates an ALSA virtual port)
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

#ifndef ALSA_MIDI_IN_H
#define ALSA_MIDI_IN_H

#include <alsa/asoundlib.h>
#include "MidiIn.h"


/**Midi input for ALSA (this creates an ALSA virtual port)*/
class ALSAMidiIn:public MidiIn
{
    public:
        /**Constructor*/
        ALSAMidiIn();
        /**Destructor*/
        ~ALSAMidiIn();

        void getmidicmd(MidiCmdType &cmdtype,
                        unsigned char &cmdchan,
                        int *cmdparams);
        /**Get the ALSA id
         * @return ALSA id*/
        int getalsaid();

    private:
        snd_seq_t *midi_handle;
};

#endif

