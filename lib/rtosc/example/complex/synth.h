#ifndef SYNTH_H
#define SYNTH_H

struct Adsr
{
    Adsr(void) :at(0.05),dt(0.05),rt(0.05), av(-1.0), dv(1.0), sv(0.0), rv(-1.0) {}
    float time;
    float relval;
    bool  pgate;

    float at,dt,   rt;
    float av,dv,sv,rv;

    float operator()(bool gate);
    static struct rtosc::Ports ports;
};

struct Synth {
    Adsr amp_env, frq_env;
    float freq;
    bool  gate;

    float sample(void);
    static struct rtosc::Ports ports;
};
#endif
