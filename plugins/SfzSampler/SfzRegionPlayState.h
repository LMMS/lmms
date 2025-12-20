
#ifndef LMMS_SFZ_REGION_PLAY_STATE_H
#define LMMS_SFZ_REGION_PLAY_STATE_H

#include "SampleFrame.h"
#include "SfzTrigger.h"
#include "SfzOpcodeState.h"

namespace lmms
{

class SfzRegion;


class SfzRegionPlayState
{
public:
	SfzRegionPlayState(const SfzRegion* region, const SfzTrigger& trigger);
	SfzRegionPlayState() = default; // Needed to initialize array

	//! Generates the next buffer of audio from this sound. If m_active is false, this function does nothing.
	//! Returns true if any sound was generated, false if the buffer is left untouched
	bool play(SampleFrame* buffer, const fpp_t frames);

	//! Handle incoming event to decide whether to deactivate/release
	void processTrigger(const SfzTrigger& trigger);

	bool active() const { return m_active; }

private:
	//! Stores whether this object still represents a sound which exists.
	//! This will be true when the sound is active, but will become false once the release has ended or it has been forcefully deactivated.
	bool m_active = false;
	//! Stores whether the note has been released yet
	bool m_released = false;

	//! The number of frames since the start of the sound. This may be negative if a note starts partway through a buffer.
	int frameCount = 0;
	//! The frame at which the note was released, relative to the start of the note
	int releaseFrame = 0;

	//! The trigger event which caused this sound
	SfzTrigger m_trigger;
	
	//! The region this sound originated from
	const SfzRegion* m_region = nullptr;
};


} // namespace lmms


#endif // LMMS_SFZ_REGION_PLAY_STATE_H