#include "Effect.h"

struct Echo : public Effect
{
    float time;//sec
    static rtosc::Ports ports;
};
