#include <cmath>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <string>
#include "../Misc/MiddleWare.h"
#include "../Misc/Part.h"
#include "../Misc/Util.h"
#include "../Misc/Allocator.h"
#include "../Misc/Microtonal.h"
#include "../DSP/FFTwrapper.h"
#include "../globals.h"
SYNTH_T *synth;

using namespace std;

char *instance_name=(char*)"";
MiddleWare *middleware;

enum RunMode {
    MODE_PROFILE,
    MODE_TEST,
};

RunMode mode;

float *outL, *outR;
Allocator  alloc;
Microtonal microtonal;
FFTwrapper fft(1024);
Part *p;

double tic()
{
    timespec ts;
    // clock_gettime(CLOCK_MONOTONIC, &ts); // Works on FreeBSD
    clock_gettime(CLOCK_REALTIME, &ts);
    double t = ts.tv_sec;
    t += 1e-9*ts.tv_nsec;
    return t;
}

double toc()
{
    return tic();
}

void setup() {
    synth = new SYNTH_T;
    synth->buffersize = 256;
    synth->samplerate = 48000;
    synth->alias();
    //for those patches that are just really big
    alloc.addMemory(malloc(1024*1024),1024*1024);

    outL  = new float[1024];
    for(int i = 0; i < synth->buffersize; ++i)
        outL[i] = 0.0f;
    outR = new float[1024];
    for(int i = 0; i < synth->buffersize; ++i)
        outR[i] = 0.0f;

    //next the bad global variables that for some reason have not been properly placed in some
    //initialization routine, but rather exist as cryptic oneliners in main.cpp:
    denormalkillbuf = new float[synth->buffersize];
    for(int i = 0; i < synth->buffersize; ++i)
        denormalkillbuf[i] = 0;
    p = new Part(alloc, &microtonal, &fft);
}

void xml(string s)
{
    double t_on = tic();
    p->loadXMLinstrument(s.c_str());
    double t_off = toc();
    if(mode == MODE_PROFILE)
        printf("%f, ", t_off - t_on);
}

void load()
{
    double t_on = tic();
    p->applyparameters();
    p->initialize_rt();
    double t_off = toc();
    if(mode == MODE_PROFILE)
        printf("%f, ", t_off - t_on);
}

void noteOn()
{
    double t_on = tic(); // timer before calling func
    for(int i=40; i<100; ++i)
        p->NoteOn(i,100,0);
    double t_off = toc(); // timer when func returns
    if(mode == MODE_PROFILE)
        printf("%f, ", t_off - t_on);
}

void speed()
{
    const int samps = 150;

    double t_on = tic(); // timer before calling func
    for(int i = 0; i < samps; ++i) {
        p->ComputePartSmps();
        if(mode==MODE_TEST)
            printf("%f, %f, ", p->partoutl[0], p->partoutr[0]);
    }
    double t_off = toc(); // timer when func returns

    if(mode==MODE_PROFILE)
        printf("%f, %d, ", t_off - t_on, samps*synth->buffersize);
}

void noteOff()
{
    double t_on = tic(); // timer before calling func
    p->AllNotesOff();
    p->ComputePartSmps();
    double t_off = toc(); // timer when func returns
    if(mode == MODE_PROFILE)
        printf("%f, ", t_off - t_on);
}

void memUsage()
{
    if(mode == MODE_PROFILE)
        printf("%lld", alloc.totalAlloced());
}

int main(int argc, char **argv)
{
    if(argc != 2) {
        fprintf(stderr, "Please supply a xiz file\n");
        return 1;
    }

    mode = MODE_TEST;
    setup();
    xml(argv[1]);
    load();
    noteOn();
    speed();
    noteOff();
    memUsage();
    printf("\n");
}
