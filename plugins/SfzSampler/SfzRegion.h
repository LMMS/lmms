
#ifndef LMMS_SFZ_REGION_H
#define LMMS_SFZ_REGION_H

#include "SfzOpcodeState.h"

namespace lmms
{

class SfzRegion : public SfzOpcodeState
{
public:
	SfzRegion(SfzOpcodeState opcodeState);
};


} // namespace lmms


#endif // LMMS_SFZ_REGION_H