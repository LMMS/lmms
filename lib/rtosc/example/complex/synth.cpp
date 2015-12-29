#include <rtosc/rtosc.h>
#include <rtosc/thread-link.h>
#include <rtosc/ports.h>
#include <rtosc/miditable.h>
#include <string.h>
#include <cmath>
#include "synth.h"
#include <rtosc/port-sugar.h>
#include <rtosc/subtree-serialize.h>

using namespace rtosc;

float Fs = 0.0f;

ThreadLink bToU(1024,1024);
ThreadLink uToB(1024,1024);

#define rObject Adsr

Ports Adsr::ports = {
    rParamF(av, rLinear(-1,1), "attack  value"),
    rParamF(dv, rLinear(-1,1), "decay   value"),
    rParamF(sv, rLinear(-1,1), "sustain value"),
    rParamF(rv, rLinear(-1,1), "release value"),

    rParamF(at, rLog(1e-3,10), "attach  time"),
    rParamF(dt, rLog(1e-3,10), "decay   time"),
    rParamF(rt, rLog(1e-3,10), "release time"),
};

//sawtooth generator
float oscil(float freq)
{
    static float phase = 0.0f;
    phase += freq/Fs;
    if(phase > 1.0f)
        phase -= 1.0f;

    return phase;
}

inline float Synth::sample(void)
{
    return oscil(freq*(1+frq_env(gate))/2.0f)*(1+amp_env(gate))/2.0f;
}

float interp(float a, float b, float pos)
{
    return (1.0f-pos)*a+pos*b;
}

//Linear ADSR
float Adsr::operator()(bool gate)
{
    time += 1.0/Fs;

    if(gate == false && pgate == true)
        relval = (*this)(true);
    if(gate != pgate)
        time = 0.0f;
    pgate = gate;

    float reltime = time;
    if(gate) {
        if(at > reltime) //Attack
            return interp(av, dv, reltime/at);
        reltime -= at;
        if(dt > reltime) //Decay
            return interp(dv, sv, reltime/dt);
        return sv;        //Sustain
    }
    if(rt > reltime)     //Release
        return interp(relval, rv, reltime/rt);

    return rv;
}

MidiTable midi(Synth::ports);
char   serial[2048];
size_t serial_size;

Synth s;
void process_control(unsigned char control[3]);

#undef  rObject
#define rObject Synth

rtosc::Ports Synth::ports = {
    rRecur(amp_env, "amplitude envelope"),
    rRecur(frq_env, "frequency envelope"),
    rParamF(freq, rLog(1,1e3), "note frequency"),
    rToggle(gate, "Note enable"),
    midi.registerPort(),
    midi.learnPort(),
    midi.unlearnPort(),
    {"echo:s", ":internal\0", NULL, [](const char *msg, RtData &d)
        {
            d.reply(rtosc_argument(msg, 0).s);
        }},
    {"save", ":internal\0", NULL, [](const char *, RtData &data)
        {
        fprintf(stderr, "saving...\n");
            serial_size = subtree_serialize(serial, sizeof(serial),
                    data.obj, &Synth::ports);
        }},
    {"load", ":internal\0", NULL, [](const char *, RtData &data)
        {
            memset(data.loc, 0, data.loc_size);
            fprintf(stderr, "loading...\n");
            subtree_deserialize(serial, serial_size, data.obj, &Synth::ports, data);
        }},
};

Ports *root_ports = &Synth::ports;


float &freq = s.freq;
bool  &gate = s.gate;

void event_cb(msg_t m)
{
    char buffer[1024];
    rtosc::RtData d;
    d.loc      = buffer;
    d.loc_size = 1024;
    d.obj      = (void*) &s;
    Synth::ports.dispatch(m+1, d);
    bToU.raw_write(m);
    puts("event-cb");
    if(rtosc_type(m,0) == 'f')
        printf("%s -> %f\n", m, rtosc_argument(m,0).f);
    if(rtosc_type(m,0) == 'i')
        printf("%s -> %d\n", m, rtosc_argument(m,0).i);
}

void modify_cb(const char *action, const char *path, const char *, int ch, int cc)
{
    if(!strcmp(action, "ADD") || !strcmp(action, "REPLACE"))
        bToU.write("/midi/add", "sii", path, ch, cc);
    else if(!strcmp(action, "DEL"))
        bToU.write("/midi/remove", "s", path);
}

#include <err.h>
void synth_init(void)
{
    printf("%p\n", Adsr::ports["dv"]->metadata);
    printf("'%d'\n", Adsr::ports["dv"]->metadata[0]);
    if(strlen(Adsr::ports["dv"]->metadata)<3)
        errx(1,"bad metadata");
    midi.event_cb  = event_cb;
    midi.modify_cb = modify_cb;
}

void process_control(unsigned char ctl[3])
{
    midi.process(ctl[0]&0x0F, ctl[1], ctl[2]);
}

class DispatchData:public rtosc::RtData
{
    public:
        DispatchData(void)
        {
            memset(buffer, 0, 1024);
            loc = buffer;
            loc_size = 1024;
            obj = &s;
        }
        char buffer[1024];

        void reply(const char *path, const char *args, ...)
        {
            va_list va;
            va_start(va,args);
            const size_t len =
                rtosc_vmessage(bToU.buffer(),bToU.buffer_size(),path,args,va);
            if(len)
                bToU.raw_write(bToU.buffer());
            va_end(va);
        }

        void reply(const char *msg)
        {
            bToU.raw_write(msg);
        }

        void broadcast(const char *path, const char *args, ...)
        {
            bToU.write("/broadcast","");
            va_list va;
            va_start(va,args);
            const size_t len =
                rtosc_vmessage(bToU.buffer(),bToU.buffer_size(),path,args,va);
            if(len)
                bToU.raw_write(bToU.buffer());
            va_end(va);
        }


        void broadcast(const char *msg)
        {
            bToU.write("/broadcast","");
            bToU.raw_write(msg);
        }

};

void process_output(float *smps, unsigned nframes)
{
    DispatchData d;
    while(uToB.hasNext()) {
        Synth::ports.dispatch(uToB.read()+1, d);
        fprintf(stderr, "backend '%s'\n", uToB.peak());
    }

    for(unsigned i=0; i<nframes; ++i)
        smps[i] = s.sample();
}
