#include "Oscil.h"
#include "EffectMgr.h"

struct Synth
{
    float volume;
    Oscillator oscil;
    EffectMgr effects[8];
    static rtosc::Ports ports;
};
