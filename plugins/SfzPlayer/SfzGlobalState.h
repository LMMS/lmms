/*
 * SfzGlobalState.h - Class which holds information about the current state of the
 * SFZ player, such as currently/previously pressed keys, and current midi CC values
 *
 * Copyright (c) 2026 Keratin
 *
 * This file is part of LMMS - https://lmms.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 *
 */

#ifndef LMMS_SFZ_GLOBAL_STATE_H
#define LMMS_SFZ_GLOBAL_STATE_H

#include <array>
#include <map>
#include <optional>

#include "SfzControlsConfig.h"
#include "SfzTrigger.h"

namespace lmms
{


class SfzGlobalState
{
public:
	//! Handles updating the last played switch keys and keeps track of which keys are currently being pressed
	void processTrigger(const SfzTrigger& trigger);

	//! Returns the midi key number of the last pressed switch key in the range [lowKey, highKey].
	//! It uses m_lastPlayedSwitchKeys array which only tracks when valid switch keys have been pressed, not just any key.
	//! If no keys have been pressed yet, it returns defaultKey.
	//! If the key range is invalid, it returns defaultKey
	//! If no default key was given, it returns std::nullopt
	// TODO add unit tests
	std::optional<int> lastSwitchKeyPressedInRange(int lowKey, int highKey, const std::optional<int> defaultKey) const;

	//! Returns the current value of the given midi CC knob/controller, or the default value if we haven't recieved any midi CC signals for it yet.
	int midiCCValue(const int index) const { return m_ccValues.at(index); }
	//! Returns a const array of the current CC knob values
	const std::array<int, NumMidiCCs> midiCCValues() const { return m_ccValues; }

	//! Returns the random value for the current trigger
	const float rand() const { return m_rand; }

	//! Sets initial values for the controls, based on any `set_ccN` opcodes in the <control> header
	void initializeMidiCCValues(const SfzControlsConfig& controlsConfig);

	//! Creates a std::map with an element for each switch key defined in the sfz file, indexed by key, with the value being the key press counter of the last time that key was pressed (which is initially 0, or std::nullopt)
	void initializeSwitchKeysMap(const SfzControlsConfig& controlsConfig);

private:
	//! Stores a number for every switch key, tracking which order the keys were played in
	//! The first key played gets a 1, the second key gets a 2, etc, overwriting when keys are played multiple times.
	//! By finding the maximum number in a range, you can find the last played key
	//! This uses a std::map instead of a 128-element array for speed, since most keys are not switch keys.
	//! The index is the key number, and the value is the key press counter when it was last pressed, or std::nullopt if it has never been pressed.
	std::map<int, std::optional<int>> m_lastPlayedSwitchKeys = {};
	//! Stores the midi key index of the last played switch key, for easy access.
	std::optional<int> m_lastPlayedSwitchKey = std::nullopt;
	//! Consequently, we also have to track how many switch keys have been played so far
	unsigned long m_switchKeyPressCounter = 0;

	//! Keeps track of which switch keys on the piano are currently being pressed
	std::map<int, bool> m_pressedSwitchKeys = {}; // TODO this is not currently used, but may be once `sw_down` opcode is implemented.

	//! Keeps track of the current random value, updated every trigger. Some regions use lorand/hirand opcodes to decide randomy whether to activate or not, and this is the random value they will compare to.
	float m_rand = 0.0f;

	//! Stores the current value of all the midi CC knobs/controllers
	//! Technically, floats should probably be used to allow for HDCC (high definition CC's) as used in ARIA, but for now dividing by 127 to get a float between 0 and 1 works fine.
	std::array<int, NumMidiCCs> m_ccValues = {};
};


} // namespace lmms


#endif // LMMS_SFZ_GLOBAL_STATE_H
