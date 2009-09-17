/*
  ZynAddSubFX - a software synthesizer

  ALSAMidiIn.C - Midi input for ALSA (this creates an ALSA virtual port)
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

#include "ALSAMidiIn.h"

ALSAMidiIn::ALSAMidiIn()
{
    int alsaport;
    inputok=false;

    midi_handle=NULL;

    if (snd_seq_open(&midi_handle,"default",SND_SEQ_OPEN_INPUT,0)!=0) return;

    snd_seq_set_client_name(midi_handle,"ZynAddSubFX");//thanks to Frank Neumann

    alsaport = snd_seq_create_simple_port(midi_handle,"ZynAddSubFX"
                                          ,SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE
                                          ,SND_SEQ_PORT_TYPE_SYNTH);
    if (alsaport<0) return;

    inputok=true;
};

ALSAMidiIn::~ALSAMidiIn()
{
    if (midi_handle) snd_seq_close(midi_handle);
};


/*
 * Get the midi command,channel and parameters
 */
void ALSAMidiIn::getmidicmd(MidiCmdType &cmdtype,unsigned char &cmdchan,int *cmdparams)
{
    snd_seq_event_t *midievent=NULL;
    cmdtype=MidiNull;

    if (inputok==false) {
        /* The input is broken. We need to block for a while anyway so other
           non-RT threads get a chance to run. */
        sleep(1);
        return;
    };

    snd_seq_event_input(midi_handle,&midievent);

    if (midievent==NULL) return;
    switch (midievent->type) {
    case SND_SEQ_EVENT_NOTEON:
        cmdtype=MidiNoteON;
        cmdchan=midievent->data.note.channel;
        cmdparams[0]=midievent->data.note.note;
        cmdparams[1]=midievent->data.note.velocity;
        break;
    case SND_SEQ_EVENT_NOTEOFF:
        cmdtype=MidiNoteOFF;
        cmdchan=midievent->data.note.channel;
        cmdparams[0]=midievent->data.note.note;
        break;
    case SND_SEQ_EVENT_PITCHBEND:
        cmdtype=MidiController;
        cmdchan=midievent->data.control.channel;
        cmdparams[0]=C_pitchwheel;//Pitch Bend
        cmdparams[1]=midievent->data.control.value;
        break;
    case SND_SEQ_EVENT_CONTROLLER:
        cmdtype=MidiController;
        cmdchan=midievent->data.control.channel;
        cmdparams[0]=getcontroller(midievent->data.control.param);
        cmdparams[1]=midievent->data.control.value;
        //fprintf(stderr,"t=%d val=%d\n",midievent->data.control.param,midievent->data.control.value);
        break;

    };
};


int ALSAMidiIn::getalsaid()
{
    if (midi_handle) {
        snd_seq_client_info_t* seq_info;
        snd_seq_client_info_malloc(&seq_info);
        snd_seq_get_client_info(midi_handle, seq_info);
        int id = snd_seq_client_info_get_client(seq_info);
        snd_seq_client_info_free(seq_info);
        return id;
    }
    return -1;
}

