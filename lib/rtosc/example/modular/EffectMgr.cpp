#include "EffectMgr.h"
#include "Echo.h"
#include "LFO.h"
#include "util.h"

rtosc::Ports EffectMgr::ports = {
    OPTION(EffectMgr, Echo, echo, eff),
    OPTION(EffectMgr, LFO,  lfo,  eff),
    DUMMY(type)
};

EffectMgr::EffectMgr(void)
    :eff(NULL){}
EffectMgr::~EffectMgr(void)
{
    delete eff;
}
