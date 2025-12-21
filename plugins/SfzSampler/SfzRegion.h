
#ifndef LMMS_SFZ_REGION_H
#define LMMS_SFZ_REGION_H

#include "SfzOpcodeState.h"
#include "SfzGlobalState.h"
#include "SfzTrigger.h"
#include "SfzRegionPlayState.h"

#include "Sample.h"

#include <QDir>

namespace lmms
{

class SfzRegion : public SfzOpcodeState
{
public:
	SfzRegion(SfzOpcodeState opcodeState);

	//! Returns true if the trigger event matches all of the requirements defined by the opcodes of this region
	//! For example, if lokey=24 and hikey=29, and the trigger is a NoteOn event on key 26, then it will return true
	//! If the trigger does not fall in the key range or velocity range or any other conditions are not met (including global state conditions, 
	//! such as which switch key was last pressed) this will return false.
	bool triggerConditionsMet(const SfzGlobalState& globalState, const SfzTrigger& trigger);

	//! Handles spawning a new sound if the all the conditions are met
	//! Also handles sending the trigger to the currently active sounds, in case they need to deactivate/release (such as on NoteOff)
	void processTrigger(const SfzGlobalState& globalState, const SfzTrigger& trigger);

	//! Renders sound from each of the active SfzRegionPlayStates and writes it to the given buffer
	//! Returns true if any sound was actually generated
	bool play(SampleFrame* workingBuffer, SampleFrame* temporaryBuffer, const fpp_t frames);

	//! Load the sample file given by the `sample` opcode into m_sample.
	//! The sample path is treated as relative to the path to the sfz file, so the parent directory is also needed
	//! Returns true if successful
	bool initializeSample(const QDir& parentDirectory);

	const Sample& sample() const { return m_sample; }

private:
	static constexpr int MAX_ACTIVE_SOUNDS = 128;
	//! Array to store all active (and inactive) sound play states for this region
	std::array<SfzRegionPlayState, MAX_ACTIVE_SOUNDS> m_activeSounds;

	//! Sample object to be played. The sample file path is defined in the `sample` opcode, but the data needs to be loaded first
	Sample m_sample;
};


} // namespace lmms


#endif // LMMS_SFZ_REGION_H