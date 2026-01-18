
#ifndef LMMS_SFZ_CONTROLS_CONFIG_H
#define LMMS_SFZ_CONTROLS_CONFIG_H

#include "SfzOpcodeState.h"

namespace lmms
{

class SfzControlsConfig : public SfzOpcodeState
{
public:
	//! Keeps track of which CC's are actually used by the instrument, so that the GUI only needs to display those, not all 128
	std::array<bool, SfzOpcodeState::NumMidiCCs> m_activeMidiCCs = {};

	//! Stores information about each switch key, to make it easy to display on the GUI
	struct SwitchKeyInfo
	{
		QString sw_label;
		std::optional<int> sw_default;
		int sw_lokey;
		int sw_hikey;
	};
	std::map<int, SwitchKeyInfo> m_switchKeyInfo;
};


} // namespace lmms


#endif // LMMS_SFZ_CONTROLS_CONFIG_H