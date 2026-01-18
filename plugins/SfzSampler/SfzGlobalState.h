
#ifndef LMMS_SFZ_GLOBAL_STATE_H
#define LMMS_SFZ_GLOBAL_STATE_H

#include <array>
#include <optional>

#include "SfzOpcodeState.h"
#include "SfzTrigger.h"

namespace lmms
{


class SfzGlobalState
{
public:
	//! Handles updating the last played keys and keeps track of which keys are currently being pressed
	void processTrigger(const SfzTrigger& trigger);

	// Returns the midi key number of the last pressed key in the range [lowKey, highKey].
	// If no keys have been pressed yet, it returns defaultKey.
	// If the key range is invalid, it returns defaultKey
	// If no default key was given, it returns std::nullopt
	// TODO add unit tests
	std::optional<int> lastKeyPressedInRange(int lowKey, int highKey, const std::optional<int> defaultKey) const;

	//! Returns the current value of the given midi CC knob/controller, or the default value if we haven't recieved any midi CC signals for it yet.
	int midiCCValue(const int index) const { return m_ccValues.at(index); }

	// Sets initial values for the controls, based on any `set_ccN` opcodes in the <control> header
	void initializeMidiCCValues(const SfzOpcodeState& controlsConfig);

private:
	//! Stores a number for every key, tracking which order the keys were played in
	//! The first key played gets a 1, the second key gets a 2, etc, overwriting when keys are played multiple times.
	//! By finding the maximum number in a range, you can find the last played key
	std::array<std::optional<int>, 128> m_lastPlayedKeys = {};
	//! Consequently, we also have to track how many keys have been played so far
	unsigned long m_keyPressCounter = 0;

	//! Keeps track of which keys on the piano are currently being pressed
	std::array<bool, 128> m_activeKeys = {};

	//! Stores the current value of all the midi CC knobs/controllers
	//! Technically, floats should probably be used to allow for HDCC (high definition CC's) as used in ARIA, but for now dividing by 127 to get a float between 0 and 1 works fine.
	//! std::optional is used so that sfz file can specify default cc values
	std::array<int, SfzOpcodeState::NumMidiCCs> m_ccValues = {};
};


} // namespace lmms


#endif // LMMS_SFZ_GLOBAL_STATE_H