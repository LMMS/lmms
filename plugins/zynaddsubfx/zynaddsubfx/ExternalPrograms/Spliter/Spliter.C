//Copyright (c) 2002-2003 Nasca Octavian Paul
//License: GNU GPL 2

#include "Spliter.h"
#include <stdio.h>

pthread_mutex_t mutex;
int Pexitprogram;

Spliter::Spliter() {
    //init
    Psplitpoint = 60;
    Pchin   = 0;
    Pchout1 = 0;
    Pchout2 = 1;
    Poct1   = 0;
    Poct2   = 0;
    //ALSA init
    snd_seq_open(&midi_in, "default", SND_SEQ_OPEN_INPUT, 0);
    snd_seq_open(&midi_out, "default", SND_SEQ_OPEN_OUTPUT, 0);

    char portname[50]; sprintf(portname, "Spliter IN");
    int  alsaport = snd_seq_create_simple_port(
        midi_in,
        portname,
        SND_SEQ_PORT_CAP_WRITE
        | SND_SEQ_PORT_CAP_SUBS_WRITE,
        SND_SEQ_PORT_TYPE_SYNTH);
    sprintf(portname, "Spliter OUT");
    alsaport = snd_seq_create_simple_port(
        midi_out,
        portname,
        SND_SEQ_PORT_CAP_READ
        | SND_SEQ_PORT_CAP_SUBS_READ,
        SND_SEQ_PORT_TYPE_SYNTH);
}

Spliter::~Spliter() {
    snd_seq_close(midi_in);
    snd_seq_close(midi_out);
}

// This splits the Midi events from one channel to another two channels
void Spliter::midievents() {
    snd_seq_event_t *midievent;
    midievent = NULL;
    snd_seq_event_input(midi_in, &midievent);

    if(midievent == NULL)
        return;
    if((midievent->type == SND_SEQ_EVENT_NOTEON)
       || (midievent->type == SND_SEQ_EVENT_NOTEOFF)) {
        int cmdchan = midievent->data.note.channel;
        if(cmdchan == Pchin) {
            snd_seq_ev_set_subs(midievent);
            snd_seq_ev_set_direct(midievent);
            if(midievent->data.note.note < Psplitpoint) {
                midievent->data.note.channel = Pchout1;
                int tmp = midievent->data.note.note;
                tmp += Poct1 * 12; if(tmp > 127)
                    tmp = 127; if(tmp < 0)
                    tmp = 0;
                midievent->data.note.note = tmp;
            }
            else {
                midievent->data.note.channel = Pchout2;
                int tmp = midievent->data.note.note;
                tmp += Poct2 * 12; if(tmp > 127)
                    tmp = 127; if(tmp < 0)
                    tmp = 0;
                midievent->data.note.note = tmp;
            }
            snd_seq_event_output_direct(midi_out, midievent);
        }
        else {
            snd_seq_ev_set_subs(midievent);
            snd_seq_ev_set_direct(midievent);
            snd_seq_event_output_direct(midi_out, midievent);
        }
    }
    snd_seq_free_event(midievent);
}
