#pragma once
namespace rtosc{struct Ports;};

struct EffectMgr
{
    union {
        struct Effect *eff;
        struct Echo   *echo;
        struct LFO    *lfo;
    };
    EffectMgr(void);
    ~EffectMgr(void);
    static rtosc::Ports ports;
};
