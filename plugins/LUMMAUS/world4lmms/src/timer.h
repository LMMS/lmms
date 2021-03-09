#ifndef _H_TIMER_
#define _H_TIMER_

#include "types.h"
#include "log.h"

#define PROFILER_START(label) uint64 timer_start_##label = GetTimeMs64()
#define PROFILER_END(label) log_info("Elapse time " #label " in ms: %lld\n", GetTimeMs64() - timer_start_##label)

uint64 GetTimeMs64();

#endif