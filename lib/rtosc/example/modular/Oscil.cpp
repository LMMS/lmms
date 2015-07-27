#include "Oscil.h"
#include "util.h"

rtosc::Ports Oscillator::ports = {
    PARAM(Oscillator, freq, freq, log, 0.01, 1e3, "frequency")
};

