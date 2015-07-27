#include "Echo.h"
#include "util.h"

rtosc::Ports Echo::ports = {
    PARAM(Echo, time, time, log, 1e-3, 10, "Delay of echo")
};
