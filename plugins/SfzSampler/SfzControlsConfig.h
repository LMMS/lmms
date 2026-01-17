
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

	//! Stores a list of defined switch keys and their labels (if defined), so that the GUI can display a list of them to the user
	std::map<int, QString> m_switchKeyLabels;
};


} // namespace lmms


#endif // LMMS_SFZ_CONTROLS_CONFIG_H