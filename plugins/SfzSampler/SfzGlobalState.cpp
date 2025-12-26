

#include "SfzGlobalState.h"

namespace lmms
{

void SfzGlobalState::processTrigger(const SfzTrigger& trigger)
{
	if (trigger.type() == SfzTrigger::Type::ControlChange)
	{
		m_ccValues.at(trigger.controlChangeNumber().value()) = trigger.controlChangeValue().value();
	}
}

int SfzGlobalState::lastKeyPressedInRange(const int lowKey, const int highKey)
{
	return 0;
}


} // namespace lmms