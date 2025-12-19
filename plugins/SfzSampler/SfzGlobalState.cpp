

#include "SfzGlobalState.h"

namespace lmms
{

SfzGlobalState::SfzGlobalState(const int numRegions)
{
	m_regionNoteCounts.resize(numRegions);
}

void SfzGlobalState::processTrigger(const SfzTrigger& trigger)
{
}

void SfzGlobalState::regionTriggered(const SfzRegion& region)
{
}

void SfzGlobalState::regionEnded(const SfzRegion& region)
{
}


} // namespace lmms