
#ifndef LMMS_SFZ_GLOBAL_STATE_H
#define LMMS_SFZ_GLOBAL_STATE_H

#include <array>
#include <optional>

#include "SfzTrigger.h"

namespace lmms
{


class SfzGlobalState
{
public:
	//! Handles updating the last played keys and keeps track of which keys are currently being pressed
	void processTrigger(const SfzTrigger& trigger);

	// Returns the midi key number of the last pressed key in the range [lowKey, highKey]
	int lastKeyPressedInRange(const int lowKey, const int highKey);

private:
	//! Stores a number for every key, tracking which order the keys were played in
	//! The first key played gets a 1, the second key gets a 2, etc, overwriting when keys are played multiple times/
	//! By finding the maximum number in a range, you can find the last played key
	std::array<std::optional<int>, 128> m_lastPlayedKeys;

	//! Keeps track of which keys on the piano are currently being pressed
	std::array<bool, 128> m_activeKeys;
};


} // namespace lmms


#endif // LMMS_SFZ_GLOBAL_STATE_H