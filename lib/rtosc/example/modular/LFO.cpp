#include <rtosc/ports.h>
#include "LFO.h"
#include "util.h"

using namespace rtosc;

Ports LFO::ports = {
    PARAM(LFO, freq, freq, log, 1e-3, 10, "frequency"),
};
