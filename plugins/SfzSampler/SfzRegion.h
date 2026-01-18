
#ifndef LMMS_SFZ_REGION_H
#define LMMS_SFZ_REGION_H

#include "SfzOpcodeState.h"
#include "SfzGlobalState.h"
#include "SfzTrigger.h"
#include "SfzRegionPlayState.h"
#include "SfzSampleBuffer.h"
#include "SfzSamplePool.h"

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
	bool play(SampleFrame* workingBuffer, const fpp_t frames);

	//! Load the sample file given by the `sample` opcode into m_sample.
	//! The sample path is treated as relative to the path to the sfz file, so the parent directory is also needed
	//! Returns true if successful
	bool initializeSample(const QDir& parentDirectory, SfzSamplePool& samplePool);

	const SfzSampleBuffer* sample() const { return m_sample; }

private:
	static constexpr int MAX_ACTIVE_SOUNDS = 128;
	//! Array to store all active (and inactive) sound play states for this region
	std::array<SfzRegionPlayState, MAX_ACTIVE_SOUNDS> m_activeSounds;

	//! Pointer to sample object to be played. The sample file path is defined in the `sample` opcode, but the data needs to be loaded first
	//! The actual sample objects are stored in a shared pool, SfzSamplePool, so that if multiple of the same sample are loaded, they don't waste memory.
	const SfzSampleBuffer* m_sample = nullptr;

	//! Maximum active index in the play state array
	//! By always spawning new sounds at the lowest open index and keeping track of the maximum index which contains an actice sound,
	//! you only need to loop through the first n elements and ignore the rest since you know they are inactive (Thanks to Lost Robot for the idea)
	size_t m_maxActiveIndex = 0;
	
	//! Helper function to figure out what the maximum active index is, in the event the maximum index deacticated
	void recalculateMaxActiveIndex();

	//! In order to do round robin, the region needs to keep track of how many notes it has played in its lifetime. Or rather, the number of notes it *would* have played if it weren't restricted to only play a note when the round-robin counter hit the right numbers.
	size_t m_roundRobinCount = 0;

	//! Store the current total midi CC modulation amounts for the different targets, just so that we don't
	// have to recalculate them every buffer, instead only when a trigger occurs.
	float m_amplitude_totalCC = 0.0f;

	float m_ampeg_delay_totalCC = 0.0f;
	float m_ampeg_attack_totalCC = 0.0f;
	float m_ampeg_hold_totalCC = 0.0f;
	float m_ampeg_decay_totalCC = 0.0f;
	float m_ampeg_sustain_totalCC = 0.0f;
	float m_ampeg_release_totalCC = 0.0f;

	//! Helper function to calculate the total modulation of all midi CC controllers on a parameter. Essentially it just multiplies the modulation amounts by the current CC values and adds it all up.
	float totalCCModulation(const std::array<float, SfzOpcodeState::NumMidiCCs>& ccModulationAmounts, const SfzGlobalState& globalState) const;
	void recalculateTotalCCModulation(const SfzGlobalState& globalState);

	friend class SfzRegionPlayState; // TODO this was just to make it easy but...?
};


} // namespace lmms


#endif // LMMS_SFZ_REGION_H