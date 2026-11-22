/*
 * SfzControlsConfig.h - Helper class for storing info about which keyswitches exist, and which midi CC's are used by the sfz
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


#ifndef LMMS_SFZ_CONTROLS_CONFIG_H
#define LMMS_SFZ_CONTROLS_CONFIG_H

#include "SfzOpcodeState.h"

namespace lmms
{

//! Helper class for storing info about which keyswitches exist, and which midi CC's are used by the sfz
class SfzControlsConfig : public SfzOpcodeState
{
public:
	//! Stores information about each switch key, to make it easy to display on the GUI
	struct SwitchKeyInfo
	{
		QString swLabel;
		std::optional<int> swDefault;
		int swLokey;
		int swHikey;
	};

	std::map<int, SwitchKeyInfo>& switchKeyInfo() { return m_switchKeyInfo; }
	// Const version of getter for SfzGlobalState which stores a const version of SfzControlsConfig
	const std::map<int, SwitchKeyInfo>& switchKeyInfo() const { return m_switchKeyInfo; }
	std::array<bool, NumMidiCCs>& activeMidiCCs() { return m_activeMidiCCs; }

private:
	std::map<int, SwitchKeyInfo> m_switchKeyInfo;
	//! Keeps track of which CC's are actually used by the instrument, so that the GUI only needs to display those, not all 128
	std::array<bool, NumMidiCCs> m_activeMidiCCs = {};
};


} // namespace lmms


#endif // LMMS_SFZ_CONTROLS_CONFIG_H
