
#ifndef LMMS_SFZ_GLOBAL_STATE_H
#define LMMS_SFZ_GLOBAL_STATE_H

#include <array>
#include <optional>

#include "SfzTrigger.h"
#include "SfzRegion.h"

namespace lmms
{


class SfzGlobalState
{
public:
	SfzGlobalState(const int numRegions);
	SfzGlobalState() = default;

	//! Handles updating the last played keys and keeps track of which keys are currently being pressed
	void processTrigger(const SfzTrigger& trigger);

	//! Handles keeping track of how many sounds are actively being played per region
	void regionTriggered(const int regionNumber);
	void regionEnded(const int regionNumber);

private:
	//! Stores a number for every key, tracking which order the keys were played in
	//! The first key played gets a 1, the second key gets a 2, etc, overwriting when keys are played multiple times/
	//! By finding the maximum number in a range, you can find the last played key
	std::array<std::optional<int>, 128> m_lastPlayedKeys;

	//! Keeps track of which keys on the piano are currently being pressed
	std::array<bool, 128> m_activeKeys;

	//! Keeps track of the number of active sounds on each region
	//! This is stored as a vector, but it is resized/initialized to the correct length when the SFZ file is parsed, so no runtime allocations are performed
	std::vector<int> m_regionNoteCounts;
};


} // namespace lmms


#endif // LMMS_SFZ_GLOBAL_STATE_H