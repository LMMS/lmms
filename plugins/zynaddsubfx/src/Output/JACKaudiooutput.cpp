/*
  ZynAddSubFX - a software synthesizer

  JACKaudiooutput.C - Audio output for JACK
  Copyright (C) 2002 Nasca Octavian Paul
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
#include <jack/midiport.h>
#include "JACKaudiooutput.h"

Master *jackmaster;
jack_client_t *jackclient;
char jackname[100];
jack_port_t *outport_left,*outport_right,*midi_inport;

int jackprocess(jack_nframes_t nframes,void *arg);
int jacksrate(jack_nframes_t nframes,void *arg);
void jackshutdown(void *arg);

bool JACKaudiooutputinit(Master *master_)
{
    jackmaster=master_;
    jackclient=0;

    for (int i=0;i<15;i++) {
        if (i!=0) snprintf(jackname,100,"ZynAddSubFX_%d",i);
        else snprintf(jackname,100,"ZynAddSubFX");
        jackclient=jack_client_new(jackname);
        if (jackclient!=0) break;
    };

    if (jackclient==0) {
        fprintf(stderr,"\nERROR: Cannot make a jack client (possible reasons: JACK server is not running or jackd is launched by root and zynaddsubfx by another user.).\n");
        return(false);
    };

    fprintf(stderr,"Internal SampleRate   = %d\nJack Output SampleRate= %d\n",SAMPLE_RATE,jack_get_sample_rate(jackclient));
    if ((unsigned int)jack_get_sample_rate(jackclient)!=(unsigned int) SAMPLE_RATE)
        fprintf(stderr,"It is recomanded that the both samplerates to be equal.\n");

    jack_set_process_callback(jackclient,jackprocess,0);
    jack_set_sample_rate_callback(jackclient,jacksrate,0);
    jack_on_shutdown(jackclient,jackshutdown,0);

    outport_left=jack_port_register(jackclient,"out_1",
                                    JACK_DEFAULT_AUDIO_TYPE,JackPortIsOutput|JackPortIsTerminal,0);
    outport_right=jack_port_register(jackclient,"out_2",
                                     JACK_DEFAULT_AUDIO_TYPE,JackPortIsOutput|JackPortIsTerminal,0);
    midi_inport=jack_port_register(jackclient,"midi_input",
                                   JACK_DEFAULT_MIDI_TYPE,JackPortIsInput|JackPortIsTerminal,0);

    if (jack_activate(jackclient)) {
        fprintf(stderr,"Cannot activate jack client\n");
        return(false);
    };

    /*
    jack_connect(jackclient,jack_port_name(outport_left),"alsa_pcm:out_1");
    jack_connect(jackclient,jack_port_name(outport_right),"alsa_pcm:out_2");
     */
    return(true);
};

int jackprocess(jack_nframes_t nframes,void *arg)
{
    jack_default_audio_sample_t *outl=(jack_default_audio_sample_t *) jack_port_get_buffer (outport_left, nframes);
    jack_default_audio_sample_t *outr=(jack_default_audio_sample_t *) jack_port_get_buffer (outport_right, nframes);

    if (!pthread_mutex_trylock(&jackmaster->mutex)) {
        JACKhandlemidi(nframes);
        jackmaster->GetAudioOutSamples(nframes,jack_get_sample_rate(jackclient),outl,outr);
        pthread_mutex_unlock(&jackmaster->mutex);
    } else {
        memset(outl, 0, sizeof(jack_default_audio_sample_t) * nframes);
        memset(outr, 0, sizeof(jack_default_audio_sample_t) * nframes);
    }

    return(0);
};

void JACKfinish()
{
    jack_client_close(jackclient);
};

int jacksrate(jack_nframes_t nframes,void *arg)
{

    return(0);
};

void jackshutdown(void *arg)
{
};


void JACKhandlemidi(unsigned long frames)
{

    // We must have the master mutex before we run this function

    // XXX This is really nasty, not only do we lose the sample accuracy of
    // JACK MIDI, but any accuracy at all below the buffer size

    void* midi_buf = jack_port_get_buffer(midi_inport, frames);
    jack_midi_event_t jack_midi_event;
    jack_nframes_t event_index = 0;
    unsigned char* midi_data;
    unsigned char type, chan;

    while (jack_midi_event_get(&jack_midi_event,midi_buf, event_index++) == 0) {
        midi_data = jack_midi_event.buffer;
        type = midi_data[0] & 0xF0;
        chan = midi_data[0] & 0x0F;

        switch (type) {

        case 0x80: /* note-off */
            jackmaster->NoteOff(chan, midi_data[1]);
            break;

        case 0x90: /* note-on */
            jackmaster->NoteOn(chan, midi_data[1], midi_data[2]);
            break;

        case 0xB0: /* controller */
            jackmaster->SetController(chan, midi_data[1], midi_data[2]);
            break;

        case 0xE0: /* pitch bend */
            jackmaster->SetController(chan, C_pitchwheel,
                                      ((midi_data[2] << 7) | midi_data[1]));
            break;

            /* XXX TODO: handle MSB/LSB controllers and RPNs and NRPNs */
        }
    }

}


const char* JACKgetname()
{
    if (jackclient != NULL)
        return jackname;
    return NULL;
}
