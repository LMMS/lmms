

#include "SfzRegionPlayState.h"

namespace lmms
{

SfzRegionPlayState::SfzRegionPlayState(const SfzRegion* region, const SfzTrigger& trigger)
	: m_trigger(trigger)
	, m_region(region)
{
}


bool SfzRegionPlayState::play(SampleFrame* buffer, const fpp_t frames)
{
	return false;
}


void SfzRegionPlayState::processTrigger(const SfzTrigger& trigger)
{
}

} // namespace lmms