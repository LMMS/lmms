#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <err.h>
#include <jack/jack.h>
#include <jack/midiport.h>

extern float Fs;
extern float &freq;
extern bool  &gate;
void process_output(float *f, unsigned nframes);
int current_note = 0;

void dsp_dispatch(const char *msg);


//JACK stuff
jack_port_t   *port, *iport;
jack_client_t *client;
int process(unsigned nframes, void *args);
void synth_init(void);
void audio_cleanup(void)
{
    puts("Exiting jack...");
    jack_deactivate(client);
    jack_client_close(client);
}
void audio_init(void)
{
    synth_init();
    client = jack_client_open("rtosc-demo2", JackNullOption, NULL, NULL);
    if(!client)
        errx(1, "jack_client_open() failure");

    jack_set_process_callback(client, process, NULL);

    port = jack_port_register(client, "output",
            JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput | JackPortIsTerminal, 0);

    //Make midi port
    iport = jack_port_register(client, "input",
            JACK_DEFAULT_MIDI_TYPE, JackPortIsInput | JackPortIsTerminal, 0);

    if(!port)
        errx(1, "jack_port_register() failure");

    if(jack_activate(client))
        errx(1, "jack_activate() failure");

    atexit(audio_cleanup);
}

void process_control(unsigned char control[3]);
int process(unsigned nframes, void *)
{
    Fs = jack_get_sample_rate(client);

    //Handle midi first
    void *midi_buf = jack_port_get_buffer(iport, nframes);
    jack_midi_event_t ev;
    jack_nframes_t event_idx = 0;
    while(jack_midi_event_get(&ev, midi_buf, event_idx++) == 0) {
        switch(ev.buffer[0]&0xf0) {
            case 0x90: //Note On
                freq = 440.0f * powf(2.0f, (ev.buffer[1]-69.0f)/12.0f);
                current_note = ev.buffer[1];
                gate = 1;
                break;
            case 0x80: //Note Off
                if(current_note == ev.buffer[1])
                    current_note = gate =0;
                break;
            case 0xB0: //Controller
                process_control(ev.buffer);
                break;
        }
    }

    //Get all samples
    float *smps = (float*) jack_port_get_buffer(port, nframes);
    process_output(smps, nframes);

    return 0;
}
