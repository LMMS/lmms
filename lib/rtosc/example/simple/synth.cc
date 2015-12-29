#include <math.h>
#include <stdio.h>
#include <string.h>
#include <err.h>
#include <jack/jack.h>
#include <jack/ringbuffer.h>
#include <rtosc.h>
#include <thread-link.h>

#define OFF (0)

//Shape of oscillator
//0 - sinusoidal
//1 - sawtooth
//2 - rectangular (25% duty cycle)
static int shape = 0; 

//Current frequency
static float freq = 440.0f;

//Whether the note is running or not
static bool gate = OFF;

//Synthesis
inline float get_sample(const float phase)
{
    if(shape == 0)
        return sin(phase*2*M_PI);
    else if(shape == 1)
        return phase*2.0f-1.0f;
    else if(shape == 2)
        return phase > 0.75 ? 1.0 : 0.0f;
    return 0.0f;
}

//Setter functions
#define setter(type_ch, type) std::function<void(msg_t)> type_ch##setter(type &param) { \
    return [&param](msg_t msg){param = argument(msg, 0).type_ch;}; \
}
setter(i,int);
setter(f,float);
setter(T,bool);

ThreadLink<2048,16> bToU,uToB;

void plot_data_cb(const char *msg)
{
    const int samples = argument(msg, 0).i;
    char *buffer = bToU.buffer();

    //Construct blob piecewise
    if(sosc(buffer, bToU.buffer_size(), "/ui/plot", "b", samples*sizeof(float), NULL)) {

        //Fill reserved space
        float *data = (float*) argument(buffer,0).b.data;
        for(int i=0; i < samples; ++i)
            data[i] = get_sample((float)i/samples);

        bToU.raw_write(buffer);
    }
}


static bool _T(msg_t m){(void) m;return true;};

Dispatch<4> dispatchB{
    std::make_tuple("/synth/shape",     "i", _T, isetter(shape)),
    std::make_tuple("/synth/freq",      "f", _T, fsetter(freq)),
    std::make_tuple("/synth/gate",      "T", _T, Tsetter(gate)),
    std::make_tuple("/synth/plot/data", "i", _T, plot_data_cb),
};

//JACK stuff
jack_port_t   *jport;
jack_client_t *jclient;
int process(unsigned nframes, void *args);
void gui_dispatch(const char *msg);


void start_synth(void)
{
    jclient = jack_client_open("rtosc-demo", JackNullOption, NULL, NULL);
    if(!jclient)
        errx(1, "jack_client_open() failure");

    jack_set_process_callback(jclient, process, NULL);

    jport = jack_port_register(jclient, "output",
            JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput | JackPortIsTerminal, 0);

    if(!jport)
        errx(1, "jack_port_register() failure");

    if(jack_activate(jclient))
        errx(1, "jack_activate() failure");
}

void stop_synth(void)
{
    jack_client_close(jclient);
}

float phase = 0.0f;
int process(unsigned nframes, void *args)
{
    (void) args;
    while(uToB.hasNext())
        dispatchB(uToB.read());

    float *smps = (float*) jack_port_get_buffer(jport, nframes);
    memset(smps, 0, nframes*sizeof(float));

    const float dp = freq/jack_get_sample_rate(jclient);
    for(unsigned i=0; i<nframes; ++i) {
        smps[i] = get_sample(phase);
        phase += dp;
        if(phase > 1.0f)
            phase -= 1.0f;
    }
    return 0;
}
