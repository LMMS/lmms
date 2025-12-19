

#include "SfzRegionPlayState.h"

namespace lmms
{

SfzRegionPlayState::SfzRegionPlayState(const SfzRegion& region, const SfzTrigger& trigger)
	: m_trigger(trigger)
	, m_region(&region)
{
}


void SfzRegionPlayState::play(SampleFrame* buffer, fpp_t frames)
{
}


void SfzRegionPlayState::processTrigger(const SfzTrigger& trigger)
{
}

} // namespace lmms