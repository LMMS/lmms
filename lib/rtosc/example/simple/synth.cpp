#include <math.h>
#include <stdio.h>
#include <string.h>
#include <err.h>
#include <jack/jack.h>
#include <jack/ringbuffer.h>
#include <rtosc/rtosc.h>

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
float get_sample(const float phase)
{
    if(shape == 0)
        return sin(phase*2*M_PI);
    else if(shape == 1)
        return phase*2.0f-1.0f;
    else if(shape == 2)
        return phase > 0.75 ? 1.0 : 0.0f;
    return 0.0f;
}

//OSC
static char dsp_osc_buf[2048];
extern char gui_osc_buf[];
void gui_message(const char *msg);
#define message(path, type, ...) do { \
    if(rtosc_message(dsp_osc_buf, 2048, path, ...)) \
        gui_message(dsp_osc_buf); \
    else \
        warnx("Message to %s is too long...", path);\
}

//Setter functions
#define setter(type_ch, type) void type_ch##setter(const char *osc_msg, void *param) { \
    *((type*)param) = rtosc_argument(osc_msg, 0).type_ch; \
}
setter(i,int);
setter(f,float);
setter(T,bool);


void plot_data_cb(const char *msg, void*)
{
    const int samples = rtosc_argument(msg, 0).i;

    //Construct blob piecewise
    if(rtosc_message(dsp_osc_buf, 2048, "/ui/plot", "b", samples, NULL)) {

        //Fill reserved space
        float *data = (float*) rtosc_argument(dsp_osc_buf,0).b.data;
        for(int i=0; i < samples; ++i)
            data[i] = get_sample((float)i/samples);

    }
    gui_message(dsp_osc_buf);
}

struct dispatch_t
{
    const char *label;
    void (*fn)(const char *,void*);
    void *data;
};

//Simple dispatch table
dispatch_t dispatch_table[] = {
    {"/synth/shape", isetter, &shape},
    {"/synth/freq",  fsetter, &freq},
    {"/synth/gate",  Tsetter, &gate},
    {"/synth/plot/data", plot_data_cb, 0},
    {0,0,0}
};

void dsp_dispatch(const char *msg)
{
    dispatch_t *itr = dispatch_table;
    while(itr->label && strcmp(msg, itr->label))
        ++itr;
    if(itr->label)
        itr->fn(msg, itr->data);
}


//JACK stuff
jack_port_t   *jport;
jack_client_t *jclient;
jack_ringbuffer_t *gui_ring;
jack_ringbuffer_t *dsp_ring;
int process(unsigned nframes, void *args);
void dsp_ev(void);
void gui_dispatch(const char *msg);
void dsp_message(const char *msg)
{
    if(jack_ringbuffer_write_space(dsp_ring) >= 2048)
        jack_ringbuffer_write(dsp_ring, msg, 2048);
    else
        puts("dsp ringbuffer full...");
}
void gui_message(const char *msg)
{
    if(jack_ringbuffer_write_space(gui_ring) >= 2048)
        jack_ringbuffer_write(gui_ring, msg, 2048);
}
void dsp_ev(void)
{
    while(jack_ringbuffer_read_space(dsp_ring) >= 2048) {
        jack_ringbuffer_read(dsp_ring, dsp_osc_buf, 2048);
        dsp_dispatch(dsp_osc_buf);
    }
}
void gui_ev(void)
{
    while(jack_ringbuffer_read_space(gui_ring) >= 2048) {
        jack_ringbuffer_read(gui_ring, gui_osc_buf, 2048);
        gui_dispatch(gui_osc_buf);
    }
}
void start_synth(void)
{
    //Setup Ringbuffers
    gui_ring = jack_ringbuffer_create(2048*8);
    dsp_ring = jack_ringbuffer_create(2048*32);
    jack_ringbuffer_mlock(gui_ring);
    jack_ringbuffer_mlock(dsp_ring);
    
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
int process(unsigned nframes, void *)
{
    dsp_ev();
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
