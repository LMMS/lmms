
#ifndef LMMS_SFZ_REGION_H
#define LMMS_SFZ_REGION_H

#include "SfzOpcodeState.h"
#include "SfzGlobalState.h"
#include "SfzTrigger.h"

namespace lmms
{

class SfzRegion : public SfzOpcodeState
{
public:
	SfzRegion(SfzOpcodeState opcodeState);

	//! Returns true if the trigger event matches all of the requirements defined by the opcodes of this region
	//! For example, if lokey=24 and hikey=29, and the trigger is a NoteOn event on key 26, then it will return true
	//! If the trigger does not fall in the key range or velocity range or any other conditions are not met (including global state conditions, 
	//! such as which switch key was last pressed) this will return false.
	bool triggerConditionsMet(const SfzGlobalState& globalState, const SfzTrigger& trigger);
};


} // namespace lmms


#endif // LMMS_SFZ_REGION_H