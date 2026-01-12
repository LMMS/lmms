
#ifndef LMMS_SFZ_REGION_PLAY_STATE_H
#define LMMS_SFZ_REGION_PLAY_STATE_H

#include "SampleFrame.h"
#include "SfzTrigger.h"
#include "SfzOpcodeState.h"

#include "BasicFilters.h"

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

	//! Helper function for calculating the envelope value at the current frame
	float envelopeGenerator(const f_cnt_t delay, const f_cnt_t attack, const f_cnt_t hold, const f_cnt_t decay, const float sustain, const f_cnt_t release) const;

private:
	//! Stores whether this object still represents a sound which exists.
	//! This will be true when the sound is active, but will become false once the release has ended or it has been forcefully deactivated.
	bool m_active = false;
	//! Stores whether the note has been released yet
	bool m_released = false;

	//! The number of frames since the start of the sound. This may be negative if the region has delay or the note starts partway through a buffer.
	int m_frameCount = 0;
	//! The frame at which the note was released, relative to the start of the note
	int m_releaseFrame = 0;

	//! Stores the current frame index being played in the region's sample. This is a float, since interpolation is done to change pitch/sample rate
	float m_sampleFrame = 0;

	//! Some SFZ's utilize filters (e.g, a lowpass which changes cutoff depending on velocity), so each voice needs to have a filter object
	// TODO this assumes 2 channels--is that okay?
	BasicFilters<2> m_filter = {44100}; // This just initializes it to something so this class can be default-initialized in an array, but this will be reinitialized later.

	//! The trigger event which caused this sound
	SfzTrigger m_trigger;
	
	//! The region this sound originated from
	const SfzRegion* m_region = nullptr;
};


} // namespace lmms


#endif // LMMS_SFZ_REGION_PLAY_STATE_H