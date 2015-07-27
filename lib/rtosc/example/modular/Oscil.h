#pragma once
namespace rtosc{struct Ports;};

struct Oscillator
{
    float freq;
    static rtosc::Ports ports;
};
