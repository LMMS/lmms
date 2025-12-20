





#include "SfzRegion.h"

#include "Engine.h"
#include "AudioEngine.h"

#include <QDebug>

namespace lmms
{


SfzRegion::SfzRegion(SfzOpcodeState opcodeState)
	: SfzOpcodeState(opcodeState)
{
}

bool SfzRegion::triggerConditionsMet(const SfzGlobalState& globalState, const SfzTrigger& trigger)
{
	return false;
}

void SfzRegion::processTrigger(const SfzGlobalState& globalState, const SfzTrigger& trigger)
{
	// If the trigger conditions are met, spawn a new sound
	if (triggerConditionsMet(globalState, trigger))
	{
		// Loop through array to find open position
		bool foundOpenPosition = false;
		for (auto& regionPlayState : m_activeSounds)
		{
			if (!regionPlayState.active())
			{
				regionPlayState = SfzRegionPlayState(this, trigger);
				foundOpenPosition = true;
				break;
			}
		}
		if (!foundOpenPosition) { qDebug() << "[SFZ Player] Could not find vacant position in RegionPlayState buffer!"; }
	}

	// Loop through all the active sounds to check if any need to be deactivated/released by the trigger
	for (auto& regionPlayState : m_activeSounds)
	{
		if (regionPlayState.active())
		{
			regionPlayState.processTrigger(trigger);
		}
	}
}


bool SfzRegion::play(SampleFrame* workingBuffer, SampleFrame* temporaryBuffer, const fpp_t frames)
{
	bool anythingPlayed = false;
	for (auto& regionPlayState : m_activeSounds)
	{
		anythingPlayed = anythingPlayed || regionPlayState.play(temporaryBuffer, frames);
		if (anythingPlayed)
		{
			for (f_cnt_t f = 0; f < frames; ++f) { workingBuffer[f] += temporaryBuffer[f]; }
		}
	}
	return anythingPlayed;
}


} // namespace lmms