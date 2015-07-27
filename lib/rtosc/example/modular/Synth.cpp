#include "Synth.h"
#include "EffectMgr.h"
#include "util.h"
#include <rtosc/ports.h>
using namespace rtosc;

Ports Synth::ports = {
    PARAM(Synth, volume, volume, log, 1e-3, 1, "loudness of output"),
    RECUR(Synth, Oscillator, oscil, oscil),
    RECURS(Synth, EffectMgr, effect, effects, 8)

};
