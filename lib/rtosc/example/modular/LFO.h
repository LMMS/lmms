#include "Effect.h"

struct LFO : public Effect
{
    float freq;//Hz
    static rtosc::Ports ports;
};
